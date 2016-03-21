// Copyright (c) 2016, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <platform.h>
#include "otp_board_info.h"
#include "ethernet.h"
#include "icmp.h"
#include "smi.h"
#include "debug_print.h"
#include "xtcp.h"
#include "web_server.h"

#define BOARD_X200_MC     (1)
#define BOARD_D4U_AVB_DG  (2)

#define BOARD BOARD_D4U_AVB_DG

xtcp_ipconfig_t ipconfig = {
        { 192, 168,   1, 178 },
        { 255, 255,   0,   0 },
        { 192, 168,   0,   1 }
};


#if (BOARD == X200_MC)
// These ports are for accessing the OTP memory
otp_ports_t otp_ports = on tile[0]: OTP_PORTS_INITIALIZER;

rgmii_ports_t rgmii_ports = on tile[1]: RGMII_PORTS_INITIALIZER;

port p_smi_mdio   = on tile[1]: XS1_PORT_1C;
port p_smi_mdc    = on tile[1]: XS1_PORT_1D;
port p_eth_reset  = on tile[1]: XS1_PORT_4A;
#endif

#if (BOARD == BOARD_D4U_AVB_DG)
// port p_eth_rstn   = on tile[0]:PORT_RSTN; // port 1O
on tile[0]:port p_eth_dummy  = PORT_SHARED_OUT_C0;
on tile[0]:port p_eth_rxclk  = PORT_ETH_RXCLK;
on tile[0]:port p_eth_rxd    = PORT_ETH_RXD;
on tile[0]:port p_eth_txd    = PORT_ETH_TXD;
on tile[0]:port p_eth_rxdv   = PORT_ETH_RXDV;
on tile[0]:port p_eth_txen   = PORT_ETH_TXEN;
on tile[0]:port p_eth_txclk  = PORT_ETH_TXCLK;
on tile[0]:port p_eth_rxerr  = PORT_ETH_RXER;
on tile[0]:clock eth_rxclk   = XS1_CLKBLK_2;
on tile[0]:clock eth_txclk   = XS1_CLKBLK_3;
on tile[0]:port p_smi_mdio   = PORT_ETH_MDIO;
on tile[0]:port p_smi_mdc    = PORT_ETH_MDC;
on tile[0]:out port p_eth_reset   = XS1_PORT_1O;
on stdcore[0]: otp_ports_t otp_ports = OTP_PORTS_INITIALIZER;
#define ETH_RX_BUFFER_SIZE_WORDS 1600
#endif


// An enum to manage the array of connections from the ethernet component
// to its clients.
enum eth_clients {
  ETH_TO_XTCP,
  NUM_ETH_CLIENTS
};

enum cfg_clients {
  CFG_TO_XTCP,
#if (BOARD == X200_MC)
  CFG_TO_PHY_DRIVER,
#endif
  NUM_CFG_CLIENTS
};

enum xtcp_clients {
    XTCP_TO_WEB,
    NUM_XTCP_CLIENTS
};

void reset_phy()
{
    uint32_t t;
    const int phy_reset_delay_ms = 1;
    timer tmr;
    tmr :> t;
    p_eth_reset <: 0;
    delay_milliseconds(phy_reset_delay_ms);
    p_eth_reset <: 1;
}


#if (BOARD == X200_MC)
[[combinable]]
void ar8035_phy_driver(client interface smi_if smi,
                client interface ethernet_cfg_if eth) {
  ethernet_link_state_t link_state = ETHERNET_LINK_DOWN;
  ethernet_speed_t link_speed = LINK_1000_MBPS_FULL_DUPLEX;
  const int link_poll_period_ms = 1000;
  const int phy_address = 0x4;
  timer tmr;
  int t;
  tmr :> t;
  reset_phy();

  while (smi_phy_is_powered_down(smi, phy_address));
  smi_configure(smi, phy_address, LINK_1000_MBPS_FULL_DUPLEX, SMI_ENABLE_AUTONEG);

  while (1) {
    select {
    case tmr when timerafter(t) :> t:
      ethernet_link_state_t new_state = smi_get_link_state(smi, phy_address);
      // Read AR8035 status register bits 15:14 to get the current link speed
      if (new_state == ETHERNET_LINK_UP) {
        link_speed = (ethernet_speed_t)(smi.read_reg(phy_address, 0x11) >> 14) & 3;
      }
      if (new_state != link_state) {
        link_state = new_state;
        eth.set_link_state(0, new_state, link_speed);
      }
      t += link_poll_period_ms * XS1_TIMER_KHZ;
      break;
    }
  }
}
#endif

#define N_CONNECTIONS 1
struct {
        xtcp_connection_t conn;
} connections[N_CONNECTIONS];
char received_data[XTCP_CLIENT_BUF_SIZE];
int n_received;

void swapcase(char *data, int n)
{
        int i;
        for (i = 0; i < n; i++) {
                if ((data[i] >= 'a') &&
                    (data[i] <= 'z'))
                        data[i] -= ('a' - 'A');
                else if ((data[i] >= 'A') &&
                         (data[i] <= 'Z'))
                        data[i] -= ('A' - 'a');
        }
}

void audio_handle_event (chanend c_xtcp, xtcp_connection_t &conn)
{
        if (conn.local_port != 9000)
                return;
        switch (conn.event) {
        case XTCP_NEW_CONNECTION:
                if (conn.local_port == 9000)
                        connections[0].conn = conn; // stash away the connection for later comparison
                break;
        case XTCP_RECV_DATA:
                if (conn.local_port == 9000) {
                        n_received = xtcp_recv(c_xtcp, received_data);
                        swapcase(received_data, n_received);
                        xtcp_init_send(c_xtcp, conn);
                }
                break;
        case XTCP_PUSH_DATA:
                break;
        case XTCP_REQUEST_DATA:
        case XTCP_RESEND_DATA:
                if (conn.local_port == 9000)
                        xtcp_send(c_xtcp, received_data, n_received);
                break;
        case XTCP_SENT_DATA:
                if (conn.local_port == 9000)
                        xtcp_complete_send(c_xtcp);
                break;
        case XTCP_ABORTED:
        case XTCP_CLOSED:
        case XTCP_TIMED_OUT:
        case XTCP_POLL:
        case XTCP_ALREADY_HANDLED:
                break;
        case XTCP_IFUP:
                break;
        case XTCP_IFDOWN:
                break;
        default:
                break;
        }
}


void tcp_handler(chanend c_xtcp) {
        xtcp_connection_t conn;
        web_server_init(c_xtcp, null, null);
        xtcp_listen(c_xtcp, 9000, XTCP_PROTOCOL_TCP);
        // Initialize your other code here
        while (1) {
                select
                {
                case xtcp_event(c_xtcp,conn):
                        audio_handle_event(c_xtcp, conn);
                        web_server_handle_event(c_xtcp, null, null, conn);
                        break;
                }
        }
}

int main()
{
  ethernet_cfg_if i_cfg[NUM_CFG_CLIENTS];
  ethernet_rx_if i_rx[NUM_ETH_CLIENTS];
  ethernet_tx_if i_tx[NUM_ETH_CLIENTS];
  chan c_xtcp[NUM_XTCP_CLIENTS];
  
  smi_if i_smi;
#if (BOARD == X200_MC)
  streaming chan c_rgmii_cfg;
#endif

  par {
#if (BOARD == X200_MC)
    on tile[1]: rgmii_ethernet_mac(i_rx, NUM_ETH_CLIENTS,
                                   i_tx, NUM_ETH_CLIENTS,
                                   null, null,
                                   c_rgmii_cfg,
                                   rgmii_ports, 
                                   ETHERNET_DISABLE_SHAPER);
    on tile[1].core[0]: rgmii_ethernet_mac_config(i_cfg, NUM_CFG_CLIENTS, c_rgmii_cfg);
    on tile[1].core[0]: ar8035_phy_driver(i_smi, i_cfg[CFG_TO_PHY_DRIVER]);
    on tile[1]: smi(i_smi, p_smi_mdio, p_smi_mdc);
    on tile[0]: xtcp(c_xtcp, NUM_XTCP_CLIENTS,
                     NULL, // mii
                     i_cfg[CFG_TO_XTCP], i_rx[ETH_TO_XTCP], i_tx[ETH_TO_XTCP],
                     NULL, 0, // SMI & phy addresss
                     NULL, // mac address
                     otp_ports, // for mac address
                     ipconfig);
#endif
#if (BOARD == BOARD_D4U_AVB_DG)
    on tile[0]: { reset_phy();
                    mii_ethernet_mac(i_cfg, NUM_CFG_CLIENTS,
                                 i_rx, NUM_ETH_CLIENTS,
                                 i_tx, NUM_ETH_CLIENTS,
                                 p_eth_rxclk, p_eth_rxerr,
                                 p_eth_rxd, p_eth_rxdv,
                                 p_eth_txclk, p_eth_txen, p_eth_txd,
                                 p_eth_dummy,
                                 eth_rxclk, eth_txclk,
                                 ETH_RX_BUFFER_SIZE_WORDS);
    }
    on tile[0]: smi(i_smi, p_smi_mdio, p_smi_mdc);
    on tile[0]: xtcp(c_xtcp, NUM_XTCP_CLIENTS,
                     NULL, // mii
                     i_cfg[CFG_TO_XTCP], i_rx[ETH_TO_XTCP], i_tx[ETH_TO_XTCP],
                     i_smi, 4, // SMI & phy addresss
                     NULL, // mac address
                     otp_ports, // for mac address
                     ipconfig);

#endif
    on tile[0]: tcp_handler(c_xtcp[XTCP_TO_WEB]);
  }
  return 0;
}

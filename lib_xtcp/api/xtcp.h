// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef __xtcp_h__
#define __xtcp_h__

#include "mii.h"
#include "smi.h"
#include "ethernet.h"
#include "otp_board_info.h"
#include <xccompat.h>

#include "xtcp_conf_derived.h"
#ifndef XTCP_CLIENT_BUF_SIZE
#define XTCP_CLIENT_BUF_SIZE (1472)
#endif
#ifndef XTCP_MAX_RECEIVE_SIZE
#ifdef UIP_CONF_RECEIVE_WINDOW
#define XTCP_MAX_RECEIVE_SIZE (UIP_CONF_RECEIVE_WINDOW)
#else
#define XTCP_MAX_RECEIVE_SIZE (1472)
#endif
#endif

/** Please note that IPv6 is not officially supported and use of this
 *   define should be considered experimental at the user's own risk
 */
#if IPV6
#define UIP_CONF_IPV6 1
#else
#define UIP_CONF_IPV6 0
#endif

typedef unsigned int xtcp_appstate_t;

#if UIP_CONF_IPV6
/** XTCP IP address.
 *
 *  This data type represents a single ipv6 address in the XTCP
 *  stack.
 */
typedef union xtcp_ip6addr_t {
  unsigned char  u8[16];			/* Initialiser, must come first. */
  unsigned short u16[8];
} xtcp_ip6addr_t;

typedef xtcp_ip6addr_t xtcp_ipaddr_t;

#else /* UIP_CONF_IPV6 -> UIP_CONF_IPV4 */

/** XTCP IP address.
 *
 *  This data type represents a single ipv4 address in the XTCP
 *  stack.
 */
typedef unsigned char xtcp_ipaddr_t[4];

#endif /* UIP_CONF_IPV6 */

/** IP configuration information structure.
 *
 *  This structure describes IP configuration for an ip node.
 *
 **/
#if UIP_CONF_IPV6
typedef struct xtcp_ipconfig_t {
  int v;		           /**< used ip protocol version */
  xtcp_ipaddr_t ipaddr;    /**< The IP Address of the node */
} xtcp_ipconfig_t;
#else
typedef struct xtcp_ipconfig_t {
  xtcp_ipaddr_t ipaddr;    /**< The IP Address of the node */
  xtcp_ipaddr_t netmask;   /**< The netmask of the node. The mask used
                                to determine which address are routed locally.*/
  xtcp_ipaddr_t gateway;   /**< The gateway of the node */
} xtcp_ipconfig_t;
#endif

/** XTCP protocol type.
 *
 * This determines what type a connection is: either UDP or TCP.
 *
 **/
typedef enum xtcp_protocol_t {
  XTCP_PROTOCOL_TCP, /**< Transmission Control Protocol */
  XTCP_PROTOCOL_UDP  /**< User Datagram Protocol */
} xtcp_protocol_t;


/** XTCP event type.
 *
 *  The event type represents what event is occuring on a particualr connection.
 *  It is instantiated when an event is received by the client using the
 *  xtcp_event() function.
 *
 **/
typedef enum xtcp_event_type_t {
  XTCP_NEW_CONNECTION,  /**<  This event represents a new connection has been
                              made. In the case of a TCP server connections it
                              occurs when a remote host firsts makes contact
                              with the local host. For TCP client connections
                              it occurs when a stream is setup with the remote
                              host.
                              For UDP connections it occurs as soon as the
                              connection is created.        **/

  XTCP_RECV_DATA,       /**<  This event occurs when the connection has received
                              some data. The client **must** follow receipt of
                              this event with a call to xtcp_recv() before
                              any other interaction with the server. **/

  XTCP_PUSH_DATA,       /**<  This event occurs when the connection has received
                              a packet with the TCP push flag set indicating
                              that the other side has temporarily finished
                              sending data.    **/

  XTCP_REQUEST_DATA,    /**<  This event occurs when the server is ready to send
                              data and is requesting that the client send data.
                              This event happens after a call to
                              xtcp_init_send() from the client.
                              The client **must** follow receipt of this event
                              with a call to xtcp_send() before any other
                              interaction with the server. */

  XTCP_SENT_DATA,       /**<  This event occurs when the server has successfully
                              sent the previous piece of data that was given
                              to it via a call to xtcp_send(). The server
                              is now requesting more data so the client
                              **must** follow receipt of this event
                              with a call to xtcp_send() before any other
                              interaction with the server. */

  XTCP_RESEND_DATA,    /**<  This event occurs when the server has failed to
                              send the previous piece of data that was given
                              to it via a call to xtcp_send(). The server
                              is now requesting for the same data to be sent
                              again. The client
                              **must** follow receipt of this event
                              with a call to xtcp_send() before any other
                              interaction with the server. */

  XTCP_TIMED_OUT,      /**<   This event occurs when the connection has
                              timed out with the remote host (TCP only).
                              This event represents the closing of a connection
                              and is the last event that will occur on
                              an active connection. */

  XTCP_ABORTED,        /**<   This event occurs when the connection has
                              been aborted by the local or remote host
                              (TCP only).
                              This event represents the closing of a connection
                              and is the last event that will occur on
                              an active connection. */

  XTCP_CLOSED,         /**<   This event occurs when the connection has
                              been closed by the local or remote host.
                              This event represents the closing of a connection
                              and is the last event that will occur on
                              an active connection. */

  XTCP_POLL,           /**<   This event occurs at regular intervals per
                              connection. Polling can be initiated and
                              the interval can be set with
                              xtcp_set_poll_interval() */

  XTCP_IFUP,           /**<   This event occurs when the link goes up (with
                              valid new ip address). This event has no
                              associated connection. */

  XTCP_IFDOWN,         /**<   This event occurs when the link goes down.
                              This event has no associated connection. */

  XTCP_ALREADY_HANDLED /**<   This event type does not get set by the server
                              but can be set by the client to show an event
                              has been handled */
} xtcp_event_type_t;

/** Type representing a connection type.
 *
 */
typedef enum xtcp_connection_type_t {
  XTCP_CLIENT_CONNECTION,  /**< A client connection */
  XTCP_SERVER_CONNECTION   /**< A server connection */
} xtcp_connection_type_t;


/** This type represents a TCP or UDP connection.
 *
 *  This is the main type containing connection information for the client
 *  to handle. Elements of this type are instantiated by the xtcp_event()
 *  function which informs the client about an event and the connection
 *  the event is on.
 *
 **/
typedef struct xtcp_connection_t {
  int id;  /**< A unique identifier for the connection */
  xtcp_protocol_t protocol; /**< The protocol of the connection (TCP/UDP) */
  xtcp_connection_type_t connection_type; /**< The type of connection (client/sever) */
  xtcp_event_type_t event; /**< The last reported event on this connection. */
  xtcp_appstate_t appstate; /**< The application state associated with the
                                 connection.  This is set using the
                                 xtcp_set_connection_appstate() function. */
  xtcp_ipaddr_t remote_addr; /**< The remote ip address of the connection. */
  unsigned int remote_port;  /**< The remote port of the connection. */
  unsigned int local_port;  /**< The local port of the connection. */
  unsigned int mss;  /**< The maximum size in bytes that can be send using
                        xtcp_send() after a send event */
#ifdef XTCP_ENABLE_PARTIAL_PACKET_ACK
  unsigned int outstanding; /**< The amount left inflight after a partial packet has been acked */
#endif
} xtcp_connection_t;


/** \brief Convert a unsigned integer representation of an ip address into
 *         the xtcp_ipaddr_t type.
 *
 * \param ipaddr The result ipaddr
 * \param i      An 32-bit integer containing the ip address (network order)
 * \note         Not available for IPv6
 */
void xtcp_uint_to_ipaddr(xtcp_ipaddr_t ipaddr, unsigned int i);

/** \brief Listen to a particular incoming port.
 *
 *  After this call, when a connection is established an
 *  XTCP_NEW_CONNECTION event is signalled.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param port_number the local port number to listen to
 * \param proto       the protocol to listen to (TCP or UDP)
 */
void xtcp_listen(chanend c_xtcp, int port_number, xtcp_protocol_t proto);

/** \brief Stop listening to a particular incoming port. Applies to TCP
 *  connections only.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param port_number local port number to stop listening on
 */
void xtcp_unlisten(chanend c_xtcp, int port_number);

/** \brief Try to connect to a remote port.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param port_number the remote port to try to connect to
 * \param ipaddr      the ip addr of the remote host
 * \param proto       the protocol to connect with (TCP or UDP)
 */
void xtcp_connect(chanend c_xtcp,
                  int port_number,
                  xtcp_ipaddr_t ipaddr,
                  xtcp_protocol_t proto);


/** \brief Bind the local end of a connection to a particular port (UDP).
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 * \param port_number the local port to set the connection to
 */
void xtcp_bind_local(chanend c_xtcp,
                     REFERENCE_PARAM(xtcp_connection_t,  conn),
                     int port_number);

/** \brief Bind the remote end of a connection to a particular port and
 *         ip address.
 *
 * This is only valid for XTCP_PROTOCOL_UDP connections.
 * After this call, packets sent to this connection will go to
 * the specified address and port
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 * \param addr        the intended remote address of the connection
 * \param port_number the intended remote port of the connection
 */
void xtcp_bind_remote(chanend c_xtcp,
                      REFERENCE_PARAM(xtcp_connection_t, conn),
                      xtcp_ipaddr_t addr, int port_number);


/** \brief Receive the next connect event.
 *
 *  \note This can be used in a select statement.
 *
 *  Upon receiving the event, the xtcp_connection_t structure conn
 *  is instatiated with information of the event and the connection
 *  it is on.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection relating to the current event
 */
#ifdef __XC__
transaction xtcp_event(chanend c_xtcp, xtcp_connection_t &conn);
#else
void do_xtcp_event(chanend c_xtcp,  xtcp_connection_t *conn);
#define xtcp_event(x,y) do_xtcp_event(x,y)
#endif

/** \brief Initiate sending data on a connection.
 *
 *  After making this call, the
 *  server will respond with a XTCP_REQUEST_DATA event when it is
 *  ready to accept data.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 */
void xtcp_init_send(chanend c_xtcp,
                    REFERENCE_PARAM(xtcp_connection_t, conn));




/** \brief Set the connections application state data item
 *
 * After this call, subsequent events on this connection
 * will have the appstate field of the connection set
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 * \param appstate    An unsigned integer representing the state. In C
 *                    this is usually a pointer to some connection dependent
 *                    information.
 */
void xtcp_set_connection_appstate(chanend c_xtcp,
                                  REFERENCE_PARAM(xtcp_connection_t, conn),
                                  xtcp_appstate_t appstate);

/** \brief Close a connection.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 */
void xtcp_close(chanend c_xtcp,
                REFERENCE_PARAM(xtcp_connection_t,conn));

/** \brief Abort a connection.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 */
void xtcp_abort(chanend c_xtcp,
                REFERENCE_PARAM(xtcp_connection_t,conn));


/**  \brief Receive data from the server
 *
 *   This can be called after an XTCP_RECV_DATA event.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param data        A array to place the received data into
 * \returns           The length of the received data in bytes
 */
int xtcp_recv(chanend c_xtcp, char data[]);

/** \brief Ignore data from the server.
 *
 *  This can be called after an XTCP_RECV_DATA event.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 */
void xtcp_ignore_recv(chanend c_xtcp);


/** \brief Receive data from the xtcp server
 *
 *  This can be called after an XTCP_RECV_DATA event.
 *
 *  The data is put into the array data starting at index i i.e.
 *  the first byte of data is written to data[i].
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param data        A array to place the received data into
 * \param i           The index where to start filling the data array
 * \returns           The length of the received data in bytes
 */
int xtcp_recvi(chanend c_xtcp, char data[], int i);

/** \brief Receive a number of bytes of data from the xtcp server
 *
 *  This can be called after an XTCP_RECV_DATA event.
 *
 *  Data is pulled from the xtcp server and put into the array, until
 *  either there is no more data to pull, or until count bytes have been
 *  received.  If there are more bytes to be received from the server then
 *  the remainder are discarded.  The return value reflects the number of
 *  bytes pulled from the server, not the number stored in the buffer. From
 *  this the user can determine if they have lost some data.
 *
 *  \note see the buffer client protocol for a mechanism for receiving bytes
 *        without discarding the extra ones.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param data        A array to place the received data into
 * \param count       The number of bytes to receive
 * \returns           The length of the received data in bytes, whether this
 *                    was more or less than the requested amount.
 */
int xtcp_recv_count(chanend c_xtcp, char data[], int count);

/** \brief Set a connection into ack-receive mode.
 *
 *  In ack-receive mode after a receive event the tcp window will be set to
 *  zero for the connection (i.e. no more data will be received from the other end).
 *  This will continue until the client calls the xtcp_ack_recv functions.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 */
void xtcp_ack_recv_mode(chanend c_xtcp,
                        REFERENCE_PARAM(xtcp_connection_t,conn)) ;


/** \brief Ack a receive event
 *
 * In ack-receive mode this command will acknowledge the last receive and
 * therefore
 * open the receive window again so new receive events can occur.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param conn        the connection
 **/
void xtcp_ack_recv(chanend c_xtcp,
                   REFERENCE_PARAM(xtcp_connection_t,conn));


/** \brief Send data to the xtcp server
 *
 *  Send data to the server. This should be called after a
 *  XTCP_REQUEST_DATA, XTCP_SENT_DATA or XTCP_RESEND_DATA event
 *  (alternatively xtcp_write_buf can be called).
 *  To finish sending this must be called with a length  of zero or
 *  call the xtcp_complete_send() function.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param data        An array of data to send
 * \param len         The length of data to send. If this is 0, no data will
 *                    be sent and a XTCP_SENT_DATA event will not occur.
 */
void xtcp_send(chanend c_xtcp,
               NULLABLE_ARRAY_OF(char, data),
               int len);

/** \brief Complete a send transaction with the server.
 *
 *  This function can be called after a
 *  XTCP_REQUEST_DATA, XTCP_SENT_DATA or XTCP_RESEND_DATA event
 *  to finish any sending on the connection that the event
 *  related to.
 *
 *  \param c_xtcp   chanend connected to the tcp server
 */
inline void xtcp_complete_send(chanend c_xtcp) {
#ifdef __XC__
  xtcp_send(c_xtcp, null, 0);
#else
  xtcp_send(c_xtcp, (void *) 0, 0);
#endif
}

#define xtcp_ignore_send xtcp_complete_send

/** \brief Send data to the xtcp server
 *
 *  Send data to the server. This should be called after a
 *  XTCP_REQUEST_DATA, XTCP_SENT_DATA or XTCP_RESEND_DATA event
 *  (alternatively xtcp_write_buf can be called).
 *  The data is sent starting from index i i.e. data[i] is the first
 *  byte to be sent.
 *  To finish sending this must be called with a length  of zero.
 *
 * \param c_xtcp      chanend connected to the xtcp serve
 * \param data        An array of data to send
 * \param i           The index at which to start reading from the data array
 * \param len         The length of data to send. If this is 0, no data will
 *                    be sent and a XTCP_SENT_DATA event will not occur.
 */
void xtcp_sendi(chanend c_xtcp,
                NULLABLE_ARRAY_OF(char, data),
                int i,
                int len);


/** \brief Set UDP poll interval.
 *
 *  When this is called then the udp connection will cause a poll event
 *  every poll_interval milliseconds.
 *
 * \param c_xtcp         chanend connected to the xtcp server
 * \param conn           the connection
 * \param poll_interval  the required poll interval in milliseconds
 */
void xtcp_set_poll_interval(chanend c_xtcp,
                            REFERENCE_PARAM(xtcp_connection_t, conn),
                            int poll_interval);

/** \brief Subscribe to a particular ip multicast group address
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param addr        The address of the multicast group to join. It is
 *                    assumed that this is a multicast IP address.
 * \note              Not available for IPv6
 */
void xtcp_join_multicast_group(chanend c_xtcp,
                               xtcp_ipaddr_t addr);

/** \brief Unsubscribe to a particular ip multicast group address
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param addr        The address of the multicast group to leave. It is
 *                    assumed that this is a multicast IP address which
 *                    has previously been joined.
 * \note              Not available for IPv6
 */
void xtcp_leave_multicast_group(chanend c_xtcp,
                               xtcp_ipaddr_t addr);

/** \brief Get the current host MAC address of the server.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param mac_addr    the array to be filled with the mac address
 **/
void xtcp_get_mac_address(chanend c_xtcp, unsigned char mac_addr[]);

/** \brief Get the IP config information into a local structure
 *
 * Get the current host IP configuration of the server.
 *
 * \param c_xtcp      chanend connected to the xtcp server
 * \param ipconfig    the structure to be filled with the IP configuration
 *                    information
 **/
void xtcp_get_ipconfig(chanend c_xtcp,
                       REFERENCE_PARAM(xtcp_ipconfig_t, ipconfig));


/** \brief pause a connection.
 *
 *  No further reads and writes will occur on the network.
 *  \param c_xtcp	chanend connected to the xtcp server
 *  \param conn		tcp connection structure
 *  \note         This functionality is considered experimental for when using IPv6.
 */
void xtcp_pause(chanend c_xtcp,
                REFERENCE_PARAM(xtcp_connection_t,conn));


/** \brief unpause a connection
 *
 *  Activity is resumed on a connection.
 *
 *  \param c_xtcp	chanend connected to the xtcp server
 *  \param conn		tcp connection structure
 *  \note         This functionality is considered experimental for when using IPv6.
 */
void xtcp_unpause(chanend c_xtcp,
                  REFERENCE_PARAM(xtcp_connection_t,conn));


/** \brief Enable a connection to accept acknowledgements of partial packets that have been sent.
 *
 *  \param c_xtcp	chanend connected to the xtcp server
 *  \param conn		tcp connection structure
 *  \note         This functionality is considered experimental for when using IPv6.
 */
void xtcp_accept_partial_ack(chanend c_xtcp,
                             REFERENCE_PARAM(xtcp_connection_t,conn));

#ifdef __XC__

/** Function implement the TCP/IP stack task.
 *
 *  This functions implements a TCP/IP stack that clients can access via
 *  xC channels.
 *
 *  \param c_xtcp     The channel array to connect to the clients.
 *  \param n          The number of clients to the task.
 *  \param i_mii      If this component is connected to the mii() component
 *                    in the Ethernet library then this interface should be
 *                    used to connect to it. Otherwise it should be set to
 *                    null
 *  \param i_eth_cfg  If this component is connected to an MAC component
 *                    in the Ethernet library then this interface should be
 *                    used to connect to it. Otherwise it should be set to
 *                    null.
 *  \param i_eth_rx   If this component is connected to an MAC component
 *                    in the Ethernet library then this interface should be
 *                    used to connect to it. Otherwise it should be set to
 *                    null.
 *  \param i_eth_tx   If this component is connected to an MAC component
 *                    in the Ethernet library then this interface should be
 *                    used to connect to it. Otherwise it should be set to
 *                    null.
 *  \param i_smi      If this connection to an Ethernet SMI component is
 *                    then the XTCP component will poll the Ethernet PHY
 *                    for link up/link down events. Otherwise, it will
 *                    expect link up/link down events from the connected
 *                    Ethernet MAC.
 *  \param phy_address The SMI address of the Ethernet PHY
 *  \param mac_address If this array is non-null then it will be used to set
 *                     the MAC address of the component.
 *  \param otp_ports   If this port structure is non-null then the component
 *                     will obtain the MAC address from OTP ROM. See the OTP
 *                     reading library user guide for details.
 *  \param ipconfig    This :c:type:`xtcp_ipconfig_t` structure is used
 *                     to determine the IP address configuration of the
 *                     component.
 */
void xtcp(chanend c_xtcp[n], size_t n,
          client mii_if ?i_mii,
          client ethernet_cfg_if ?i_eth_cfg,
          client ethernet_rx_if ?i_eth_rx,
          client ethernet_tx_if ?i_eth_tx,
          client smi_if ?i_smi,
          uint8_t phy_address,
          const char (&?mac_address)[6],
          otp_ports_t &?otp_ports,
          xtcp_ipconfig_t &ipconfig);

#endif

/** Utility functions **/

/** Copy an IP address data structure.
 */

#define XTCP_IPADDR_CPY(dest, src) XTCP_IPADDR_CPY_(dest, src)

/** Compare two IP address structures.
 */
#define XTCP_IPADDR_CMP(a, b) XTCP_IPADDR_CMP_(a, b)

#include "xtcp_impl.h"

#endif // __xtcp_h__


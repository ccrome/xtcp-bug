LIB_XTCP Test projects
=======================

This is a repo to help debug the lib_xtcp XMOS code.

The main app to compile is AN00199_gigabit_ethernet_demo, which is
modified to included a webserver  using xtcp. 

With the patches included here, the webserver can perform quite well
(over 2000 requests/second) as opposed to without the patch (25
requests/second).

The 'test.py' script included here stresses the webserver.

This code can switch between the xCORE 200 (gigabit) and the DSP4YOU
bord (100MBit).  to do so, change the #define BOARD in main.xc, and
also, change the Makefile so it includes the proper .xn.

This patch seems to work okay (not great performance) on the xCORE 200 MC board:

diff --git a/lib_xtcp/src/xtcp.xc b/lib_xtcp/src/xtcp.xc
index 74896a5..caf45c4 100755
--- a/lib_xtcp/src/xtcp.xc
+++ b/lib_xtcp/src/xtcp.xc
@@ -119,6 +119,10 @@ void xtcp(chanend xtcp[n], size_t n,
   timeout += 10000000;
 
   while (1) {
+      xtcpd_service_clients(xtcp, n);
+      xtcpd_check_connection_poll();
+      uip_xtcp_checkstate();
+      xtcp_process_udp_acks();
     unsafe {
     select {
     case !isnull(i_mii) => mii_incoming_packet(mii_info):
@@ -155,11 +159,6 @@ void xtcp(chanend xtcp[n], size_t n,
     case tmr when timerafter(timeout) :> timeout:
       timeout += 10000000;
 
-      xtcpd_service_clients(xtcp, n);
-      xtcpd_check_connection_poll();
-      uip_xtcp_checkstate();
-      xtcp_process_udp_acks();
-
       // Check for the link state
       if (!isnull(i_smi))
       {
@@ -197,6 +196,8 @@ void xtcp(chanend xtcp[n], size_t n,
 
       xtcp_process_periodic_timer();
       break;
+    default:
+        break;
     }
     }
   }
       



Another Bug.
-------

This works reliably on the xCORE 200 MC board with gigabit ethernet,
even though performance isn't great. 

However, when I move over to the DSP4YOU board, with 100mbit ethernet,
it fails.  It seems to work for a while, but pretty quickly dies.

In xTIME composer, on tile[0] core[2]. I get (Suspended: Signal
'ET_LOAD_STORE' received.  Description: Memory access exception.).

If I access slowly, without any stress, it seems to work okay.

To get it to fail, I simply do:

ping -f 192.168.1.178


It also fails with http access, ping access, whatever.

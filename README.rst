This is a repo to help debug the lib_xtcp XMOS code.

The main app to compile is AN00199_gigabit_ethernet_demo, which is
modified to included a webserver  using xtcp. 

With the patches included here, the webserver can perform quite well
(over 2000 requests/second) as opposed to without the patch (25
requests/second).

The 'test.py' script included here stresses the webserver.

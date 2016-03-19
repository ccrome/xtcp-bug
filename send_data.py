import time
import socket


TCP_IP = "192.168.1.178"
TCP_PORT = 9000
BUFFER_SIZE = 640
message = "x" * BUFFER_SIZE
loops = 1000

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
t1 = time.time()
for i in range(loops):
    s.send(message)
    data = s.recv(BUFFER_SIZE)
s.close()

t2 = time.time()
delta_t = t2-t1
bytes = len(message) * loops
bytes_per_second = bytes / delta_t
print "Send %f Mbytes in %f seconds for %f million bytes/second" % (bytes*1e-6, delta_t, bytes_per_second/1e6)
print "at 48kHz, we need 16 channels * 48000 * 2 = %f bytes/second" % ((16*48000.0*2)/1e6)
print "at 16kHz, we need 16 channels * 16000 * 2 = %f bytes/second" % ((16*16000.0*2)/1e6)

import threading
import urllib2
import hashlib
import time

requests = 1000
nthreads = 5
url = "http://192.168.1.178/index.html"

def gettext(url):
    res = urllib2.urlopen(url)
    return res.read()
    
def worker(num, url, md5):
    for i in range(requests):
        m = hashlib.md5()
        m.update(gettext(url))
        if (md5 != m.hexdigest()):
            raise Exception("Oops, the hash didn't match")

m = hashlib.md5()
m.update(gettext(url))
md5 = m.hexdigest()
threads = []


t1 = time.time()
print "Starting test at time: ", t1
for i in range(nthreads):
    t = threading.Thread(target=worker, args=(i,url,md5))
    threads.append(t)
    t.start()
for i in range(nthreads):
    threads[i].join()

t2 = time.time()
diff = t2 - t1
tps = (requests*nthreads)/diff
print "Completed %d requests in time %f seconds, or %f requests/second" % (requests * nthreads, diff, tps)


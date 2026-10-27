import socket
import time
import sys
import threading

HOST = '127.0.0.1'
PORT = int(sys.argv[1])
NTOTAL = int(sys.argv[2])  # 总请求数
NCONN = int(sys.argv[3])   # 并发工作线程数

lock = threading.Lock()
done = 0
errors = 0


def worker():
    global done, errors
    while True:
        with lock:
            if done >= NTOTAL:
                return
            done += 1
        try:
            s = socket.create_connection((HOST, PORT))
            s.sendall(b'TIME')
            s.recv(200)
            s.close()
        except Exception:
            with lock:
                errors += 1


start = time.time()
threads = [threading.Thread(target=worker) for _ in range(NCONN)]
for t in threads:
    t.start()
for t in threads:
    t.join()
elapsed = time.time() - start

ok = NTOTAL - errors
print(f"{ok}/{NTOTAL} requests in {elapsed:.2f}s = {ok/elapsed:.0f} req/s")

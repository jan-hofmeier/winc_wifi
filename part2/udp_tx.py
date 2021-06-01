# Simple UDP client in Python
import socket, time

ADDR = "10.1.1.11"
PORT = 1025
MESSAGE = b"Hello, World %u"
DELAY = 1

def hex_str(bytes):
    return " ".join([("%02x" % int(b)) for b in bytes])

print("Send to UDP %s:%s" % (ADDR, PORT))
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(0.2)
count = 1
while True:
    msg = MESSAGE % count
    sock.sendto(msg, (ADDR, PORT))
    print("Tx %u: %s" % (len(msg), hex_str(msg)))
    count += 1
    try:
        data = sock.recvfrom(1000)
    except:
        data = None
    if data:
        bytes, addr = data
        s = hex_str(bytes)
        print("Rx %u: %s\n" % (len(bytes), s))
    time.sleep(DELAY)
# EOF


import socket
s = socket.create_connection(('127.0.0.1', 8585))
data = b'hello' * 1000
s.sendall(data)
s.shutdown(socket.SHUT_WR)

total = 0
while True:
    chunk = s.recv(1024)
    if not chunk:
        break
    total += len(chunk)
print("receive bytes:", total)

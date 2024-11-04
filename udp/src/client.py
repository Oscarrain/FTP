import socket

size = 8192

try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    for i in range(51):  # 发送 0 到 50 的消息
        msg = str(i)
        sock.sendto(msg.encode('utf-8'), ('localhost', 9876))
        response = sock.recv(size).decode('utf-8')
        print(response)
    sock.close()

except Exception as e:
    print(f"Cannot reach the server: {e}")

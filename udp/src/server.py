import socket

size = 8192
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 9876))

# 初始化计数器
message_count = 0

try:
    while True:
        data, address = sock.recvfrom(size)
        message_count += 1  # 更新消息计数
        response = f"{message_count} {data.decode('utf-8')}"
        sock.sendto(response.encode('utf-8'), address)
finally:
    sock.close()

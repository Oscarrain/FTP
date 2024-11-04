# FTP Project

This project is an implementation of an FTP (File Transfer Protocol) client and server in C using socket programming. The client and server are built to support basic FTP operations, such as file upload, file download, listing directories, and handling active and passive connection modes. 

## Features
- **Client-Server Model**: Uses socket programming to create a connection between the client and server.
- **Basic FTP Operations**: Includes commands such as `LIST`, `RETR`, `STOR`, and `QUIT`.
- **Active and Passive Modes**: Supports both active (PORT) and passive (PASV) modes of data transfer.
- **Robust Error Handling**: Ensures proper communication and error reporting between the client and server.
- **Cross-Platform Support**: Works on Linux systems and MacOS on M Chips.

## Prerequisites
- A Linux-based environment
- GCC compiler for C
- Make utility to compile the project

## Installation and Compilation
1. Clone the repository:
   ```bash
   git clone https://github.com/Oscarrain/FTP
   cd FTP
   ```
2. Compile the project:
   ```bash
   make
   ```

## Running the Project
1. **Start the FTP Server**:
   ```bash
   ./ftp_server <port>
   ```
   - `<port>`: The port number the server will listen on.

2. **Run the FTP Client**:
   ```bash
   ./ftp_client <server_ip> <port>
   ```
   - `<server_ip>`: The IP address of the server.
   - `<port>`: The port number of the server.

## Supported FTP Commands
- **LIST**: Lists files in the current directory on the server.
- **RETR <filename>**: Downloads a file from the server.
- **STOR <filename>**: Uploads a file to the server.
- **QUIT**: Closes the FTP connection.

## Example Usage
1. Start the server:
   ```bash
   ./ftp_server 21
   ```
2. Connect with the client:
   ```bash
   ./ftp_client 127.0.0.1 21
   ```
3. Use commands such as `LIST`, `RETR <filename>`, and `STOR <filename>` to transfer files.

## Notes
- Make sure the server is running before starting the client.
- Ensure appropriate firewall rules are set to allow FTP communication.

---


# FTP 项目

此项目是在 C 语言中使用套接字编程实现的 FTP（文件传输协议）客户端和服务器。客户端和服务器支持基本的 FTP 操作，如文件上传、文件下载、目录列表，以及处理主动和被动连接模式。

## 功能
- **客户端-服务器模型**：使用套接字编程在客户端和服务器之间创建连接。
- **基本 FTP 操作**：包括 `LIST`、`RETR`、`STOR` 和 `QUIT` 等命令。
- **主动和被动模式**：支持主动（PORT）和被动（PASV）数据传输模式。
- **健壮的错误处理**：确保客户端和服务器之间的通信和错误报告正常。
- **跨平台支持**：在 Linux 系统上和装载M系列芯片的 MacOS 系统上运行。

## 前置条件
- 基于 Linux 的环境
- GCC C 编译器
- Make 工具用于编译项目

## 安装与编译
1. 克隆仓库：
   ```bash
   git clone https://github.com/Oscarrain/FTP
   cd FTP
   ```
2. 编译项目：
   ```bash
   make
   ```

## 运行项目
1. **启动 FTP 服务器**：
   ```bash
   ./ftp_server <端口号>
   ```
   - `<端口号>`：服务器监听的端口号。

2. **运行 FTP 客户端**：
   ```bash
   ./ftp_client <服务器IP> <端口号>
   ```
   - `<服务器IP>`：服务器的 IP 地址。
   - `<端口号>`：服务器的端口号。

## 支持的 FTP 命令
- **LIST**：列出服务器当前目录中的文件。
- **RETR <文件名>**：从服务器下载文件。
- **STOR <文件名>**：上传文件到服务器。
- **QUIT**：关闭 FTP 连接。

## 使用示例
1. 启动服务器：
   ```bash
   ./ftp_server 21
   ```
2. 使用客户端连接：
   ```bash
   ./ftp_client 127.0.0.1 21
   ```
3. 使用 `LIST`、`RETR <文件名>` 和 `STOR <文件名>` 等命令传输文件。

## 注意事项
- 确保在启动客户端之前已启动服务器。
- 确保设置适当的防火墙规则，以允许 FTP 通信。
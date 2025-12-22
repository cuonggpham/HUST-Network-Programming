# ===============================================
# FTP Client-Server - Huong dan build va chay
# ===============================================

## Yeu cau
- GCC compiler
- Linux OS
- pthread library

## Build thu cong

### Build Server:
```bash
cd server/src
gcc -o ../bin/server server.c account.c ftp_server.c -lpthread
```

### Build Client:
```bash
cd client/src
gcc -o ../bin/client client.c ftp_client.c
```

## Chay

### Chay Server:
```bash
cd server/bin
./server [port] [server_ip]

# Vi du:
./server 2121 192.168.1.100
```
- port: Port lang nghe (mac dinh 2121)
- server_ip: IP cua server cho PASV mode (mac dinh 127.0.0.1)

### Chay Client:
```bash
cd client/bin
./client <host> <port> [username] [password]

# Vi du:
./client localhost 2121 admin admin
```

## Cac lenh FTP ho tro
- `pwd` - Hien thi thu muc hien tai
- `ls` / `dir` - Liet ke file
- `cd <path>` - Doi thu muc
- `get <file>` - Download file
- `put <file>` - Upload file
- `quit` - Thoat

## Test voi FileZilla
1. Mo FileZilla
2. Nhap thong tin:
   - Host: localhost (hoac IP server)
   - Port: 2121
   - Username: admin
   - Password: admin
3. Nhan "Quickconnect"

## Quan ly tai khoan
Sua file `server/data/accounts.dat`:
```
# Format: username:password:root_directory
admin:admin:/tmp
user:user:/home/user
```

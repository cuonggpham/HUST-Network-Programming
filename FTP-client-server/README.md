Bạn hãy đóng vai một sinh viên đại học năm 3 ngành Công nghệ Thông tin,
đang làm bài tập lớn môn Mạng máy tính.

Programming Language: C 

Yêu cầu chung về phong cách code:
- Code đúng yêu cầu bài toán
- Viết ở mức bài tập lớn sinh viên, KHÔNG quá tối ưu
- KHÔNG dùng kiến trúc phức tạp, design pattern nâng cao
- Comment ngắn gọn, vừa đủ hiểu
- Cho phép hàm hơi dài, chưa refactor tối ưu

------------------------------------
BÀI TOÁN
------------------------------------

Xây dựng một ứng dụng FTP đơn giản gồm SERVER và CLIENT sử dụng socket TCP.

------------------------------------
SERVER CÓ CÁC CHỨC NĂNG SAU
------------------------------------

1. Quản lý tài khoản:
- Thêm tài khoản mới gồm: username, password, thư mục gốc
- Lưu danh sách tài khoản vào 1 file text
- Mỗi lần server khởi động sẽ đọc danh sách tài khoản từ file

2. Giao tiếp với client:
- Nhận các lệnh từ client
- Hiển thị lệnh nhận được trên màn hình server theo định dạng:
  hh:mm:ss <Lệnh> <Địa chỉ IP của client>

3. Xử lý các lệnh FTP cơ bản:
- Đăng nhập (USER, PASS)
- Xem danh sách file trong thư mục hiện thời (LIST)
- Đổi thư mục làm việc (CWD)
- Kiểm tra thư mục làm việc hiện thời (PWD)
- Download file từ server về client (RETR)
- Upload file từ client lên server (STOR)
- Ngắt kết nối (QUIT)

4. Yêu cầu tương thích:
- Server phải có khả năng làm việc với tối thiểu 1 client FTP của bên thứ 3
  (ví dụ: FileZilla)

------------------------------------
CLIENT CÓ CÁC CHỨC NĂNG SAU
------------------------------------

1. Gửi lệnh và thông tin đăng nhập đến server

2. Hiển thị phản hồi của server theo định dạng:
   hh:mm:ss <Lệnh gửi đi> <Phản hồi của server>

3. Nhập lệnh từ bàn phím:
- Sinh viên có thể sử dụng các lệnh FTP quen thuộc:
  PWD, CWD, LIST, RETR, STOR, QUIT

------------------------------------
YÊU CẦU TRIỂN KHAI
------------------------------------

- Dùng socket TCP
- Multi-threading cho server để xử lý nhiều client cùng lúc
- Sử dụng ngôn ngữ C
- Chạy được trên hệ điều hành Linux
- Sử dụng các thư viện chuẩn có sẵn trên Linux
- Code đơn giản, dễ hiểu
- Ưu tiên làm rõ luồng xử lý thay vì tối ưu hiệu năng

------------------------------------
OUTPUT
------------------------------------

- Code server hoàn chỉnh
- Code client hoàn chỉnh
- Nếu cần, giải thích ngắn gọn cách chạy và cách test bằng FileZilla

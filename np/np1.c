/*
 * CHUONG TRINH: Lay danh sach IP address cua may tinh
 *
 * KIEN THUC LAP TRINH MANG:
 * - Doc thong tin cau hinh mang tu lenh ifconfig
 * - Phan tich du lieu text de trich xuat IP address
 */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  // system(): Thuc thi lenh shell tu trong chuong trinh C
  // Ket qua cua ifconfig duoc redirect vao file out.txt de xu ly
  system("ifconfig > out.txt");

  // fopen(): Mo file de doc/ghi
  // "rt" = read text mode (doc file van ban)
  FILE *f = fopen("out.txt", "rt");

  // feof(): Kiem tra End Of File
  // Tra ve non-zero khi da doc het file
  while (!feof(f)) {
    // Array initialization voi {0}: Khoi tao tat ca phan tu mang = 0
    // Dam bao buffer sach truoc khi su dung
    char line[1024] = {0};

    // fgets(): Doc mot dong tu file
    // sizeof(line) - 1: Chừa 1 byte cho null terminator, tranh buffer overflow
    fgets(line, sizeof(line) - 1, f);

    // strstr(): Tim chuoi con trong chuoi cha
    // Tra ve con tro toi vi tri tim thay, NULL neu khong tim thay
    char *found = strstr(line, "inet ");
    if (found != NULL) {
      char inet[1024] = {0};
      char ip[1024] = {0};

      // sscanf(): Doc du lieu tu chuoi theo format (tuong tu scanf nhung tu
      // string) %s doc mot word (dung lai khi gap whitespace)
      sscanf(found, "%s%s", inet, ip);
      printf("%s\n", ip);
    }

    // Tim dia chi IPv6 tuong tu nhu IPv4
    found = strstr(line, "inet6 ");
    if (found != NULL) {
      char inet[1024] = {0};
      char ip[1024] = {0};
      sscanf(found, "%s%s", inet, ip);
      printf("%s\n", ip);
    }
  }

  // fclose(): Dong file sau khi su dung xong
  // Giai phong tai nguyen va flush buffer ra file
  fclose(f);
}
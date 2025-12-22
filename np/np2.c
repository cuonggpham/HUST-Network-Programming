/*
 * CHUONG TRINH: Noi chuoi - Doc nhieu dong va noi thanh mot chuoi lon
 *
 * KIEN THUC LAP TRINH MANG:
 * - Ky thuat xu ly chuoi dong (dynamic string) dung trong network protocols
 * - Buffer management cho du lieu nhan qua mang
 */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global variable: Bien toan cuc co the truy cap tu bat ky ham nao trong file
// Khoi tao = NULL de biet chua co du lieu
char *output = NULL;

// Double pointer (char **): Con tro toi con tro
// Dung khi can thay doi dia chi ma con tro tro toi (pass by reference cho
// pointer)
void Append(char **destination, const char *str) {
  // Dereferencing operator (*): Lay gia tri tai dia chi ma pointer tro toi
  // *destination la char* (chuoi hien tai)
  char *tmp = *destination;

  // Ternary operator (?:): if-else trong 1 dong
  // condition ? value_if_true : value_if_false
  int oldlen = tmp != NULL ? strlen(tmp) : 0;

  // realloc(): Cap phat lai bo nho dong (dynamic memory reallocation)
  // Giu nguyen du lieu cu, mo rong hoac thu nho vung nho
  // Neu tmp = NULL, hoat dong nhu malloc()
  tmp = (char *)realloc(tmp, oldlen + strlen(str) + 1);

  // sprintf(): Ghi du lieu vao chuoi theo format (tuong tu printf nhung ra
  // string) tmp + strlen(tmp): Con tro toi vi tri cuoi chuoi (ky tu '\0')
  // Pointer arithmetic: cong so vao pointer de di chuyen vi tri
  sprintf(tmp + strlen(tmp), "%s", str);

  // Cap nhat lai gia tri cua con tro ben ngoai ham thong qua double pointer
  *destination = tmp;
}

int main() {
  // Infinite loop: while(0 == 0) tuong duong while(true) hoac for(;;)
  while (0 == 0) {
    char tmp[1024] = {0};
    fgets(tmp, sizeof(tmp) - 1, stdin);

    // Loai bo cac ki tu xuong dong o cuoi chuoi
    // Windows dung \r\n (CRLF), Unix/Linux dung \n (LF)
    while (tmp[strlen(tmp) - 1] == '\r' || tmp[strlen(tmp) - 1] == '\n') {
      // Thay ki tu cuoi bang null terminator de "cat" chuoi
      tmp[strlen(tmp) - 1] = 0;
    }

    if (strlen(tmp) > 0) {
      // Address-of operator (&): Lay dia chi cua bien
      // &output la dia chi cua con tro output (kieu char**)
      Append(&output, tmp);
    } else
      break;
  }
  printf("%s\n", output);

  // free(): Giai phong bo nho da cap phat dong
  // QUAN TRONG: Tranh memory leak - luon free() sau khi dung xong
  // malloc/realloc
  free(output);
  output = NULL; // Dat ve NULL sau khi free de tranh dangling pointer
}

/*
 * CHUONG TRINH: Fork - Tao tien trinh con
 *
 * KIEN THUC LAP TRINH MANG:
 * - fork() la co so de tao multi-process server
 * - Moi client co the duoc xu ly boi mot process rieng biet
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  int x = 0;

  // fork(): Tao tien trinh con - SYSTEM CALL quan trong nhat cua Unix
  // Sau khi fork(), co 2 process chay DONG THOI:
  // - Parent process (process goc)
  // - Child process (ban sao cua parent)
  //
  // Gia tri tra ve:
  // - Trong PARENT: tra ve PID cua child (so duong)
  // - Trong CHILD: tra ve 0
  // - Loi: tra ve -1 (khong tao duoc child)
  //
  // SAU fork(), child COPY toan bo memory space cua parent
  // (thuc te dung Copy-on-Write de toi uu)
  if (fork() == 0) {
    // Code nay chi chay trong CHILD process (vi fork() == 0)
    x = x + 2; // x cua child = 2

    printf("Child: Hello Forking!\n");

    // exit(): Ket thuc process ngay lap tuc
    // Khac voi return: exit() ket thuc process tu bat ky dau
    // return chi thoat khoi ham hien tai
    // exit(0): Ket thuc thanh cong
    // exit(1): Ket thuc voi loi
    exit(0);
  } else {
    // Code nay chi chay trong PARENT process
    x = x + 1; // x cua parent = 1
    printf("Parent: Hello Forking!\n");

    // CHU Y QUAN TRONG:
    // x cua child va parent la HOAN TOAN DOC LAP sau fork()
    // Thay doi x trong child KHONG anh huong x trong parent va nguoc lai
    // Day la co so cua process isolation
  }

  // Doi phim Enter truoc khi thoat de thay output
  fgetc(stdin);
}
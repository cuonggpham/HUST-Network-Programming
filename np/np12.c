/*
 * CHUONG TRINH: Fork - Tao tien trinh con
 *
 * KIEN THUC LAP TRINH MANG:
 * - fork() la co so de tao multi-process server
 * - Moi client xu ly boi mot process rieng
 *
 * KIEN THUC C ADVANCE:
 * - fork(): Tao tien trinh con - Copy toan bo process hien tai
 * - Process ID: fork() tra ve 0 cho child, PID cua child cho parent
 * - exit(): Ket thuc process (khac voi return)
 * - Memory space: Parent va child co vung nho rieng biet sau khi fork
 * - Copy-on-write: He dieu hanh toi uu hoa viec copy bo nho
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    int x = 0;

    // fork() tao ban sao cua tien trinh hien tai
    // Tra ve 0 neu dang o child process
    // Tra ve PID cua child neu dang o parent process
    // Tra ve -1 neu loi
    if (fork() == 0)
    {
        // Code nay chi chay o CHILD process
        x = x + 2; // x cua child = 2
        printf("Child: Hello Forking!\n");

        // exit() ket thuc child process
        // Khong dung return vi co the con code phia duoi
        exit(0);
    }
    else
    {
        // Code nay chi chay o PARENT process
        x = x + 1; // x cua parent = 1
        printf("Parent: Hello Forking!\n");
        // Chu y: x cua child va parent la doc lap, khong anh huong lan nhau
    }

    fgetc(stdin);
}
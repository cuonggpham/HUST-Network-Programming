/*
 * CHUONG TRINH: Lay danh sach IP address cua may tinh
 * KIEN THUC LAP TRINH MANG:
 * - Doc thong tin cau hinh mang tu lenh ifconfig
 * - Phan tich du lieu text de trich xuat IP address
 *
 * KIEN THUC C ADVANCE:
 * - system(): Thuc thi lenh shell tu trong chuong trinh C
 * - fopen(), fgets(), fclose(): Xu ly file trong C
 * - strstr(): Tim chuoi con trong chuoi cha
 * - sscanf(): Doc du lieu tu chuoi theo dinh dang
 * - Array initialization: char line[1024] = {0} - Khoi tao mang voi gia tri 0
 */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    // Chay lenh ifconfig va luu ket qua vao file out.txt
    // system() cho phep thuc thi cac lenh he dieu hanh tu code C
    system("ifconfig > out.txt");

    // Mo file de doc (che do "rt" = read text)
    FILE *f = fopen("out.txt", "rt");

    // Doc tung dong cho den het file
    // feof() kiem tra xem da den cuoi file chua
    while (!feof(f))
    {
        // Khoi tao mang char voi tat ca phan tu = 0
        // sizeof(line) - 1 de tranh buffer overflow
        char line[1024] = {0};
        fgets(line, sizeof(line) - 1, f);

        // strstr() tim chuoi "inet " trong dong hien tai
        // Tra ve con tro toi vi tri bat dau cua chuoi tim thay, hoac NULL neu khong tim thay
        char *found = strstr(line, "inet ");
        if (found != NULL)
        {
            char inet[1024] = {0};
            char ip[1024] = {0};
            // sscanf() doc du lieu tu chuoi theo dinh dang
            // Doc 2 string: "inet" va dia chi IP
            sscanf(found, "%s%s", inet, ip);
            printf("%s\n", ip);
        }

        // Tim dia chi IPv6 tuong tu nhu IPv4
        found = strstr(line, "inet6 ");
        if (found != NULL)
        {
            char inet[1024] = {0};
            char ip[1024] = {0};
            sscanf(found, "%s%s", inet, ip);
            printf("%s\n", ip);
        }
    }
    // Dong file sau khi su dung xong de giai phong tai nguyen
    fclose(f);
}
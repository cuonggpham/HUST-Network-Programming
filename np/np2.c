/*
 * CHUONG TRINH: Noi chuoi - Doc nhieu dong va noi thanh mot chuoi lon
 *
 * KIEN THUC C ADVANCE:
 * - Con tro kep (double pointer): char** - Con tro toi con tro
 * - realloc(): Cap phat lai bo nho dong (dynamic memory reallocation)
 * - Global variable: Bien toan cuc co the truy cap tu bat ky ham nao
 * - Pass by reference: Truyen dia chi cua con tro de thay doi gia tri ben ngoai ham
 * - sprintf(): Ghi du lieu vao chuoi theo dinh dang
 * - Operator *: Dereferencing operator de truy cap gia tri ma con tro tro toi
 * - Operator &: Address-of operator de lay dia chi cua bien
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

// Bien toan cuc - Co the truy cap tu moi noi trong chuong trinh
char *output = NULL;

// Ham Append su dung con tro kep de co the thay doi dia chi cua output
// destination la con tro toi con tro (pointer to pointer)
void Append(char **destination, const char *str)
{
    // *destination la gia tri cua con tro ma destination tro toi
    char *tmp = *destination;

    // Tinh chieu dai hien tai cua chuoi
    // Neu tmp = NULL thi oldlen = 0
    int oldlen = tmp != NULL ? strlen(tmp) : 0;

    // realloc() cap phat lai bo nho voi kich thuoc moi
    // Giu nguyen du lieu cu va mo rong them cho chuoi moi
    tmp = (char *)realloc(tmp, oldlen + strlen(str) + 1);

    // Them chuoi moi vao cuoi chuoi hien tai
    // tmp + strlen(tmp) la dia chi cua ki tu '\0' (cuoi chuoi)
    sprintf(tmp + strlen(tmp), "%s", str);

    // Cap nhat lai gia tri cua con tro ben ngoai ham
    *destination = tmp;
}

int main()
{
    // Vong lap vo han (0 == 0 luon dung)
    while (0 == 0)
    {
        char tmp[1024] = {0};
        // Doc mot dong tu stdin (ban phim)
        fgets(tmp, sizeof(tmp) - 1, stdin);

        // Loai bo cac ki tu xuong dong '\r' va '\n' o cuoi chuoi
        // Windows dung \r\n, Unix dung \n
        while (tmp[strlen(tmp) - 1] == '\r' ||
               tmp[strlen(tmp) - 1] == '\n')
        {
            tmp[strlen(tmp) - 1] = 0;
        }

        // Neu dong khong rong thi them vao output
        if (strlen(tmp) > 0)
        {
            // Truyen &output (dia chi cua output) de ham co the thay doi gia tri cua output
            Append(&output, tmp);
        }
        else
            break; // Dong rong thi thoat vong lap
    }
    printf("%s\n", output);

    // Giai phong bo nho da cap phat dong
    // Quan trong de tranh memory leak
    free(output);
    output = NULL;
}
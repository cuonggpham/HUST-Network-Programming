/*
 * CHUONG TRINH: Tao trang HTML hien thi danh sach file va thu muc
 *
 * KIEN THUC C ADVANCE:
 * - scandir(): Quet thu muc va lay danh sach file/folder
 * - struct dirent: Cau truc chua thong tin file/folder
 * - Callback function: Ham Compare duoc truyen vao scandir() de sap xep
 * - memset(): Khoi tao vung nho voi gia tri mac dinh
 * - Con tro toi struct: (*e1)->d_name de truy cap thanh vien cua struct qua con tro
 * - fwrite(): Ghi du lieu nhi phan vao file
 * - HTML generation: Tao noi dung HTML bang C
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

char *output = NULL;
char *root = "/mnt/c";

void Append(char **destination, const char *str)
{
    char *tmp = *destination;

    int oldlen = tmp != NULL ? strlen(tmp) : 0;
    int more = strlen(str) + 1;
    tmp = (char *)realloc(tmp, oldlen + more);

    // memset() khoi tao vung nho moi voi gia tri 0
    // Khac voi realloc() chi cap phat ma khong khoi tao
    memset(tmp + oldlen, 0, more);
    sprintf(tmp + strlen(tmp), "%s", str);
    *destination = tmp;
}

// Ham Compare la callback function cho scandir()
// Dung de sap xep: thu muc truoc, file sau, trong moi nhom sap xep theo ten
// Tham so la con tro kep toi struct dirent
int Compare(const struct dirent **e1, const struct dirent **e2)
{
    // (*e1)->d_type: Lay field d_type tu struct ma con tro e1 tro toi
    // DT_DIR la constant dai dien cho thu muc (directory type)
    if ((*e1)->d_type == (*e2)->d_type)
    {
        // Cung loai thi sap xep theo ten alphabetically
        return strcmp((*e1)->d_name, (*e2)->d_name);
    }
    else
    {
        // Khac loai thi thu muc (DIR) len truoc
        if ((*e1)->d_type == DT_DIR)
            return -1; // e1 nho hon e2 -> e1 len truoc
        else
            return 1; // e1 lon hon e2 -> e2 len truoc
    }
}

int main()
{
    // struct dirent** result: Mang cac con tro toi struct dirent
    // Moi phan tu la thong tin cua mot file/folder
    struct dirent **result = NULL;

    Append(&output, "<html>");

    // scandir() quet thu muc va tra ve danh sach file/folder
    // Tham so 1: Duong dan thu muc can quet
    // Tham so 2: Con tro toi mang ket qua (output parameter)
    // Tham so 3: Ham filter (NULL = lay tat ca)
    // Tham so 4: Ham compare de sap xep
    // Tra ve so luong entry tim thay
    int n = scandir(root, &result, NULL, Compare);
    if (n > 0)
    {
        // Duyet qua tung entry
        for (int i = 0; i < n; i++)
        {
            // Kiem tra loai: DT_DIR (directory) hay file binh thuong
            if (result[i]->d_type == DT_DIR)
            {
                // Thu muc hien thi in dam (bold) <b>
                Append(&output, "<a href=\"");
                Append(&output, result[i]->d_name);
                Append(&output, "\">");
                Append(&output, "<b>");
                Append(&output, result[i]->d_name);
                Append(&output, "</b>");
                Append(&output, "</a>");
                Append(&output, "<br>");
            }
            else
            {
                // File hien thi in nghieng (italic) <i>
                Append(&output, "<a href=\"");
                Append(&output, result[i]->d_name);
                Append(&output, "\">");
                Append(&output, "<i>");
                Append(&output, result[i]->d_name);
                Append(&output, "</i>");
                Append(&output, "</a>");
                Append(&output, "<br>");
            }
            // Giai phong bo nho cua tung entry
            // scandir() cap phat dong phai tu giai phong
            free(result[i]);
            result[i] = NULL;
        }
    }

    Append(&output, "</html>");

    // Ghi vao file che do binary ("wb" = write binary)
    FILE *f = fopen("output.html", "wb");

    // fwrite() ghi du lieu nhi phan vao file
    // Tham so: con tro data, kich thuoc moi element, so luong element, file pointer
    fwrite(output, sizeof(char), strlen(output), f);
    fclose(f);

    // Don dep bo nho
    free(output);
    output = NULL;
}
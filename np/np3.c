/*
 * CHUONG TRINH: Tao trang HTML hien thi danh sach file va thu muc
 *
 * KIEN THUC LAP TRINH MANG:
 * - Tao noi dung HTML dong de phuc vu qua HTTP
 * - Directory listing cho file server
 */

#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *output = NULL;
char *root = "/mnt/c";

void Append(char **destination, const char *str) {
  char *tmp = *destination;
  int oldlen = tmp != NULL ? strlen(tmp) : 0;
  int more = strlen(str) + 1;
  tmp = (char *)realloc(tmp, oldlen + more);

  // memset(): Khoi tao vung nho voi gia tri cu the
  // memset(ptr, value, size): Set 'size' bytes tai 'ptr' thanh 'value'
  // Khac voi realloc() chi cap phat ma khong khoi tao
  memset(tmp + oldlen, 0, more);
  sprintf(tmp + strlen(tmp), "%s", str);
  *destination = tmp;
}

// Callback function: Ham duoc truyen vao ham khac de goi lai sau
// scandir() se goi ham Compare() de sap xep cac entry
// Phai co signature dung: int (*compar)(const struct dirent **, const struct
// dirent **)
int Compare(const struct dirent **e1, const struct dirent **e2) {
  // Arrow operator (->): Truy cap member cua struct thong qua con tro
  // (*e1)->d_type tuong duong voi (*(*e1)).d_type
  // d_type: Field cho biet loai entry (DT_DIR = directory, DT_REG = regular
  // file)
  if ((*e1)->d_type == (*e2)->d_type) {
    // strcmp(): So sanh 2 chuoi, tra ve 0 neu bang, <0 neu nho hon, >0 neu lon
    // hon
    return strcmp((*e1)->d_name, (*e2)->d_name);
  } else {
    // DT_DIR: Constant dai dien cho directory trong <dirent.h>
    if ((*e1)->d_type == DT_DIR)
      return -1; // e1 nho hon -> len truoc
    else
      return 1; // e2 nho hon -> len truoc
  }
}

int main() {
  // struct dirent**: Mang cac con tro toi struct dirent
  // Moi phan tu chua thong tin cua 1 file/folder (ten, loai, ...)
  struct dirent **result = NULL;

  Append(&output, "<html>");

  // scandir(): Quet thu muc va tra ve danh sach cac entry
  // Tham so 1: duong dan thu muc
  // Tham so 2: output - mang cac con tro struct dirent (se duoc cap phat)
  // Tham so 3: filter function (NULL = lay tat ca)
  // Tham so 4: compare function de sap xep (callback function)
  // Tra ve: so luong entry, -1 neu loi
  int n = scandir(root, &result, NULL, Compare);
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (result[i]->d_type == DT_DIR) {
        // d_name: Field chua ten file/folder trong struct dirent
        Append(&output, "<a href=\"");
        Append(&output, result[i]->d_name);
        Append(&output, "\">");
        Append(&output, "<b>");
        Append(&output, result[i]->d_name);
        Append(&output, "</b>");
        Append(&output, "</a>");
        Append(&output, "<br>");
      } else {
        Append(&output, "<a href=\"");
        Append(&output, result[i]->d_name);
        Append(&output, "\">");
        Append(&output, "<i>");
        Append(&output, result[i]->d_name);
        Append(&output, "</i>");
        Append(&output, "</a>");
        Append(&output, "<br>");
      }
      // scandir() cap phat dong cho moi entry -> phai tu free()
      free(result[i]);
      result[i] = NULL;
    }
  }

  Append(&output, "</html>");

  // fopen() voi "wb": Write Binary mode
  // Binary mode khong xu ly ky tu dac biet (\n, \r), ghi nguyen trang
  FILE *f = fopen("output.html", "wb");

  // fwrite(): Ghi du lieu nhi phan vao file
  // fwrite(data, size_of_element, num_elements, file)
  // Tra ve so luong element ghi thanh cong
  fwrite(output, sizeof(char), strlen(output), f);
  fclose(f);

  free(output);
  output = NULL;
}
#include <dirent.h>
#include <libgen.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *output = NULL;
char *root = "/home/dell";

void Append(char **destination, const char *str) {
  char *tmp = *destination;
  int oldlen = tmp != NULL ? strlen(tmp) : 0;
  int more = strlen(str) + 1;
  tmp = (char *)realloc(tmp, oldlen + more);
  memset(tmp + oldlen, 0, more);
  sprintf(tmp + strlen(tmp), "%s", str);
  *destination = tmp;
}

int Compare(const struct dirent **e1, const struct dirent **e2) {
  if ((*e1)->d_type == (*e2)->d_type) {
    return strcmp((*e1)->d_name, (*e2)->d_name);
  } else {
    if ((*e1)->d_type == DT_DIR)
      return -1;
    else
      return 1;
  }
}

int isDirectory(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
    return 1;
  return 0;
}

int main() {
  while (1) {
    struct dirent **result = NULL;
    free(output);
    output = NULL;

    Append(&output, "<html>");
    int n = scandir(root, &result, NULL, Compare);
    if (n > 0) {
      for (int i = 0; i < n; i++) {
        if (result[i]->d_type == DT_DIR) {
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
        free(result[i]);
        result[i] = NULL;
      }
      free(result);
    }

    Append(&output, "</html>");
    FILE *f = fopen("output.html", "wb");
    fwrite(output, sizeof(char), strlen(output), f);
    fclose(f);

    printf("Current folder: %s\n", root);
    printf("Enter folder name: ");
    char input[256];
    if (scanf("%255s", input) != 1)
      break;

    if (strcmp(input, "exit") == 0)
      break;

    if (strcmp(input, "..") == 0) {
      if (strcmp(root, "/") != 0) {
        char *tmp = strdup(root);
        char *parent = dirname(tmp);
        root = strdup(parent);
        free(tmp);
      }
    } else {
      char newpath[512];
      snprintf(newpath, sizeof(newpath), "%s/%s", root, input);
      if (isDirectory(newpath)) {
        root = strdup(newpath);
      } else {
        printf("'%s' is not a valid directory inside %s\n", input, root);
      }
    }
  }

  free(output);
  return 0;
}

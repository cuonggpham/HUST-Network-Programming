#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *output = NULL;

void Append(char **destination, const char *str) {
  char *tmp = *destination;

  int oldlen = tmp != NULL ? strlen(tmp) : 0;
  tmp = (char *)realloc(tmp, oldlen + strlen(str) + 1);
  sprintf(tmp + strlen(tmp), "%s", str);
  *destination = tmp;
}

int main() {
  while (0 == 0) {
    char tmp[1024] = {0};
    fgets(tmp, sizeof(tmp) - 1, stdin);
    while (tmp[strlen(tmp) - 1] == '\r' || tmp[strlen(tmp) - 1] == '\n') {
      tmp[strlen(tmp) - 1] = 0;
    }
    if (strlen(tmp) > 0) {
      Append(&output, tmp);
    } else
      break;
  }
  printf("%s\n", output);

  free(output);
  output = NULL;
}
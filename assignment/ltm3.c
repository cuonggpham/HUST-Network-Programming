#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char *command = "ifconfig > out.txt";
    system(command);
    FILE *f = fopen("out.txt", "r");
    while (!feof(f))
    {
        char line[1024] = {0};
        char inet[1024] = {0};
        char ip[1024] = {0};
        fgets(line, sizeof(line) - 1, f);
        char *found = strstr(line, "inet");
        if (found != NULL)
        {
            sscanf(found, "%s %s", inet, ip);
            printf("IP Address: %s\n", ip);
        }
    }
    fclose(f);
}
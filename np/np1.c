#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    system("ifconfig > out.txt");
    FILE* f = fopen("out.txt","rt");
    while (!feof(f))
    {
        char line[1024] = { 0 };
        fgets(line, sizeof(line) - 1, f);
        char* found = strstr(line,"inet ");
        if (found != NULL)
        {
            char inet[1024] = { 0 };
            char ip[1024] = { 0 };
            sscanf(found, "%s%s", inet, ip);
            printf("ipv4: %s\n", ip);
        }

        found = strstr(line,"inet6 ");
        if (found != NULL)
        {
            char inet[1024] = { 0 };
            char ip[1024] = { 0 };
            sscanf(found, "%s%s", inet, ip);
            printf("ipv6: %s\n", ip);
        }
    }
    fclose(f);
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

char output[1000000] = { 0 };

void Append(char* destination, const char* str){
    sprintf(destination + strlen(destination), "%s", str);
}

int main()
{
    while (0 == 0)
    {
        char tmp[1024] = { 0 };
        fgets(tmp, sizeof(tmp) - 1, stdin);
        while (tmp[strlen(tmp) - 1] == '\n' || 
                tmp[strlen(tmp) - 1] == '\r')
        {
            tmp[strlen(tmp) - 1] = 0;
        }
        if (strlen(tmp) > 0)
        {
            Append(output, tmp);
        }
        else
        {
            break;
        } 
    }

    printf("Output: %s\n", output);
}
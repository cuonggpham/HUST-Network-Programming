#include <stdio.h>

int main()
{
    float x;
    scanf("%f", &x);
    unsigned char *p = (unsigned char *)&x;

    for (int i = 3; i >= 0; i--)
    {
        printf("%02X ", p[i]);
    }
    printf("\n");
}

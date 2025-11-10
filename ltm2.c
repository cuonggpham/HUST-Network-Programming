#include <stdio.h>
#include <stdlib.h>

int main()
{
    float *arr = NULL;
    int count = 0;
    float num, sum = 0;

    while (1)
    {
        if (scanf("%f", &num) != 1)
            break;
        if (num == 0)
            break;

        float *tmp = realloc(arr, (count + 1) * sizeof(float));
        arr = tmp;
        arr[count++] = num;
    }

    for (int i = 0; i < count; ++i)
    {
        sum += arr[i];
    }

    printf("sum: %.2f\n", sum);
    free(arr);
}
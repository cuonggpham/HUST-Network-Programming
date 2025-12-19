/*
 * CHUONG TRINH: Multi-threading voi Pthread - Tinh tong song song
 *
 * KIEN THUC LAP TRINH MANG:
 * - Thread co the dung trong server de xu ly nhieu client dong thoi
 * - Thread nhe hon process (shared memory)
 *
 * KIEN THUC C ADVANCE:
 * - pthread_create(): Tao thread moi
 * - pthread_join(): Cho doi thread ket thuc
 * - pthread_mutex: Mutex de dong bo hoa truy cap bien chung
 * - pthread_mutex_lock/unlock: Khoa va mo khoa mutex
 * - Race condition: Nhieu thread truy cap cung du lieu dong thoi
 * - Critical section: Doan code can duoc bao ve boi mutex
 * - calloc(): Cap phat va khoi tao bo nho bang 0
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>

int K = 1000;                   // Tinh tong tu 1 den K
int N = 10;                     // So luong thread
int Sum = 0;                    // Bien toan cuc chua ket qua (shared variable)
pthread_mutex_t *pMutex = NULL; // Mutex de bao ve bien Sum

// Ham chay trong moi thread
// void* la kieu du lieu generic, co the tro toi bat ky loai nao
void *ThreadFunction(void *arg)
{
    // Chuyen void* ve int* de lay gia tri
    // *(int*)arg: Type casting roi dereference
    int i = *(int *)arg;

    // Giai phong bo nho da cap phat cho tham so
    free(arg);
    arg = NULL;

    // pthread_self() tra ve ID cua thread hien tai
    pthread_t tid = pthread_self();
    printf("ThreadID: %d, Param: %d\n", (int)tid, i);
    usleep(1000000); // Sleep 1 giay (1000000 microseconds)

    // Moi thread tinh tong mot phan cua day so
    // Thread i tinh tu (i*K/N + 1) den ((i+1)*K/N)
    for (int j = i * K / N + 1; j <= (i + 1) * K / N; j++)
    {
        // CRITICAL SECTION - Truy cap bien shared (Sum)
        pthread_mutex_lock(pMutex);   // Khoa mutex truoc khi sua Sum
        Sum += j;                     // Chi 1 thread duoc chay code nay tai 1 thoi diem
        pthread_mutex_unlock(pMutex); // Mo khoa sau khi xong
        // Neu khong co mutex, race condition se xay ra va ket qua sai
    }

    return NULL; // Thread ket thuc
}

int main()
{
    // Cap phat bo nho cho mutex
    pMutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));

    // Khoi tao mutex voi thiet lap mac dinh (NULL)
    pthread_mutex_init(pMutex, NULL);

    // Cap phat mang luu thread IDs
    pthread_t *tids = (pthread_t *)calloc(N, sizeof(pthread_t));

    // Tao N threads
    for (int i = 0; i < N; i++)
    {
        // Cap phat bo nho cho tham so truyen vao thread
        // Phai cap phat dong vi moi thread can gia tri rieng
        int *arg = (int *)calloc(1, sizeof(int));
        *arg = i;

        // pthread_create() tao thread moi
        // Tham so: &thread_id, attributes(NULL=default), ham chay, tham so
        pthread_create(&tids[i], NULL, ThreadFunction, arg);
    }

    // Cho doi tat ca thread ket thuc
    for (int i = 0; i < N; i++)
    {
        // pthread_join() block cho den khi thread ket thuc
        // Tuong tu wait() cho process
        pthread_join(tids[i], NULL);
    }

    // Don dep bo nho
    free(tids);
    tids = NULL;

    // Huy mutex truoc khi giai phong
    pthread_mutex_destroy(pMutex);
    free(pMutex);
    pMutex = NULL;

    printf("Sum: %d\n", Sum);
}
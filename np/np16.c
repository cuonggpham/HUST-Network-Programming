/*
 * CHUONG TRINH: Multi-threading voi Pthread - Tinh tong song song
 *
 * KIEN THUC LAP TRINH MANG:
 * - Thread co the dung trong server de xu ly nhieu client dong thoi
 * - Thread nhe hon process (shared memory) -> nhanh hon fork
 * - Thread cho phep chia se du lieu de dang hon process
 */

#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int K = 1000; // Tinh tong tu 1 den K
int N = 10;   // So luong thread

// Shared variable: Tat ca thread deu truy cap duoc (khac voi process)
// NGUY HIEM: Can mutex de tranh race condition
int Sum = 0;

// pthread_mutex_t: Kieu du lieu mutex (MUTual EXclusion)
// Dung de bao ve critical section - vung code chi 1 thread duoc chay
pthread_mutex_t *pMutex = NULL;

// Ham chay trong moi thread
// PHAI co signature: void* function(void* arg)
// void* la kieu generic, co the tro toi bat ky kieu nao
void *ThreadFunction(void *arg) {
  // Type casting void* ve int* roi dereference de lay gia tri
  // (int*)arg: Cast void* thanh int*
  // *(int*)arg: Lay gia tri int tai dia chi do
  int i = *(int *)arg;

  // free() argument vi da cap phat dong trong main
  // Moi thread co trach nhiem giai phong memory cua minh
  free(arg);
  arg = NULL;

  // pthread_self(): Tra ve Thread ID cua thread hien tai
  // Kieu pthread_t (thuong la unsigned long)
  pthread_t tid = pthread_self();
  printf("ThreadID: %d, Param: %d\n", (int)tid, i);

  // usleep(): Sleep trong microseconds (1 giay = 1,000,000 us)
  // sleep(): Sleep trong seconds
  usleep(1000000); // 1 giay

  // Moi thread tinh tong mot phan cua day so
  // Chia de tri (divide and conquer): N threads tinh N phan roi cong lai
  for (int j = i * K / N + 1; j <= (i + 1) * K / N; j++) {
    // CRITICAL SECTION: Vung code truy cap shared resource (Sum)
    // Chi 1 thread duoc chay code nay tai 1 thoi diem

    // pthread_mutex_lock(): Block cho den khi lay duoc mutex
    // Neu thread khac dang giu mutex -> cho doi
    pthread_mutex_lock(pMutex);

    // RACE CONDITION neu khong co mutex:
    // Thread A doc Sum = 10
    // Thread B doc Sum = 10
    // Thread A ghi Sum = 15
    // Thread B ghi Sum = 13 -> MAT du lieu cua A!
    Sum += j;

    // pthread_mutex_unlock(): Nha mutex cho thread khac
    // QUAN TRONG: Phai unlock sau khi xong, neu khong -> DEADLOCK
    pthread_mutex_unlock(pMutex);
  }

  // Thread ket thuc, return NULL (khong co gia tri tra ve)
  return NULL;
}

int main() {
  // calloc(): Cap phat va khoi tao = 0
  // calloc(count, size) vs malloc(count * size)
  // calloc an toan hon vi khoi tao memory
  pMutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));

  // pthread_mutex_init(): Khoi tao mutex truoc khi su dung
  // Tham so 2 = NULL: Su dung thiet lap mac dinh
  pthread_mutex_init(pMutex, NULL);

  // Mang luu Thread IDs de co the join sau
  pthread_t *tids = (pthread_t *)calloc(N, sizeof(pthread_t));

  // Tao N threads
  for (int i = 0; i < N; i++) {
    // Cap phat dong cho argument vi moi thread can gia tri RIENG
    // Neu dung &i (local variable), tat ca thread thay cung gia tri (bug!)
    int *arg = (int *)calloc(1, sizeof(int));
    *arg = i;

    // pthread_create(): Tao thread moi
    // Tham so 1: output - Thread ID
    // Tham so 2: attributes (NULL = default)
    // Tham so 3: ham chay trong thread (function pointer)
    // Tham so 4: argument truyen cho ham
    // Thread bat dau chay NGAY LAP TUC sau khi tao
    pthread_create(&tids[i], NULL, ThreadFunction, arg);
  }

  // Cho doi tat ca thread ket thuc
  for (int i = 0; i < N; i++) {
    // pthread_join(): Block cho den khi thread chi dinh ket thuc
    // Tuong tu wait()/waitpid() cho process
    // Tham so 2: Nhan gia tri return cua thread (NULL = khong can)
    pthread_join(tids[i], NULL);
  }

  free(tids);
  tids = NULL;

  // pthread_mutex_destroy(): Giai phong tai nguyen mutex
  // PHAI goi truoc khi free() mutex
  pthread_mutex_destroy(pMutex);
  free(pMutex);
  pMutex = NULL;

  printf("Sum: %d\n", Sum);
}
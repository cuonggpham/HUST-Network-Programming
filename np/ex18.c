/*
 * CHUONG TRINH: Travelling Salesman Problem (TSP) - Bai toan nguoi ban hang
 *
 * KIEN THUC THUAT TOAN:
 * - TSP: Tim duong di ngan nhat di qua tat ca cac thanh pho va quay ve diem
 * xuat phat
 * - Backtracking: Thu tat ca cac hoan vi, quay lui khi khong kha thi
 * - Branch and Bound: Cat nhanh (pruning) khi biet ket qua khong the tot hon
 * - Do phuc tap: O(n!) - Rat lon, can toi uu (pruning) de giam thoi gian
 */

#include <limits.h>
#include <stdio.h>

// Global variables: Tat ca ham de quy deu truy cap duoc
// Dung global vi de quy can chia se trang thai

int m[20] = {0}; // Mang danh dau thanh pho da tham (0 = chua, 1 = da tham)
int d[20][20] = {
    0}; // Ma tran khoang cach giua cac thanh pho (d[i][j] = khoang cach i -> j)
int x[20] = {
    0};    // Mang luu duong di hien tai (x[i] = thanh pho thu i trong tour)
int n = 0; // So luong thanh pho

// INT_MAX: Hang so lon nhat cua kieu int (tu <limits.h>)
// Dung de khoi tao gia tri "vo cung" khi tim min
int minDistance = INT_MAX; // Khoang cach ngan nhat tim duoc (ket qua)

// D_MIN: Khoang cach nho nhat giua 2 thanh pho bat ky
// Dung cho pruning: Uoc luong lower bound cua cac buoc con lai
int D_MIN = INT_MAX;

// Ham kiem tra thanh pho v co the tham duoc khong
// Tra ve 1 neu chua tham (m[v] == 0), 0 neu da tham
int check(int v) { return m[v] == 0; }

// Ham de quy thu tat ca cac duong di (backtracking)
// i: Vi tri hien tai trong duong di (dang chon thanh pho thu i)
// distance: Tong khoang cach da di duoc tu thanh pho 0 den x[i-1]
void try(int i, int distance) {
  // Thu tat ca cac thanh pho chua tham cho vi tri i
  for (int v = 0; v < n; v++) {
    if (check(v)) // Chi xet thanh pho chua tham
    {
      x[i] = v; // Chon thanh pho v cho vi tri i trong tour
      m[v] = 1; // Danh dau da tham (de cac buoc sau khong chon lai)

      if (i == n - 1) // Da chon xong n thanh pho (tour day du)
      {
        // Tinh tong khoang cach:
        // distance: Khoang cach tu x[0] den x[i-1]
        // d[x[i-1]][x[i]]: Khoang cach tu thanh pho truoc den thanh pho cuoi
        // d[x[i]][x[0]]: Khoang cach quay ve thanh pho xuat phat (tour khep
        // kin)
        int totalDistance = distance + d[x[i - 1]][x[i]] + d[x[i]][x[0]];
        if (totalDistance < minDistance) {
          minDistance = totalDistance; // Cap nhat ket qua tot nhat
        }
      } else {
        // PRUNING (cat nhanh): Kiem tra truoc khi de quy
        // Con (n-i) thanh pho can tham + 1 duong ve
        // Lower bound = so buoc con lai * khoang cach nho nhat
        // Neu lower bound + distance hien tai >= minDistance -> khong can thu
        //
        // Cong thuc: D_MIN * (n - i) + distance + d[x[i-1]][x[i]] < minDistance
        // - D_MIN * (n - i): Uoc luong thap nhat cho cac buoc con lai
        // - distance: Khoang cach da di
        // - d[x[i-1]][x[i]]: Khoang cach den thanh pho vua chon
        if (D_MIN * (n - i) + distance + d[x[i - 1]][x[i]] < minDistance) {
          // De quy: Thu tiep thanh pho thu i+1
          // Cap nhat distance = distance + khoang cach den thanh pho vua chon
          try(i + 1, distance + d[x[i - 1]][x[i]]);
        }
        // Neu khong thoa dieu kien pruning -> bo qua (khong de quy)
      }

      // BACKTRACK: Quay lui de thu phuong an khac
      // Bo danh dau thanh pho v de co the chon o vi tri khac
      m[v] = 0;
      x[i] = 0;
    }
  }
}

int main() {
  // scanf(): Doc input tu stdin theo format
  // &n: Lay dia chi cua n de scanf ghi gia tri vao
  scanf("%d", &n);

  // Doc ma tran khoang cach n x n
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      scanf("%d", &d[i][j]);

      // Tim khoang cach nho nhat de dung cho pruning
      // Chi xet i != j (khong tinh khoang cach tu thanh pho den chinh no)
      if (i != j && d[i][j] < D_MIN) {
        D_MIN = d[i][j];
      }
    }
  }

  // Bat dau tu thanh pho 0 (co dinh diem xuat phat)
  // Voi TSP, vi tour khep kin nen diem bat dau khong quan trong
  x[0] = 0;
  m[0] = 1; // Danh dau thanh pho 0 da tham

  // Bat dau de quy tu vi tri 1 (vi tri 0 da la thanh pho 0)
  // Khoang cach ban dau = 0 (chua di buoc nao)
  try(1, 0);

  // In ket qua: Khoang cach ngan nhat tim duoc
  printf("%d", minDistance);
}
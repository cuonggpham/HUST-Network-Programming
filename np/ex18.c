/*
 * CHUONG TRINH: Travelling Salesman Problem (TSP) - Bai toan nguoi ban hang
 *
 * KIEN THUC THUAT TOAN:
 * - TSP: Tim duong di ngan nhat di qua tat ca cac thanh pho va quay ve
 * - Backtracking: Thu tat ca cac hoan vi va chon ket qua tot nhat
 * - Branch and Bound: Cat nhanh khi biet ket qua khong the tot hon
 * - Pruning: Loai bo cac nhanh khong kha thi de tang toc
 *
 * KIEN THUC C ADVANCE:
 * - Recursion: De quy de thu tat ca cac hoan vi
 * - Global variables: Luu trang thai chung cho de quy
 * - INT_MAX: Gia tri lon nhat cua int tu limits.h
 * - Optimization: Dung D_MIN de cat nhanh som
 */

#include <stdio.h>
#include <limits.h>

int m[20] = {0};           // Mang danh dau thanh pho da tham
int d[20][20] = {0};       // Ma tran khoang cach giua cac thanh pho
int x[20] = {0};           // Mang luu duong di hien tai
int n = 0;                 // So luong thanh pho
int minDistance = INT_MAX; // Khoang cach ngan nhat tim duoc
int D_MIN = INT_MAX;       // Khoang cach nho nhat giua 2 thanh pho bat ky

// Kiem tra thanh pho v co the tham chua
int check(int v)
{
    return m[v] == 0; // Chua tham neu m[v] = 0
}

// Ham de quy thu tat ca cac duong di
// i: Vi tri hien tai trong duong di
// distance: Tong khoang cach da di
void try(int i, int distance)
{
    // Thu tat ca cac thanh pho chua tham
    for (int v = 0; v < n; v++)
    {
        if (check(v))
        {
            x[i] = v; // Chon thanh pho v cho vi tri i
            m[v] = 1; // Danh dau da tham

            if (i == n - 1) // Da di het tat ca thanh pho
            {
                // Tinh tong khoang cach bao gom quay ve thanh pho dau
                if (distance + d[x[i - 1]][x[i]] + d[x[i]][x[0]] < minDistance)
                {
                    minDistance = distance + d[x[i - 1]][x[i]] + d[x[i]][x[0]];
                }
            }
            else
            {
                // PRUNING: Cat nhanh neu ket qua khong the tot hon
                // Con (n-i) thanh pho -> tot nhat la (n-i) * D_MIN
                // Neu tong da lon hon minDistance thi khong can thu tiep
                if (D_MIN * (n - i) + distance + d[x[i - 1]][x[i]] < minDistance)
                {
                    // De quy thu thanh pho tiep theo
                    try(i + 1, distance + d[x[i - 1]][x[i]]);
                }
            }

            // Backtrack: Bo danh dau de thu phuong an khac
            m[v] = 0;
            x[i] = 0;
        }
    }
}

int main()
{
    // Doc so luong thanh pho
    scanf("%d", &n);

    // Doc ma tran khoang cach
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            scanf("%d", &d[i][j]);

            // Tim khoang cach nho nhat de dung cho pruning
            if (i != j && d[i][j] < D_MIN)
            {
                D_MIN = d[i][j];
            }
        }
    }

    // Bat dau tu thanh pho 0
    x[0] = 0;
    m[0] = 1; // Danh dau da tham thanh pho 0

    // Bat dau de quy tu vi tri 1 voi khoang cach ban dau = 0
    try(1, 0);

    // In ket qua
    printf("%d", minDistance);
}
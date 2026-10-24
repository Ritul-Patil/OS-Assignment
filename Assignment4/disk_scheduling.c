/*
 * Disk Scheduling Algorithms: FCFS, SSTF, SCAN, C-SCAN
 *
 * All four algorithms run on the SAME request queue, initial head position
 * and disk size. For each one the seek sequence and total head movement are
 * printed, then the algorithms are compared.
 *
 * Assumptions:
 *   - Cylinders are numbered 0 .. disk_size-1.
 *   - SCAN goes all the way to the end of the disk before reversing.
 *   - C-SCAN goes to the end, jumps to the other end, and continues in the
 *     same direction. The return jump IS counted as head movement.
 *
 * Compile: gcc -Wall -Wextra -o diskscheduling disk_scheduling.c
 * Run:     ./diskscheduling
 */

#include <stdio.h>
#include <stdlib.h>

#define MAX_REQ 50
#define MAX_SEQ (MAX_REQ + 4)

typedef struct {
    const char *name;
    int total;
} Result;

static int cmp_int(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

/* Sum of |differences| along head -> seq[0] -> seq[1] -> ... */
static int total_movement(int head, const int seq[], int len) {
    int total = 0, pos = head;
    for (int i = 0; i < len; i++) {
        total += abs(seq[i] - pos);
        pos = seq[i];
    }
    return total;
}

static Result report(const char *name, int head, const int seq[], int len) {
    Result r = {name, total_movement(head, seq, len)};

    printf("\n=== %s ===\n", name);
    printf("Seek sequence: %d", head);
    for (int i = 0; i < len; i++)
        printf(" -> %d", seq[i]);
    printf("\n");

    printf("Head movement: ");
    int pos = head;
    for (int i = 0; i < len; i++) {
        printf("%s%d", i ? " + " : "", abs(seq[i] - pos));
        pos = seq[i];
    }
    printf(" = %d\n", r.total);
    return r;
}

static Result fcfs(int head, const int req[], int n) {
    return report("FCFS", head, req, n);
}

static Result sstf(int head, const int req[], int n) {
    int seq[MAX_SEQ], done[MAX_REQ] = {0};
    int pos = head;

    for (int k = 0; k < n; k++) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (done[i])
                continue;
            if (best == -1 || abs(req[i] - pos) < abs(req[best] - pos))
                best = i;
        }
        done[best] = 1;
        seq[k] = req[best];
        pos = req[best];
    }
    return report("SSTF", head, seq, n);
}

/* dir: 1 = towards higher cylinders first, 0 = towards lower first */
static Result scan(int head, const int req[], int n, int disk_size, int dir) {
    int sorted[MAX_REQ], seq[MAX_SEQ], len = 0;
    for (int i = 0; i < n; i++)
        sorted[i] = req[i];
    qsort(sorted, n, sizeof(int), cmp_int);

    if (dir) {
        for (int i = 0; i < n; i++)
            if (sorted[i] >= head)
                seq[len++] = sorted[i];
        seq[len++] = disk_size - 1;                 /* go to the end */
        for (int i = n - 1; i >= 0; i--)
            if (sorted[i] < head)
                seq[len++] = sorted[i];
    } else {
        for (int i = n - 1; i >= 0; i--)
            if (sorted[i] <= head)
                seq[len++] = sorted[i];
        seq[len++] = 0;                             /* go to the start */
        for (int i = 0; i < n; i++)
            if (sorted[i] > head)
                seq[len++] = sorted[i];
    }
    return report("SCAN", head, seq, len);
}

static Result cscan(int head, const int req[], int n, int disk_size, int dir) {
    int sorted[MAX_REQ], seq[MAX_SEQ], len = 0;
    for (int i = 0; i < n; i++)
        sorted[i] = req[i];
    qsort(sorted, n, sizeof(int), cmp_int);

    if (dir) {
        for (int i = 0; i < n; i++)
            if (sorted[i] >= head)
                seq[len++] = sorted[i];
        seq[len++] = disk_size - 1;                 /* go to the end */
        seq[len++] = 0;                             /* jump to the start */
        for (int i = 0; i < n; i++)
            if (sorted[i] < head)
                seq[len++] = sorted[i];
    } else {
        for (int i = n - 1; i >= 0; i--)
            if (sorted[i] <= head)
                seq[len++] = sorted[i];
        seq[len++] = 0;                             /* go to the start */
        seq[len++] = disk_size - 1;                 /* jump to the end */
        for (int i = n - 1; i >= 0; i--)
            if (sorted[i] > head)
                seq[len++] = sorted[i];
    }
    return report("C-SCAN", head, seq, len);
}

static void print_comparison(const Result res[], int n) {
    printf("\n\n================ COMPARISON ================\n");
    printf("%-10s %-22s\n", "Algorithm", "Total Head Movement");
    printf("------------------------------------\n");
    for (int i = 0; i < n; i++)
        printf("%-10s %-22d\n", res[i].name, res[i].total);

    int best = 0;
    for (int i = 1; i < n; i++)
        if (res[i].total < res[best].total)
            best = i;
    printf("\nMost efficient for this input: %s (%d cylinders)\n",
           res[best].name, res[best].total);
}

int main(void) {
    int n, head, disk_size, dir;
    int req[MAX_REQ];

    printf("Enter number of requests (max %d): ", MAX_REQ);
    if (scanf("%d", &n) != 1 || n < 1 || n > MAX_REQ) {
        printf("Invalid number of requests.\n");
        return 1;
    }
    printf("Enter disk size (number of cylinders): ");
    if (scanf("%d", &disk_size) != 1 || disk_size < 2) {
        printf("Invalid disk size.\n");
        return 1;
    }
    printf("Enter request queue: ");
    for (int i = 0; i < n; i++) {
        if (scanf("%d", &req[i]) != 1 || req[i] < 0 || req[i] >= disk_size) {
            printf("Invalid request (must be 0 to %d).\n", disk_size - 1);
            return 1;
        }
    }
    printf("Enter initial head position: ");
    if (scanf("%d", &head) != 1 || head < 0 || head >= disk_size) {
        printf("Invalid head position.\n");
        return 1;
    }
    printf("Enter direction for SCAN/C-SCAN (1 = towards higher, 0 = towards lower): ");
    if (scanf("%d", &dir) != 1 || (dir != 0 && dir != 1)) {
        printf("Invalid direction.\n");
        return 1;
    }

    Result results[4];
    results[0] = fcfs(head, req, n);
    results[1] = sstf(head, req, n);
    results[2] = scan(head, req, n, disk_size, dir);
    results[3] = cscan(head, req, n, disk_size, dir);

    print_comparison(results, 4);
    return 0;
}
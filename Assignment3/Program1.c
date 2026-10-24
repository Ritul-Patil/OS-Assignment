/*
 * Memory Allocation Strategies: First Fit, Best Fit, Worst Fit
 *
 * All three strategies run on the SAME set of memory blocks and process
 * requests. When a process is placed in a block larger than needed, the
 * leftover part stays in the block as free memory (block size shrinks).
 *
 * Compile: gcc -Wall -Wextra -o memalloc memory_allocation.c
 * Run:     ./memalloc
 */

#include <stdio.h>
#include <string.h>

#define MAX 50

typedef enum { FIRST_FIT, BEST_FIT, WORST_FIT } Strategy;

typedef struct {
    const char *name;
    int allocated_count;
    int total_processes;
    int allocated_memory;
    int free_memory;
    int total_memory;
    double utilization;
} Result;

/* Find the block index for a process of given size, or -1 if none fits. */
static int find_block(const int blocks[], int nb, int size, Strategy s) {
    int idx = -1;
    for (int i = 0; i < nb; i++) {
        if (blocks[i] < size)
            continue;
        if (s == FIRST_FIT)
            return i;                                   /* first that fits */
        if (idx == -1 ||
            (s == BEST_FIT  && blocks[i] < blocks[idx]) ||  /* smallest fit */
            (s == WORST_FIT && blocks[i] > blocks[idx]))    /* largest fit  */
            idx = i;
    }
    return idx;
}

static Result run_strategy(const char *name, Strategy s,
                           const int orig_blocks[], int nb,
                           const int procs[], int np) {
    int blocks[MAX];
    int alloc[MAX];
    memcpy(blocks, orig_blocks, nb * sizeof(int));

    Result r = {name, 0, np, 0, 0, 0, 0.0};
    for (int i = 0; i < nb; i++)
        r.total_memory += orig_blocks[i];

    for (int p = 0; p < np; p++) {
        int b = find_block(blocks, nb, procs[p], s);
        alloc[p] = b;
        if (b != -1) {
            blocks[b] -= procs[p];
            r.allocated_count++;
            r.allocated_memory += procs[p];
        }
    }

    r.free_memory = r.total_memory - r.allocated_memory;
    r.utilization = 100.0 * r.allocated_memory / r.total_memory;

    printf("\n=== %s ===\n", name);
    printf("%-10s %-14s %-10s %-10s\n", "Process", "Size (KB)", "Block No.", "Status");
    printf("--------------------------------------------------\n");
    for (int p = 0; p < np; p++) {
        if (alloc[p] != -1)
            printf("P%-9d %-14d %-10d %-10s\n", p + 1, procs[p], alloc[p] + 1, "Allocated");
        else
            printf("P%-9d %-14d %-10s %-10s\n", p + 1, procs[p], "-", "Not Alloc");
    }

    printf("\nBlock state after allocation:\n");
    printf("%-10s %-14s %-14s\n", "Block", "Original (KB)", "Remaining (KB)");
    for (int i = 0; i < nb; i++)
        printf("%-10d %-14d %-14d\n", i + 1, orig_blocks[i], blocks[i]);

    return r;
}

static void print_comparison(const Result res[], int n) {
    printf("\n\n================ COMPARISON ================\n");
    printf("%-12s %-12s %-16s %-14s %-14s\n",
           "Strategy", "Allocated", "Allocated (KB)", "Free (KB)", "Utilization");
    printf("----------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("%-12s %d/%-10d %-16d %-14d %.2f%%\n",
               res[i].name, res[i].allocated_count, res[i].total_processes,
               res[i].allocated_memory, res[i].free_memory, res[i].utilization);
    }

    int best = 0;
    for (int i = 1; i < n; i++) {
        if (res[i].allocated_count > res[best].allocated_count ||
            (res[i].allocated_count == res[best].allocated_count &&
             res[i].utilization > res[best].utilization))
            best = i;
    }
    printf("\nBest performer for this input: %s\n", res[best].name);
}

int main(void) {
    int nb, np;
    int blocks[MAX], procs[MAX];

    printf("Enter number of memory blocks (max %d): ", MAX);
    if (scanf("%d", &nb) != 1 || nb < 1 || nb > MAX) {
        printf("Invalid number of blocks.\n");
        return 1;
    }
    printf("Enter block sizes (KB): ");
    for (int i = 0; i < nb; i++) {
        if (scanf("%d", &blocks[i]) != 1 || blocks[i] <= 0) {
            printf("Invalid block size.\n");
            return 1;
        }
    }

    printf("Enter number of processes (max %d): ", MAX);
    if (scanf("%d", &np) != 1 || np < 1 || np > MAX) {
        printf("Invalid number of processes.\n");
        return 1;
    }
    printf("Enter process sizes (KB): ");
    for (int i = 0; i < np; i++) {
        if (scanf("%d", &procs[i]) != 1 || procs[i] <= 0) {
            printf("Invalid process size.\n");
            return 1;
        }
    }

    Result results[3];
    results[0] = run_strategy("First Fit", FIRST_FIT, blocks, nb, procs, np);
    results[1] = run_strategy("Best Fit",  BEST_FIT,  blocks, nb, procs, np);
    results[2] = run_strategy("Worst Fit", WORST_FIT, blocks, nb, procs, np);

    print_comparison(results, 3);
    return 0;
}
/*
 * Page Replacement Algorithms: FIFO, LRU, Optimal
 *
 * All three algorithms run on the SAME page reference string and the SAME
 * number of frames. After every reference the frame contents are printed,
 * and at the end the page faults / hits are compared.
 *
 * Compile: gcc -Wall -Wextra -o pagerep page_replacement.c
 * Run:     ./pagerep
 */

#include <stdio.h>

#define MAX_REFS   100
#define MAX_FRAMES 10

typedef enum { FIFO, LRU, OPTIMAL } Algo;

typedef struct {
    const char *name;
    int faults;
    int hits;
    int total;
} Result;

/* Return index of page in frames, or -1 if not present. */
static int find_page(const int frames[], int nf, int page) {
    for (int i = 0; i < nf; i++)
        if (frames[i] == page)
            return i;
    return -1;
}

/* Choose a victim frame for OPTIMAL: the page used farthest in the future
 * (or never used again). */
static int optimal_victim(const int frames[], int nf,
                          const int refs[], int n, int current) {
    int victim = 0, farthest = -1;
    for (int i = 0; i < nf; i++) {
        int next = n;                       /* n means "never used again" */
        for (int j = current + 1; j < n; j++) {
            if (refs[j] == frames[i]) {
                next = j;
                break;
            }
        }
        if (next > farthest) {
            farthest = next;
            victim = i;
        }
    }
    return victim;
}

static Result simulate(const char *name, Algo algo,
                       const int refs[], int n, int nf) {
    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES];              /* for LRU */
    int fifo_next = 0;                      /* for FIFO */
    int used = 0;                           /* frames filled so far */
    Result r = {name, 0, 0, n};

    for (int i = 0; i < nf; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    printf("\n=== %s ===\n", name);
    printf("%-6s |", "Page");
    for (int f = 0; f < nf; f++)
        printf(" F%-2d", f + 1);
    printf(" | Result\n");
    printf("-------+");
    for (int f = 0; f < nf; f++)
        printf("----");
    printf("-+-------\n");

    for (int i = 0; i < n; i++) {
        int page = refs[i];
        int idx = find_page(frames, nf, page);
        int hit = (idx != -1);

        if (hit) {
            r.hits++;
            last_used[idx] = i;
        } else {
            r.faults++;
            int slot;
            if (used < nf) {
                slot = used++;              /* free frame available */
                if (algo == FIFO)
                    fifo_next = used % nf;
            } else if (algo == FIFO) {
                slot = fifo_next;
                fifo_next = (fifo_next + 1) % nf;
            } else if (algo == LRU) {
                slot = 0;
                for (int f = 1; f < nf; f++)
                    if (last_used[f] < last_used[slot])
                        slot = f;
            } else {
                slot = optimal_victim(frames, nf, refs, n, i);
            }
            frames[slot] = page;
            last_used[slot] = i;
        }

        printf("%-6d |", page);
        for (int f = 0; f < nf; f++) {
            if (frames[f] == -1)
                printf(" -  ");
            else
                printf(" %-3d", frames[f]);
        }
        printf(" | %s\n", hit ? "Hit" : "Fault");
    }

    printf("\n%s: Page Faults = %d, Page Hits = %d\n", name, r.faults, r.hits);
    return r;
}

static void print_comparison(const Result res[], int n) {
    printf("\n\n================ COMPARISON ================\n");
    printf("%-10s %-14s %-12s %-12s %-10s\n",
           "Algorithm", "Page Faults", "Page Hits", "Hit Ratio", "Fault Ratio");
    printf("--------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        char hit_ratio[16];
        snprintf(hit_ratio, sizeof hit_ratio, "%.2f%%",
                 100.0 * res[i].hits / res[i].total);
        printf("%-10s %-14d %-12d %-12s %.2f%%\n",
               res[i].name, res[i].faults, res[i].hits, hit_ratio,
               100.0 * res[i].faults / res[i].total);
    }

    int best = 0;
    for (int i = 1; i < n; i++)
        if (res[i].faults < res[best].faults)
            best = i;
    printf("\nFewest page faults for this input: %s\n", res[best].name);
}

int main(void) {
    int n, nf;
    int refs[MAX_REFS];

    printf("Enter length of reference string (max %d): ", MAX_REFS);
    if (scanf("%d", &n) != 1 || n < 1 || n > MAX_REFS) {
        printf("Invalid length.\n");
        return 1;
    }
    printf("Enter the reference string (space separated): ");
    for (int i = 0; i < n; i++) {
        if (scanf("%d", &refs[i]) != 1 || refs[i] < 0) {
            printf("Invalid page number.\n");
            return 1;
        }
    }
    printf("Enter number of frames (max %d): ", MAX_FRAMES);
    if (scanf("%d", &nf) != 1 || nf < 1 || nf > MAX_FRAMES) {
        printf("Invalid number of frames.\n");
        return 1;
    }

    Result results[3];
    results[0] = simulate("FIFO",    FIFO,    refs, n, nf);
    results[1] = simulate("LRU",     LRU,     refs, n, nf);
    results[2] = simulate("Optimal", OPTIMAL, refs, n, nf);

    print_comparison(results, 3);
    return 0;
}
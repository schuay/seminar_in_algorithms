#include <atomic>
#include <cstdio>
#include <limits>
#include <random>

#include "heap.h"
#include "linden.h"
#include "noble.h"

#undef max /* Clash between macro and limits. */
#undef min

#define DEFAULT_SECS     (10)
#define DEFAULT_NTHREADS (1)
#define DEFAULT_OFFSET   (32)
#define DEFAULT_SIZE     (1 << 15)

static std::atomic<bool> loop;
static std::atomic<int> wait_barrier;

static Heap pq_heap(DEFAULT_SIZE << 2);

static void
usage(FILE *out,
      const char *argv0)
{
    fprintf(out, "Usage: %s [OPTION]...\n"
        "\n"
        "Options:\n", argv0);

    fprintf(out, "\t-h\t\tDisplay usage.\n");
    fprintf(out, "\t-t SECS\t\tRun for SECS seconds. "
        "Default: %i\n",
        DEFAULT_SECS);
    fprintf(out, "\t-o OFFSET\tUse an offset of OFFSET nodes. Sensible "
        "\n\t\t\tvalues could be 16 for 8 threads, 128 for 32 threads. "
        "\n\t\t\tDefault: %i\n",
        DEFAULT_OFFSET);
    fprintf(out, "\t-n NUM\t\tUse NUM threads. "
        "Default: %i\n",
        DEFAULT_NTHREADS);
    fprintf(out, "\t-s SIZE\t\tInitialize queue with SIZE elements. "
        "Default: %i\n",
        DEFAULT_SIZE);
}

static void *
run(void *args)
{
    thread_args_t *as = (thread_args_t *)args;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> rand_int;
    std::uniform_int_distribution<> rand_bool(0, 1);

    /* TODO: Adjust to our machines. */
    pin(gettid(), as->id);

    // call in to main thread
    std::atomic_fetch_add(&wait_barrier, 1);

    // wait until signaled by main thread
    while (!loop.load(std::memory_order_relaxed)) {
        /* Wait */;
    }

    uint32_t cnt = 0;
    /* start benchmark execution */
    do {
        uint32_t v;
        if (rand_bool(gen) == 0) {
            pq_heap.insert(rand_int(gen));
        } else {
            pq_heap.delete_min(v);
        }
        cnt++;
    } while (loop.load(std::memory_order_relaxed));
    /* end of measured execution */

    as->measure = cnt;

    return NULL;
}

int
main(int argc __attribute__ ((unused)),
     char **argv __attribute__ ((unused)))
{
    int nthreads  = DEFAULT_NTHREADS;
    int offset    = DEFAULT_OFFSET;
    int secs      = DEFAULT_SECS;
    int init_size = DEFAULT_SIZE;

    int opt;
    while ((opt = getopt(argc, argv, "hn:o:s:t:")) >= 0) {
        switch (opt) {
        case 'h': usage(stdout, argv[0]); exit(EXIT_SUCCESS); break;
        case 'n': nthreads	= atoi(optarg); break;
        case 'o': offset	= atoi(optarg); break;
        case 's': init_size	= atoi(optarg); break;
        case 't': secs		= atoi(optarg); break;
        default: assert(0);
        }
    }

    pq_heap.init(init_size);

    thread_args_t *ts = new thread_args_t[nthreads];
    memset(ts, 0, nthreads * sizeof(thread_args_t));

    thread_args_t *t;
    for (int i = 0; i < nthreads && (t = &ts[i]); i++) {
        t->id = i;
        pthread_create(&t->thread, NULL, run, t);
    }

    while (wait_barrier.load(std::memory_order_relaxed) != nthreads) {
        /* Wait. */;
    }

    struct timespec start, end;

    loop.store(true);
    gettime(&start);
    usleep(1000000 * secs);
    loop.store(false);
    gettime(&end);

    for (int i = 0; i < nthreads && (t = &ts[i]); i++) {
        pthread_join(t->thread, NULL);
    }

    /* PRINT PERF. MEASURES */
    int sum = 0, min = std::numeric_limits<int>::max(), max = 0;

    for (int i = 0; i < nthreads && (t = &ts[i]); i++) {
        sum += t->measure;
        min = std::min(min, t->measure);
        max = std::max(max, t->measure);
    }
    struct timespec elapsed = timediff(start, end);
    double dt = elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000.0;

    printf("Total time:\t%1.8f s\n", dt);
    printf("Ops:\t\t%d\n", sum);
    printf("Ops/s:\t\t%.0f\n", (double) sum / dt);
    printf("Min ops/t:\t%d\n", min);
    printf("Max ops/t:\t%d\n", max);

    delete[] ts;

    return 0;
}

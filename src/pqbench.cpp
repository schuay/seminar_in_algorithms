#include <atomic>
#include <cstdio>
#include <limits>
#include <hwloc.h>
#include <random>

#include "globallock.h"
#include "heap.h"
#include "linden.h"
#include "noble.h"
#include "spraylist.h"

#undef max /* Clash between macro and limits. */
#undef min

#define DEFAULT_SECS     (10)
#define DEFAULT_NTHREADS (1)
#define DEFAULT_OFFSET   (128)
#define DEFAULT_SIZE     (1 << 15)
#define DEFAULT_VERBOSE  (false)

static std::atomic<bool> loop;
static std::atomic<int> wait_barrier;

static GlobalLock pq_globallock;
static Heap pq_heap(DEFAULT_SIZE << 3);
static Noble pq_noble;
static Linden pq_linden(DEFAULT_OFFSET);
static SprayList pq_spraylist;

typedef void (*fn_insert)(const uint32_t);
typedef bool (*fn_delete_min)(uint32_t &);

static fn_insert ins;
static fn_delete_min del;

static hwloc_topology_t topology;

template <typename T>
static void
pq_init(T &pq,
        const size_t size)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis;

    for (int i = 0; i < size; i++) {
        pq.insert(dis(gen));
    }
}

static void
usage(FILE *out,
      const char *argv0)
{
    fprintf(out, "Usage: %s [OPTION]...\n"
        "\n"
        "Options:\n", argv0);

    fprintf(out, "\t-h\t\tDisplay usage.\n");
    fprintf(out, "\t-q QUEUE\tRun benchmarks on queue of type TYPE (globallock|heap|linden|noble|spraylist).\n");
    fprintf(out, "\t-t SECS\t\tRun for SECS seconds. "
        "Default: %i\n",
        DEFAULT_SECS);
    fprintf(out, "\t-n NUM\t\tUse NUM threads. "
        "Default: %i\n",
        DEFAULT_NTHREADS);
    fprintf(out, "\t-s SIZE\t\tInitialize queue with SIZE elements. "
        "Default: %i\n",
        DEFAULT_SIZE);
    fprintf(out, "\t-v\tEnable verbose output. Default: %i\n",
        DEFAULT_VERBOSE);
}

static void
pin_to_core(const int id)
{
    const int depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CORE);
    const int ncores = hwloc_get_nbobjs_by_depth(topology, depth);

    const hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, depth, id % ncores);

    hwloc_cpuset_t cpuset = hwloc_bitmap_dup(obj->cpuset);
    hwloc_bitmap_singlify(cpuset);

    if (hwloc_set_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD) != 0) {
        fprintf(stderr, "Could not bind to core: %s\n", strerror(errno));
    }

    hwloc_bitmap_free(cpuset);
}

static void *
run(void *args)
{
    thread_args_t *as = (thread_args_t *)args;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> rand_int;
    std::uniform_int_distribution<> rand_bool(0, 1);

    /* Special handling for SprayList. */
    pq_spraylist.init_thread(as->nthreads);

    pin_to_core(as->id);

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
            ins(rand_int(gen));
        } else {
            del(v);
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
    int secs      = DEFAULT_SECS;
    int init_size = DEFAULT_SIZE;
    bool verbose  = DEFAULT_VERBOSE;

    const char *type_str = nullptr;

    /* A hack to avoid segfault on destructor in empty linden queue. */
    pq_linden.insert(42);

    int opt;
    while ((opt = getopt(argc, argv, "hn:o:q:s:t:v")) >= 0) {
        switch (opt) {
        case 'h': usage(stdout, argv[0]); exit(EXIT_SUCCESS); break;
        case 'n': nthreads  = atoi(optarg); break;
        case 'q': type_str  = optarg; break;
        case 's': init_size = atoi(optarg); break;
        case 't': secs      = atoi(optarg); break;
        case 'v': verbose   = true; break;
        default: assert(0);
        }
    }

    if (type_str == nullptr) {
        usage(stderr, argv[0]);
        exit(EXIT_FAILURE);
    } else if (strcmp(type_str, "globallock") == 0) {
        ins = [](const uint32_t v) { pq_globallock.insert(v); };
        del = [](uint32_t &v) { return pq_globallock.delete_min(v); };
        pq_init(pq_globallock, init_size);
    } else if (strcmp(type_str, "heap") == 0) {
        ins = [](const uint32_t v) { pq_heap.insert(v); };
        del = [](uint32_t &v) { return pq_heap.delete_min(v); };
        pq_init(pq_heap, init_size);
    } else if (strcmp(type_str, "linden") == 0) {
        ins = [](const uint32_t v) { pq_linden.insert(v); };
        del = [](uint32_t &v) { return pq_linden.delete_min(v); };
        pq_init(pq_linden, init_size);
    } else if (strcmp(type_str, "noble") == 0) {
        ins = [](const uint32_t v) { pq_noble.insert(v); };
        del = [](uint32_t &v) { return pq_noble.delete_min(v); };
        pq_init(pq_noble, init_size);
    } else if (strcmp(type_str, "spraylist") == 0) {
        ins = [](const uint32_t v) { pq_spraylist.insert(v); };
        del = [](uint32_t &v) { return pq_spraylist.delete_min(v); };
        pq_init(pq_spraylist, init_size);
    } else {
        usage(stderr, argv[0]);
        exit(EXIT_FAILURE);
    }

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    thread_args_t *ts = new thread_args_t[nthreads];
    memset(ts, 0, nthreads * sizeof(thread_args_t));

    thread_args_t *t;
    for (int i = 0; i < nthreads && (t = &ts[i]); i++) {
        t->id = i;
        t->nthreads = nthreads;
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

    if (verbose) {
        printf("Total time:\t%1.8f s\n", dt);
        printf("Ops:\t\t%d\n", sum);
        printf("Ops/s:\t\t%.0f\n", (double) sum / dt);
        printf("Min ops/t:\t%d\n", min);
        printf("Max ops/t:\t%d\n", max);
    } else {
        printf("%.0f\n", (double) sum / dt);
    }

    hwloc_topology_destroy(topology);
    delete[] ts;

    return 0;
}

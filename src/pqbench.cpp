#include <cstdio>

#include "heap.h"
#include "linden.h"
#include "noble.h"

#define DEFAULT_SECS     (10)
#define DEFAULT_NTHREADS (1)
#define DEFAULT_OFFSET   (32)
#define DEFAULT_SIZE     (1 << 15)

template <class T>
static void
generic_pq_use(T &pq)
{
    pq.insert(1);
    pq.insert(2);
    pq.insert(3);

    uint32_t v;
    const bool ret = pq.delete_min(v);
    printf("delete_min -> (%d, %d)\n", ret, v);
}

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

int
main(int argc __attribute__ ((unused)),
     char **argv __attribute__ ((unused)))
{
    int opt;
    thread_args_t *t;

    int nthreads  = DEFAULT_NTHREADS;
    int offset    = DEFAULT_OFFSET;
    int secs      = DEFAULT_SECS;
    int init_size = DEFAULT_SIZE;

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

    Heap heap(42);
    generic_pq_use(heap);

    Noble noble;
    generic_pq_use(noble);

    Linden linden(42);
    generic_pq_use(linden);

    return 0;
}

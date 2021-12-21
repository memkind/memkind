#define _GNU_SOURCE
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <memkind.h>

#define PN "memtier"

extern char **environ;

static __attribute__((noreturn)) void die(const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (write(STDERR_FILENO, buf, len))
        ;
    exit(1);
}

// Require the approximation to be within 10%.
#define ACCURACY 11 / 10

static size_t human_friendly_ratio(size_t x, size_t y)
{
    if (x < y) {
        size_t t = x;
        x = y;
        y = t;
    }

    if (!y)
        return x;

    for (size_t b = 1;; b++) {
        // Make a/b the nearest fraction to x/y.
        size_t a = (2 * x * b + x / 2) / y / 2;

        printf("%zu/%zu → %zu/%zu Δ=%f\n", x, y, a, b,
               (1.0 * a / b) / (1.0 * x / y));
        if (a * y > x * b * ACCURACY)
            continue;
        if (a * y * ACCURACY < x * b)
            continue;
        return y / b;
    }
}

#define vmsg(...)                                                              \
    do                                                                         \
        if (verbose)                                                           \
            fprintf(stderr, __VA_ARGS__);                                      \
    while (0)

int main(int argc, char **argv)
{
    unsigned long ratio[2];
    bool noratio = true;
    const char *thresh = 0;
    bool verbose = false;

    static struct option long_options[] = {
        {"ratio", required_argument, 0, 'r'},
        {"threshold", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0}};

    while (1) {
        switch (getopt_long(argc, argv, "+r:t:vh", long_options, 0)) {
            case -1:
                goto endarg;
            case 'r':; // ancient gcc requires an empty statement here
                char *br;
                ratio[0] = strtoul(optarg, &br, 0);
                if (*br != ':')
                    die(PN ": --ratio must have a colon between values.\n");
                ratio[1] = strtoul(++br, &br, 0);
                if (*br)
                    die(PN
                        ": --ratio must have exactly two values in this version of memtier.\n");
                noratio = false;
                break;
            case 't':
                thresh = optarg; // TODO: validate?
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                printf(
                    "memtier is a malloc interceptor to use memkind memory tiering with unmodified\n"
                    "programs.  Use it as a prefix:\n"
                    "    memtier [options] program [args]\n"
                    "Options:\n"
                    "    -r/--ratio      colon-separated relative portions of memory tiers to use,\n"
                    "                    defaults to amount of memory installed on this system\n"
                    "    -t/--threshold  alloc size threshold configurations, see the man page\n"
                    "    -v/--verbose    display generated configuration\n");
                exit(0);
            default:
                die(PN ": unknown option ｢%s｣\n", argv[optind]);
        }
    }
endarg:

    if (noratio) {
        ssize_t dram = memkind_get_capacity(MEMKIND_REGULAR);
        if (dram < 0)
            dram = 0;
        ssize_t pmem = memkind_get_capacity(MEMKIND_DAX_KMEM);
        if (pmem <= 0) // explicit ratio of 0 is ok, implicit suggests an error
            die(PN
                ": there appears to be no KMEM_DAX devices installed in this system.\n");
        size_t d = human_friendly_ratio(dram, pmem);
        ratio[0] = dram / d;
        ratio[1] = pmem / d;
        vmsg("Using ratio %zu:%zu\n", ratio[0], ratio[1]);
    }

    if (!ratio[0] && !ratio[1])
        die(PN ": empty ratio would disallow using any memory.\n");

    if (thresh && (!ratio[0] || !ratio[1]))
        die(PN ": --threshold requires multiple enabled tiers.\n");

    if (optind >= argc)
        die("Usage: " PN " [options] command [args]\n");

    printf("\e[31:1m$0=｢\e[22m%s\e[1m｣\e[0m\n", argv[0]);
    char libpath[4096];
    // If our tool is not properly installed, assume the library lives in the
    // same dir.
    char *exeslash = strncmp(argv[0], "/usr", 4) ? strrchr(argv[0], '/') : 0;
    if (exeslash) {
        const char *subdir = "";
        // libtool puts the wrapper into .libs/memtier but the lib into
        // tiering/.libs/libmemtier.so
        if (exeslash - libpath >= 5 && !strncmp(exeslash - 5, ".libs/", 5)) {
            exeslash -= 5;
            subdir = "tiering/.libs";
        }
        snprintf(libpath, sizeof(libpath), "%.*s%s/libmemtier.so",
                 (int)(exeslash - argv[0]), argv[0], subdir);
    } else
        strcpy(libpath, "libmemtier.so");
    char *env = getenv("LD_PRELOAD");
    if ((env ? asprintf(&env, "LD_PRELOAD=%s:%s", libpath, env)
             : asprintf(&env, "LD_PRELOAD=%s", libpath)) == -1)
        die(PN ": out of memory\n");
    vmsg("setting %s\n", env);
    putenv(env);

    if (!ratio[1]) // DRAM only
        putenv("MEMKIND_MEM_TIERS=KIND:DRAM,RATIO:1;POLICY:STATIC_RATIO");
    else if (!ratio[0]) // PMEM only
        putenv("MEMKIND_MEM_TIERS=KIND:KMEM_DAX,RATIO:1;POLICY:STATIC_RATIO");
    else {
        if (asprintf(
                &env,
                "MEMKIND_MEM_TIERS=KIND:DRAM,RATIO:%lu;KIND:KMEM_DAX,RATIO:%lu;POLICY:%s",
                ratio[0], ratio[1],
                thresh ? "DYNAMIC_THRESHOLD" : "STATIC_RATIO") == -1)
            die(PN ": out of memory\n");
        vmsg("setting %s\n", env);
        putenv(env);

        if (thresh) {
            if (asprintf(&env, "MEMKIND_MEM_THRESHOLDS=%s", thresh) == -1)
                die(PN ": out of memory\n");
            vmsg("setting %s\n", env);
            putenv(env);
        }
    }

    execvpe(argv[optind], argv + optind, environ);
    // We land here only on error.
    die(PN ": couldn't exec ｢%s｣: %m\n", argv[optind]);
}

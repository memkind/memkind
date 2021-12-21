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
    if (write(2, buf, len))
        ;
    exit(1);
}

static size_t gcd(size_t x, size_t y)
{
    while (x && y)
        if (x < y)
            y %= x;
        else
            x %= y;

    return x ? x : y;
}

#define vmsg(...)                                                              \
    if (verbose)                                                               \
    fprintf(stderr, __VA_ARGS__)

int main(int argc, char **argv)
{
    unsigned long ratio[2];
    bool noratio = true;
    const char *thresh = 0;
    bool verbose = true; // TODO: flip to false

    int a = 1;
    while (a < argc) {
        if (argv[a][0] != '-')
            break;
        if (!strcmp(argv[a], "--")) {
            a++;
            break;
        }
        if (!strcmp(argv[a], "-r") ||
            !strcmp(argv[a], "--ratio")) { // TODO: -r1:3
            if (++a >= argc)
                die(PN ": --ratio requires an argument.\n");
            char *br;
            ratio[0] = strtoul(argv[a++], &br, 0);
            if (*br != ':')
                die(PN ": --ratio must have a colon between values.\n");
            ratio[1] = strtoul(++br, &br, 0);
            if (*br)
                die(PN
                    ": --ratio must have exactly two values in this version of memtier.\n");
            noratio = false;
        } else if (!strcmp(argv[a], "-t") || !strcmp(argv[a], "--threshold")) {
            if (++a >= argc)
                die(PN ": --threshold requires an argument.\n");
            thresh = argv[a++]; // TODO: validate?
        } else if (!strcmp(argv[a], "-v") || !strcmp(argv[a], "--verbose"))
            ++a, verbose = true;
        else
            die(PN ": unknown option ｢%s｣\n", argv[a]);
    }

    if (noratio) {
        ssize_t dram = memkind_get_capacity(MEMKIND_REGULAR);
        if (dram < 0)
            dram = 0;
        ssize_t pmem = memkind_get_capacity(MEMKIND_DAX_KMEM);
        if (pmem <= 0) // explicit ratio of 0 is ok, implicit suggests an error
            die(PN
                ": there appears to be no KMEM_DAX devices installed in this system.\n");
        size_t d = gcd(dram, pmem);
        ratio[0] = dram / d;
        ratio[1] = pmem / d;
        vmsg("Using ratio %zu:%zu\n", ratio[0], ratio[1]);
    }

    if (!ratio[0] && !ratio[1])
        die(PN ": empty ratio would disallow using any memory.\n");

    if (thresh && (!ratio[0] || !ratio[1]))
        die(PN ": --threshold requires multiple tiers.\n");

    if (a >= argc)
        die("Usage: " PN " [options] command [args]\n");

    char lib[4096];
    // If our tool is not properly installed, assume the library lives in same
    // dir.
    char *exeslash = strncmp(argv[0], "/usr", 4) ? strrchr(argv[0], '/') : 0;
    if (exeslash)
        snprintf(lib, sizeof(lib), "%.*s/libmemtier.so",
                 (int)(exeslash - argv[0]), argv[0]);
    else
        strcpy(lib, "libmemtier.so");
    char *env = getenv("LD_PRELOAD");
    if ((env ? asprintf(&env, "LD_PRELOAD=%s:%s", lib, env)
             : asprintf(&env, "LD_PRELOAD=%s", lib)) == -1)
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
                "MEMKIND_MEM_TIERS=KIND:DRAM,RATIO=%lu;KIND:KMEM_DAX,RATIO=%lu;POLICY:%s",
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

    execvpe(argv[a], argv + a, environ);
    die(PN ": couldn't exec ｢%s｣: %m\n", argv[a]);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// pointer to a struct or a string?  Всё равно!

const char *MEMKIND_REGULAR = "REGULAR";
const char *MEMKIND_DAX_KMEM = "DAX_KMEM";

ssize_t memkind_get_capacity(const char *kind)
{
    char varname[64];
    snprintf(varname, sizeof varname, "FAKE_%s", kind);
    char *var = getenv(varname);

    if (!var)
        return fprintf(stderr, "%s not defined!\n", varname), -1;

    // memory amounts are unsigned
    char *err;
    size_t cap = strtoul(var, &err, 0);
    if (0 == strcmp(err, "KB"))
        cap *= 1024;
    else if (0 == strcmp(err, "MB"))
        cap *= 1048576;
    else if (0 == strcmp(err, "GB"))
        cap *= 1024 * 1048576;
    else if (0 == strcmp(err, "TB"))
        cap *= 1048576 * 1048576UL;
    else if (*err)
        return fprintf(stderr, "%s is not a number!\n", varname), -1;
    return cap;
}

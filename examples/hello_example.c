#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <numakind.h>

int main(int argc, char **argv)
{
    const size_t strlen = 512;
    char *default_str = NULL;
    char *hbw_str = NULL;
    char *hbw_hugetlb_str = NULL;
    char *hbw_preferred_str = NULL;
    char *hbw_preferred_hugetlb_str = NULL;

    numakind_init();
    
    default_str = (char *)numakind_malloc(NUMAKIND_DEFAULT, strlen);
    if (default_str == NULL) {
        perror("numakind_malloc()");
        fprintf(stderr, "Unable to allocate default string\n");
        return errno ? -errno : 1;
    }
    hbw_str = (char *)numakind_malloc(NUMAKIND_HBW, strlen);
    if (hbw_str == NULL) {
        perror("numakind_malloc()");
        fprintf(stderr, "Unable to allocate hbw string\n");
        return errno ? -errno : 1;
    }
    hbw_hugetlb_str = (char *)numakind_malloc(NUMAKIND_HBW_HUGETLB, strlen);
    if (hbw_hugetlb_str == NULL) {
        perror("numakind_malloc()");
        fprintf(stderr, "Unable to allocate hbw_hugetlb string\n");
        return errno ? -errno : 1;
    }
    hbw_preferred_str = (char *)numakind_malloc(NUMAKIND_HBW_PREFERRED, strlen);
    if (hbw_preferred_str == NULL) {
        perror("numakind_malloc()");
        fprintf(stderr, "Unable to allocate hbw_preferred string\n");
        return errno ? -errno : 1;
    }
    hbw_preferred_hugetlb_str = (char *)numakind_malloc(NUMAKIND_HBW_PREFERRED_HUGETLB, strlen);
    if (hbw_preferred_hugetlb_str == NULL) {
        perror("numakind_malloc()");
        fprintf(stderr, "Unable to allocate hbw_preferred_hugetlb string\n");
        return errno ? -errno : 1;
    }

    sprintf(default_str, "Hello world from standard memory\n");
    sprintf(hbw_str, "Hello world from high bandwidth memory\n");
    sprintf(hbw_hugetlb_str, "Hello world from high bandwidth 2 MB paged memory\n");
    sprintf(hbw_preferred_str, "Hello world from high bandwidth memory if sufficient resources exist\n");
    sprintf(hbw_preferred_hugetlb_str, "Hello world from high bandwidth 2 MB paged memory if sufficient resources exist\n");

    fprintf(stdout, "%s", default_str);
    fprintf(stdout, "%s", hbw_str);
    fprintf(stdout, "%s", hbw_hugetlb_str);
    fprintf(stdout, "%s", hbw_preferred_str);
    fprintf(stdout, "%s", hbw_preferred_hugetlb_str);

    return 0;
}

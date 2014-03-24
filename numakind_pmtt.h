#ifndef numakind_pmtt_include_h
#define numakind_pmtt_include_h

#ifdef __cplusplus
extern "C" {
#endif

static const char *NUMAKIND_BANDWIDTH_PATH = "/etc/numakind/node-bandwidth";

typedef enum {
    BW_FPARSE_ERROR_OPEN = -1,
    BW_FPARSE_ERROR_ALLOCATE = -2,
    BW_FPARSE_ERROR_WRITE = -3
}numakind_bw_error_t;

#ifdef __cplusplus
}
#endif

#endif

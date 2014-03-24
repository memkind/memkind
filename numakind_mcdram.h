#ifndef numakind_mcdram_include_h
#define numakind_mcdram_include_h
#ifdef __cplusplus
extern "C" {
#endif

const char *NUMAKIND_BANDWIDTH_PATH = "/etc/numakind/node-bandwidth";

int numakind_mcdram_isavail(void);
int numakind_mcdram_nodemask(unsigned long *nodemask, unsigned long maxnode);

#ifdef __cplusplus
}
#endif
#endif

#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stubs para funções POSIX não implementadas na newlib do devkitA64 / libnx
int fchown(int fd, uid_t owner, gid_t group) {
    (void)fd;
    (void)owner;
    (void)group;
    return 0;
}

uid_t geteuid(void) {
    return 0;
}

#ifdef __cplusplus
}
#endif

#include <errno.h>
#include <sys/auxv.h>
#include <unistd.h>

int issetugid() {
  errno = 0;
  return getauxval(AT_SECURE) || errno == ENOENT;
}

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int sz;
  char *buf;

  sz = memsize();
  printf("The current memory size is %d bytes\n", sz);

  buf = malloc(20000);

  sz = memsize();
  printf("The current memory size after allocation is %d bytes\n", sz);

  free(buf);

  sz = memsize();
  printf("The current memory size after free is %d bytes\n", sz);

  exit(0);
}
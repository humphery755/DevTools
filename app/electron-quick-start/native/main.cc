
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

extern "C" {
 int XAdd(int, int);
}
int main(int argc, char**argv){
  printf("add(1,1)=%d\n", XAdd(1,1));
  return 0;
}

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define EXAMPLE __declspec(dllexport)

extern "C" {
//EXAMPLE int XAdd(int, int);


EXAMPLE int XAdd(int a, int b){
  return a+b;
}

}

/********************************************************************
 * File: trace.c
 *
 * main function for the pvtrace utility.
 *
 * Author: M. Tim Jones <mtj@mtjones.com>
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "symbols.h"
#include "stack.h"

//unsigned char file_buffer[50]; 

int main( int argc, char *argv[] )
{
  FILE *tracef;
  char type;
  unsigned int address;

  if (argc != 2) {
    printf("Usage: pvtrace <image>\n\n");
    exit(-1);
  }
  initSymbol( argv[1] );
  stackInit();
  tracef = fopen("trace.txt", "r");
  if (tracef == NULL) {
    printf("Can't open trace.txt\n");
    exit(-1);
  }

  while(!feof(tracef)){
    //sscanf(file_buffer, "%c0x%x\n",&type, &address);
    fscanf( tracef, "%c0x%x\n", &type, &address );
    //printf("%s:  %c,0x%x\n",file_buffer,type,address);
    switch(type){
      case 'E':
      /* Function Entry */
      addSymbol( address );
      addCallTrace( address );
      stackPush( address );
      break;
      case 'X': 
      /* Function Exit */
      (void) stackPop();
      break;
      default:
      break;
    }
  }

  emitSymbols();
  fclose( tracef );  
  return 0;
}



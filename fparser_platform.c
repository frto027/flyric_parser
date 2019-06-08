#include "fparser_platform.h"

#include <stdlib.h>
#include <stdio.h>


void * frpmalloc(unsigned int size){
    return malloc(size);
}

void frpfree(void * ptr){
    free(ptr);
}

void frpErrorCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]){
    printf("Compile error at line %d\n",linecount);
    printf("%s\n",msg);
    while(cursor--)
        putchar(' ');
    putchar('^');
    putchar('\n');
    if(extra)
        printf("%s\n\n",extra);

}
void frpWarringCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]){
    printf("Warring at line %d\n",linecount);
    printf("%s\n",msg);
    while(cursor--)
        putchar(' ');
    putchar('^');
    putchar('\n');
    if(extra)
        printf("%s\n\n",extra);
}

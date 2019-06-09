#include "fparser_platform.h"

#include <stdlib.h>
#include <stdio.h>

#define BUILTIN_FUNC(funcname) float funcname(float * args)

void * frpmalloc(unsigned int size){
    return malloc(size);
}

void frpfree(void * ptr){
    free(ptr);
}

void frpErrorCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]){
    printf("Compile error at line %d col %d\n",linecount+1,cursor+1);
    printf("%s\n",msg);
    while(cursor--)
        putchar(' ');
    putchar('^');
    putchar('\n');
    if(extra)
        printf("%s\n\n",extra);

}
void frpWarringCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]){
    printf("Warring at line %d col %d\n",linecount+1,cursor+1);
    printf("%s\n",msg);
    while(cursor--)
        putchar(' ');
    putchar('^');
    putchar('\n');
    if(extra)
        printf("%s\n\n",extra);
}


#include <math.h>
BUILTIN_FUNC(frp_sin){
    return sinf(args[0]);
}

float frpCurveExpressAdd(float a,float b){
    //printf("%f + %f --> %f\n",a,b,a+b);
    return a + b;
}

float frpCurveExpressSub(float a,float b){
    //printf("%f - %f --> %f\n",a,b,a-b);
    return a - b;
}


float frpCurveExpressMul(float a,float b){
    //printf("%f * %f --> %f\n",a,b,a*b);
    return a * b;
}


float frpCurveExpressDiv(float a,float b){
    //printf("%f / %f --> %f\n",a,b,a/b);
    return a / b;
}

float frpCurveExpressAddW(float a[]){return frpCurveExpressAdd(a[0],a[1]);}
float frpCurveExpressSubW(float a[]){return frpCurveExpressSub(a[0],a[1]);}
float frpCurveExpressMulW(float a[]){return frpCurveExpressMul(a[0],a[1]);}
float frpCurveExpressDivW(float a[]){return frpCurveExpressDiv(a[0],a[1]);}

FRCBuiltinFunctionType FRCBuiltinFunctions[] = {
    {"sin",frp_sin,1},
    {NULL,NULL,0}
};

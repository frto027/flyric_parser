#include "fparser.h"
#include "fparser_platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define BUILTIN_FUNC(funcname) float funcname(float * args)

// memory malloc,realloc and free function
void * frpmalloc(unsigned long size){
    return malloc(size);
}

void * frprealloc (void * ptr,unsigned long size){
    return realloc(ptr,size);
}

void frpfree(void * ptr){
    free(ptr);
}
// these function will be called when any parse error happened.
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



//the builtin operator '+'
float frpCurveExpressAdd(float a,float b){
    //printf("%f + %f --> %f\n",a,b,a+b);
    return a + b;
}
//the builtin operator '-'
float frpCurveExpressSub(float a,float b){
    //printf("%f - %f --> %f\n",a,b,a-b);
    return a - b;
}
//the builtin operator '*'
float frpCurveExpressMul(float a,float b){
    //printf("%f * %f --> %f\n",a,b,a*b);
    return a * b;
}
//the builtin operator '/'
float frpCurveExpressDiv(float a,float b){
    //printf("%f / %f --> %f\n",a,b,a/b);
    return a / b;
}
//the builtin operator '-'(negative)
//frpCurveExpressNegW is ok.skip

//these functions are called by bision for const value eval.
float frpCurveExpressAddW(float a[]){return frpCurveExpressAdd(a[0],a[1]);}
float frpCurveExpressSubW(float a[]){return frpCurveExpressSub(a[0],a[1]);}
float frpCurveExpressMulW(float a[]){return frpCurveExpressMul(a[0],a[1]);}
float frpCurveExpressDivW(float a[]){return frpCurveExpressDiv(a[0],a[1]);}
float frpCurveExpressNegW(float *a){
    return -*a;
}


//some builtin function that curve used
//declare function here
BUILTIN_FUNC(frp_sin){
    return sinf(args[0]);
}
//add functions defin here.
FRCBuiltinFunctionType FRCBuiltinFunctions[] = {
    {"sin",frp_sin,1},
    {NULL,NULL,0}
};

//do not change this function if you don't know what it is.
//define the default parameter name of an anim curve expression.
//never more than FRCE_MAX_ARG_COUNT.
void frpAnimFuncArgInit(){
    frp_bison_arg_listcount = 0;
    frp_bison_arg_names_raw[frp_bison_arg_listcount++] = "x";
    frp_bison_arg_names_raw[frp_bison_arg_listcount++] = "t";
    frp_bison_arg_names_raw[frp_bison_arg_listcount++] = "d";
}
void frpAnimFuncArgCalc(float * arg){
    //please refer to fparser.c
    //arg[i] is the argument called frp_bison_arg_names_raw[i]
}

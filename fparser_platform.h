#ifndef FPARSER_PLAT_H
#define FPARSER_PLAT_H

#ifndef NULL
#define NULL 0
#endif
//you can change this file or redefine methods here for your own program

//file format without BOM is adviced, but txt from windows notepad has a bad head.
#define FRP_AUTO_CHECK_BOM 1

typedef char frp_bool;
typedef unsigned int frp_size;
typedef unsigned char frp_uint8;
typedef signed long long frp_time;

///
/// \brief frpmalloc
/// malloc memory
/// \param size
/// size of memory
/// \return
/// pointer to size
///
extern void * frpmalloc(unsigned long size);
extern void * frprealloc (void * ptr,unsigned long size);
///
/// \brief frpfree
/// free memory
/// \param ptr
/// pointer to memory
///
extern void frpfree(void * ptr);

extern void frpErrorCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]);
extern void frpWarringCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]);

typedef struct{
    const char * name;
    float (*fp)(float *);
    frp_size argc;
}FRCBuiltinFunctionType;

extern FRCBuiltinFunctionType FRCBuiltinFunctions[];

extern float frpCurveExpressAdd(float a,float b);
extern float frpCurveExpressSub(float a,float b);
extern float frpCurveExpressMul(float a,float b);
extern float frpCurveExpressDiv(float a,float b);

extern float frpCurveExpressAddW(float a[]);
extern float frpCurveExpressSubW(float a[]);
extern float frpCurveExpressMulW(float a[]);
extern float frpCurveExpressDivW(float a[]);
extern float frpCurveExpressNegW(float *a);


extern void frpAnimFuncArgInit();
extern void frpAnimFuncArgCalc(float *);
#endif // FPARSER_PLAT_H

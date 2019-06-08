#ifndef FPARSER_PLAT_H
#define FPARSER_PLAT_H

#ifndef NULL
#define NULL 0
#endif

//file format without BOM is adviced, but txt from windows notepad has a bad head.
#define FRP_AUTO_CHECK_BOM 1

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
extern void * frpmalloc(unsigned int size);
///
/// \brief frpfree
/// free memory
/// \param ptr
/// pointer to memory
///
extern void frpfree(void * ptr);

extern void frpErrorCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]);
extern void frpWarringCompileMessage(const frp_uint8 * msg,frp_size linecount,frp_size cursor,const char extra[]);


#endif // FPARSER_PLAT_H

#ifndef FPARSER__H
#define FPARSER__H

#include "fparser_public.h"
#include "fparser_platform.h"


//tool functions
//extern frp_size frpstrlen(const frp_str str);
#define frpstr_index(p_frpfile,str,i) ((p_frpfile)->textpool[(str).beg + (i)])
#define frpstr_len(str) ((frp_size)((size_t)(str).len))

///
/// \brief frpstrcmp
/// \param stra
/// \param strb
/// \return -1 0 or 1
///

//variables for flex

#define FRP_BISON_ARG_SOURCE_TEXTPOOL 0
#define FRP_BISON_ARG_SOURCE_RAWSTR 1

extern int frp_bisonlex_destroy(void);

extern const frp_uint8 * frp_flex_textpool;
extern frp_str frp_flex_textpoolstr;
extern int frp_bison_arg_source;
extern frp_size frp_bison_arg_listcount;
extern frp_str frp_bison_arg_names[FRCE_MAX_ARG_COUNT];//TEXTPOOL
extern const char * frp_bison_arg_names_raw[FRCE_MAX_ARG_COUNT];//RAWSTR
//function for flex,debug only
//extern int frp_yyflex(void);
extern int frp_bisonparse(void);
extern const char * frp_bison_errormsg;
#define FRP_BISON_TASK_CHECK 0
#define FRP_BISON_TASK_CALC 1
#define FRP_BISON_TASK_PRINT 2
extern FRCExpress * frp_bison_result;
extern int frp_bison_task;
extern FRCurveLine *frp_bison_curves_tobeused;
//defin of yystep
//bison need this for there head file. but it did not gen.may be a bug.
struct FRCExpressNodes;
typedef union frp_bison_type{
    struct FRCExpressNodes * listnodes;
    int list_count_check;
    FRCExpress * express;
    float num;
    char * temptext;
}frp_bison_type;
#define YYSTYPE frp_bison_type

#endif

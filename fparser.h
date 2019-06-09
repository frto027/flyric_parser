#ifndef FPARSER__H
#define FPARSER__H

#include "fparser_platform.h"

#define FRP_MAX_SEGMENT_PROPERTY_COUNT 128
#define FRP_MAX_SEGMENT_COUNT 3
typedef char frp_bool;
typedef struct{
    unsigned int beg;
    frp_size len;
}frp_str;

extern char FRP_support_seg_names[][30];

#define FRPVALUE_TYPE_EXT   0 //继承
#define FRPVALUE_TYPE_STR   1 //字符串
#define FRPVALUE_TYPE_INT   2 //整数
#define FRPVALUE_TYPE_NUM   3 //实数
//#define FRPVALUE_TYPE_EXP   4 //表达式
#define FRPVALUE_TYPE_TIM   5 //时间 frp_time

struct FRPExpContext{//计算表达式的上下文
    frp_time time;
};

typedef struct FRPValue{
    int type;
    union{
        frp_str str;
        int integer;
        float num;
        frp_time time;
    };
}FRPValue;
// begin of lyric line elements
typedef struct FRPNode{
    FRPValue * values;
    struct FRPNode * next;
}FRPNode;

struct FRPSeg;
typedef struct FRPLine{
    FRPNode * node;
    FRPValue * values;
    struct FRPSeg * seg;
    struct FRPLine *next;
}FRPLine;
typedef struct FRFlyc{
    FRPLine * lines;
    frp_str value_names[FRP_MAX_SEGMENT_PROPERTY_COUNT];
    frp_size value_count;
}FRFlyc;
// end of lyric line elements

// begin of animation define elements
typedef struct FRAProp{
    frp_str name;
    FRPValue * value;
    struct FRAProp * next;
}FRAProp;
typedef struct FRALine{
    frp_str name;
    FRAProp * prop;
    struct FRALine * next;
}FRALine;
typedef struct FRAnim{//property is solid
    FRALine * child;//linked list
} FRAnim;
// end of animation define elements
// begin of curve define elements
#define FRCE_TYPE_CONST 1
#define FRCE_TYPE_FUNC  2
#define FRCE_TYPE_ARGM  3
#define FRCE_TYPE_CURVE 5


#define FRCE_MAX_ARG_COUNT 10

struct FRCurveLine;
typedef struct FRCExpress{
    int type;
    union{
        float con;
        struct{
            float (*fp)(float *);
            frp_size argc;
            struct FRCExpress *argv;//calculate this first,then input these into fp
        }func;
        union{
            frp_size argid;//you should use this id to calculate arg
        };
        struct{
            struct FRCurveLine * curveline;
            struct FRCExpress *argv;
        }curveexp;
    };
}FRCExpress;

typedef struct FRCurveLine{
    frp_str curvname;
    FRCExpress *express;
    frp_size argc;
    struct FRCurveLine *next;
    //express should have args count
}FRCurveLine;
typedef struct FRCurve{
    FRCurveLine * lines;
}FRCurve;

// end of curve define elements

typedef struct FRPSeg{
    frp_str segname;
    union{
        FRFlyc flyc;
        FRAnim anim;
        FRCurve curve;
    };
}FRPSeg;

typedef struct FRPFile{
    FRPSeg segs[FRP_MAX_SEGMENT_COUNT];
    frp_size segcount;
    const frp_uint8 * textpool;
    frp_bool release_textpool;
}FRPFile;

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
extern int frpstr_cmp(const frp_uint8 * textpool,const frp_str stra,const frp_str strb);
extern int frpstr_rcmp(const frp_uint8 * textpool,const frp_str stra,const frp_uint8 strb[]);
extern int frpstr_rrcmp(const frp_uint8 stra[],const frp_uint8 strb[]);
//这里会处理转义符的
extern void frpstr_fill(const frp_uint8 * textpool,const frp_str str,frp_uint8 buff[],frp_size size);
//find and get a segment from file,do not build new segment.return null if failed
extern FRPSeg * frp_getseg(FRPFile * file,const frp_uint8 * name);

extern void frpinit(void);

extern FRPFile * frpopen(const frp_uint8 * lyric,frp_size maxlength,frp_bool no_copy);
extern void frpdestroy(FRPFile * file);
//增加某一属性的处理方式


#define FRP_FLYC_PTYPE_STR      0 //全局默认字符串
#define FRP_FLYC_PTYPE_TYPE     1 //字符串，但支持持Line和Type两个
#define FRP_FLYC_PTYPE_DURATION 2 //frp_time格式，支持即时计算mnext单词
#define FRP_FLYC_RELATION_LINE  3 //相对上一个Line的偏移，主要用于StartTime属性 最终是一个TIM
#define FRP_FLYC_PTYPE_NUM      4 //实数 只有这一个能应用动画
#define FRP_FLYC_PTYPE_INT      5 //整数

extern void frp_flyc_add_parse_rule(const char *name,int frp_flyc_ptype);

//variables for flex

extern const frp_uint8 * frp_flex_textpool;
extern frp_str frp_flex_textpoolstr;
extern frp_size frp_bison_arg_listcount;
extern frp_str frp_bison_arg_names[FRCE_MAX_ARG_COUNT];
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

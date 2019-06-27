#ifndef FPARSER_PUB_H
#define FPARSER_PUB_H

//type define
#include "fparser_platform.h"
//定义字符串
typedef struct{
    unsigned int beg;
    frp_size len;
}frp_str;

//count define
//flyc segment max property count
//flyc段最多有几个属性
#define FRP_MAX_SEGMENT_PROPERTY_COUNT 128
//max segments can be parsed in file
//歌词文件中最多会处理几个段
#define FRP_MAX_SEGMENT_COUNT 3
//max argument count in curve expression function call
//曲线的表达式最多能有几个参数
#define FRCE_MAX_ARG_COUNT 10

//========================data struct define=================

//begin of flyc property define
#define FRPVALUE_TYPE_EXT   0 //继承 unused
#define FRPVALUE_TYPE_STR   1 //字符串
#define FRPVALUE_TYPE_INT   2 //整数
#define FRPVALUE_TYPE_NUM   3 //实数
//#define FRPVALUE_TYPE_EXP   4 //表达式
#define FRPVALUE_TYPE_TIM   5 //时间 frp_time
#define FRPVALUE_TYPE_RELATIVE_TIME_LINE 6//临时类型，相对于行的偏移
#define FRPVALUE_TYPE_RELATIVE_TIME_NODE 7//临时类型，相对于Node的偏移
//FRLanim save the animate in current property
//flyc段中的某个属性
struct FRLanim;
typedef struct FRPValue{
    int type;
    union{
        frp_str str;
        int integer;
        struct{
            float num;
            struct FRLanim * anim_apply;
        };
        frp_time time;
    };
}FRPValue;
//end of flyc property define

// begin of lyric line elements
typedef struct FRPNode{
    FRPValue * values;
    frp_time starttime,endtime;
    struct FRPNode * next;
}FRPNode;

struct FRPSeg;
typedef struct FRPLine{
    FRPNode * node;
    FRPValue * values;
    struct FRPSeg * seg;
    struct FRPLine *next;
    frp_time starttime,endtime;
}FRPLine;
typedef struct FRFlyc{
    FRPLine * lines;
    frp_str value_names[FRP_MAX_SEGMENT_PROPERTY_COUNT];
    frp_size value_count;
}FRFlyc;
// end of lyric line elements

// begin of curve define elements
#define FRCE_TYPE_CONST 1
#define FRCE_TYPE_FUNC  2
#define FRCE_TYPE_ARGM  3
#define FRCE_TYPE_PROPERTY 4//读取属性值
#define FRCE_TYPE_CURVE 5
#define FRCE_TYPE_PROPERTY_NAME 6

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
        union{
            //property name.use this after bison perser.replace with propid before calculate expression
            frp_uint8 * propname;
            frp_size propid;//property id of curve.
        };
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

// begin of animation define elements
typedef struct FRAProp{
    union{
        frp_str property_name;//after parse segment
        frp_size property_id;//after parse all file(id of flyc segment property)
    };
    FRCExpress * func_exp;//argc = 3 (x,t,d)
    FRCExpress * during_exp;//argc = 0
    FRCExpress * offset_exp;
    //int play_time;//FRAP_PLAYTIME
    struct FRAProp * next;
}FRAProp;
typedef struct FRALine{
    frp_str name;
    FRAProp * prop;
    struct FRALine * next;
}FRALine;
typedef struct FRAnim{//property is solid
    FRALine * lines;//linked list
} FRAnim;
// end of animation define elements

typedef struct FRPSeg{
    frp_str segname;
    union{
        FRFlyc flyc;
        FRAnim anim;
        FRCurve curve;
    };
}FRPSeg;
//frp_play used
//记录某个属性上应用的动画信息
typedef struct FRLanim{
    frp_time starttime,endtime;
    FRAProp * animprop;//这个不归当前元素释放
    struct FRLanim * next;
}FRLanim;

typedef struct FRTNode{
    FRPLine * line;//释放的时候不要释放这个
    int refcount;//对next的引用计数
    struct FRTNode * next;//双向链表
}FRTNode;
typedef struct FRTimeline{
    FRTNode * lines;//当前时间点播放的所有行链表
    frp_time begtime;//当前点的开始时间
    struct FRTimeline *before,*after;
}FRTimeline;
//file
typedef struct FRPFile{
    FRPSeg segs[FRP_MAX_SEGMENT_COUNT];
    frp_size segcount;
    const frp_uint8 * textpool;
    frp_bool release_textpool;
    FRTimeline * timeline;//时间线，双向链表，这个不是头节点，而是随着文件读取前后滑动的
}FRPFile;

//===================function define==================

//fparser library
//library init and destroy
extern void frpstartup(void);
extern void frpshutdown(void);

//configure of library.
//call it after frpstartup,before open any file.
//只有调用这个方法后才能应用动画，property是动画名字，字符串。
extern void frp_anim_add_support(const frp_uint8 * property);

//you may want to change a value type of property in flyc segment.use it carefully.
//增加某一属性的处理方式
#define FRP_FLYC_PTYPE_STR      0 //全局默认字符串
#define FRP_FLYC_PTYPE_TYPE     1 //字符串，但支持持Line和Type两个
#define FRP_FLYC_PTYPE_DURATION 2 //frp_time格式，支持即时计算mnext单词
#define FRP_FLYC_RELATION_LINE  3 //相对上一个Line的偏移(单箭头> <)或Node的偏移(>> <<)，主要用于StartTime属性 最终是一个TIM
#define FRP_FLYC_PTYPE_NUM      4 //实数 只有这一个能应用动画
#define FRP_FLYC_PTYPE_INT      5 //整数
extern void frp_flyc_add_parse_rule(const char *name,int frp_flyc_ptype);


//open and close file
//如果nocopy为真，则传入的lyric在之后会作为文字池被使用，不能修改，否则建立一个lyric的拷贝
extern FRPFile * frpopen(const frp_uint8 * lyric,frp_size maxlength,frp_bool no_copy);
extern void frpdestroy(FRPFile * file);
 
//get segment from file
//find and get a segment from file,do not build new segment.return null if failed
//获取文件中符合某一名字的段，没有被解析的段是获取不到的
extern FRPSeg * frp_getseg(FRPFile * file,const frp_uint8 * name);

//you should get an id for a property to access the value.
//获得属性id，所有的计算都通过属性id来进行识别
extern frp_size frp_play_get_property_id(FRPFile* file,const frp_uint8 *property_name);
//if the line is not showed,the value is can be calculated too.
//计算浮点属性值，这是包含动画的
extern float frp_play_property_float_value(frp_time time, FRPValue * values,frp_size property_id);

//get lines when playing at 'time'
extern FRTNode * frp_play_getline_by_time(FRPFile * file,frp_time time);
extern frp_time frp_play_next_switchline_time(FRPFile * file,frp_time time);
extern frp_bool frp_play_has_more_line(FRPFile * file);
extern frp_size frp_play_fill_node_text(FRPFile * file, FRPNode * node,frp_size property_id,frp_uint8 * buff,frp_size size);
extern frp_size frp_play_fill_line_text(FRPFile * file, FRPLine * line,frp_size property_id,frp_uint8 * buff,frp_size size);

//===============string function==================
extern int frpstr_cmp(const frp_uint8 * textpool,const frp_str stra,const frp_str strb);
extern int frpstr_rcmp(const frp_uint8 * textpool,const frp_str stra,const frp_uint8 strb[]);
extern int frpstr_rrcmp(const frp_uint8 stra[],const frp_uint8 strb[]);
//这里会处理转义符的
extern frp_size frpstr_fill(const frp_uint8 * textpool,const frp_str str,frp_uint8 buff[],frp_size size);

#endif
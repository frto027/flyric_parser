﻿#include "fparser.h"
#include "fparser_platform.h"


#define MIN(a,b) ((a)>(b)?(b):(a))
#define ANSI2UTF8(str) ((const frp_uint8 *)(str))

#define PARSE_SEG_ARGS FRPFile * file,frp_size * cursor,FRPSeg * seg,frp_size maxlen

void flyc_seg_parser(PARSE_SEG_ARGS);
void flyc_seg_init(FRPSeg * seg){
    seg->flyc.lines = NULL;
    seg->flyc.value_count = NULL;
}

static struct{
    const char *parser;
    void (*fp)(PARSE_SEG_ARGS);
    void (*init)(FRPSeg* seg);
} frp_seg_parserinfo[] = {
{"flyc",flyc_seg_parser,flyc_seg_init},
{NULL,NULL,NULL}
};


static FRPValue frpDefaultValue[] = {
    {    .type = FRPVALUE_TYPE_STR,
         .str={0,0}
    },{
        .type = FRPVALUE_TYPE_STR,
        .str={0,0}
    },{
        .type=FRPVALUE_TYPE_TIM,
        .time=0
    },{
        .type=FRPVALUE_TYPE_TIM,
        .time=0
    },{
        .type=FRPVALUE_TYPE_NUM,
        .num=0
    },{
        .type = FRPVALUE_TYPE_INT,
        .integer=0
    }
};

//seal the text at position i
void frp_utf8_fix(frp_uint8 buff[],frp_size i){
    if(i == 0){
        *buff=0;
        return;
    }
    frp_size j = i-1;
    while(j && (buff[j] & 0xC0) == 0x80)
        j--;
    if((buff[j] & 0xC0) == 0x80 || buff[j] == 0xFF){
        buff[j] = '\0';
    }else{
        frp_size k = 0;
        while((buff[j]<<k & 0x80))
            k++;
        if(k == 0)
            k++;
        if(j + k > i)
            buff[j]=0;
        else
            buff[j+k] = 0;
    }
}
int frpstr_rcmp(const frp_uint8 * textpool,const frp_str stra,const frp_uint8 strb[]){
    frp_size blen = 0;
    while(blen <= stra.len && strb[blen])
        blen++;
    frp_size i = MIN(stra.len,blen);
    const frp_uint8 * sa = textpool + stra.beg;
    const frp_uint8 * sb = strb;
    int ret = 0;
    while(i-- && (ret = *sa++ - *sb++) == 0)
        continue;

    if(ret > 0)
        return 1;
    if(ret < 0)
        return -1;
    if(stra.len < blen)
        return -1;
    if(stra.len > blen)
        return 1;
    return 0;
}
int frp_in_str(const frp_uint8 ch,const char *str){
    while(*str){
        if(ch == *str)
            return 1;
        str++;
    }
    return 0;
}
int frpstr_cmp(const frp_uint8 * textpool,const frp_str stra,const frp_str strb){
    frp_size i = MIN(stra.len,strb.len);
    const frp_uint8 * sa = textpool + stra.beg;
    const frp_uint8 * sb = textpool + strb.beg;
    int ret = 0;
    while(i-- && (ret = *sa++ - *sb++) == 0)
        continue;

    if(ret > 0)
        return 1;
    if(ret < 0)
        return -1;
    if(stra.len < strb.len)
        return -1;
    if(stra.len > strb.len)
        return 1;
    return 0;
}

void frpstr_fill(const frp_uint8 * textpool,const frp_str str,frp_uint8 buff[],frp_size size){
    frp_size mx = MIN(str.len,size);
    const frp_uint8 * strbeg = textpool + str.beg;
    frp_size i = 0;
    int covert = 0;
    while(i < mx){
        if(*strbeg == '\\'){
            covert = 1;
        }else{
            if(covert){
                switch (*strbeg) {
                case 'n':
                    buff[i++] = '\n';
                    break;
                case ',':
                case '\\':
                default:
                    buff[i++] = *strbeg;
                    break;
                }
            }else{
                buff[i++] = *strbeg;
            }
        }
        strbeg++;
    }
    frp_utf8_fix(buff,i);
}
//find and get seg.
FRPSeg * frp_getseg(FRPFile * file,const frp_uint8 * name){
    for(frp_size i =0 ;i<file->segcount;i++){
        if(frpstr_rcmp(file->textpool,file->segs[i].segname,name) == 0)
            return file->segs + i;
    }
    return NULL;
}
//this will make new segment if not found
FRPSeg * frp_findseg(FRPFile * file,frp_str seg_name,void (**fp)(PARSE_SEG_ARGS)){
    int support = 0;

    void (*seg_init)(FRPSeg *) = NULL;

    for(unsigned int i=0;frp_seg_parserinfo[i].parser != NULL;i++){
        if(frpstr_rcmp(file->textpool,seg_name,ANSI2UTF8(frp_seg_parserinfo[i].parser)) == 0){
            support = 1;
            *fp = frp_seg_parserinfo[i].fp;
            seg_init = frp_seg_parserinfo[i].init;
            break;
        }
    }
    if(!support)
        return NULL;

    for(frp_size i=0;i<file->segcount;i++)
        if(frpstr_cmp(file->textpool,seg_name,file->segs[i].segname) == 0)
            return file->segs + i;
    if(file->segcount > FRP_MAX_SEGMENT_COUNT)
        return NULL;

    FRPSeg * seg = file->segs + file->segcount++;
    //init seg here
    seg->segname = seg_name;
    seg_init(seg);
    return seg;
}

#define CURSOR_ARGS const frp_uint8 * lyric,frp_size * cursor,frp_size maxlen
#define CURSOR_ARGS_CALL lyric, cursor, maxlen
void frpWarringMessage(const frp_uint8 * lyric,frp_size cursor,frp_size max,char extra[]){
    frp_size beg = cursor,end = cursor;
    while(beg > 0 && !frp_in_str(lyric[beg],"\r\n"))//lyric[beg] != '\n')
        beg--;
    while(frp_in_str(lyric[beg],"\r\n"))
        beg++;
    while(end < max && !frp_in_str(lyric[end] , "\r\n"))
        end++;
    frp_size offset = cursor - beg;
    frp_size linecount = 0;
    for(frp_size i=0;i<beg;i++){
        if(lyric[i]=='\n')
            linecount++;
    }
    frp_uint8 * msg = frpmalloc(end - beg + 1);
    if(msg == NULL){
        frpErrorCompileMessage(ANSI2UTF8("???"),0,0,"Failed to alloc memory.");
        return;
    }
    for(frp_size i=beg;i<end;i++){
        msg[i - beg] = lyric[i];
    }
    msg[end - beg]='\0';

    frpWarringCompileMessage(msg,linecount,offset,extra);
}
void frpErrorMessage(const frp_uint8 * lyric,frp_size cursor,frp_size max,char extra[]){
    frp_size beg = cursor,end = cursor;
    while(beg > 0 && !frp_in_str(lyric[beg],"\r\n"))//lyric[beg] != '\n')
        beg--;
    while(frp_in_str(lyric[beg],"\r\n"))
        beg++;
    while(end < max && !frp_in_str(lyric[end] , "\r\n"))
        end++;
    frp_size offset = cursor - beg;
    frp_size linecount = 0;
    for(frp_size i=0;i<beg;i++){
        if(lyric[i]=='\n')
            linecount++;
    }
    frp_uint8 * msg = frpmalloc(end - beg + 1);
    if(msg == NULL){
        frpErrorCompileMessage(ANSI2UTF8("???"),0,0,"Failed to alloc memory.");
        return;
    }
    for(frp_size i=beg;i<end;i++){
        msg[i - beg] = lyric[i];
    }
    msg[end - beg]='\0';

    frpErrorCompileMessage(msg,linecount,offset,extra);
}

void frpParseSpace(CURSOR_ARGS){
    frp_uint8 ch;
    while(*cursor < maxlen && ((void)(ch = lyric[*cursor]), ch == ' ' || ch == '\t'))
        (*cursor)++;
}
void frpParseSkipEscape(CURSOR_ARGS){
    if(*cursor < maxlen && lyric[*cursor]=='\\'){
        (*cursor)++;
    }
    if(*cursor < maxlen)
        (*cursor)++;
}
frp_str frpParseString(CURSOR_ARGS,const char stops[]){
    frp_str s;
    s.beg = *cursor;

    while(*cursor < maxlen && !frp_in_str(lyric[*cursor],stops)){
        if(lyric[*cursor] == '\\')
            frpParseSkipEscape(CURSOR_ARGS_CALL);
        (*cursor)++;
    }

    s.len = *cursor - s.beg;
    return s;
}
int frpParseInt(CURSOR_ARGS){
    frpParseSpace(CURSOR_ARGS_CALL);
    if(*cursor >= maxlen)
        return 0;
    int positive = 1;
    int num = 0;
    if(lyric[*cursor] == '+')
        (*cursor)++;
    else if(lyric[*cursor] == '-'){
        positive = 0;
        (*cursor)++;
    }
    while(*cursor < maxlen && lyric[*cursor] >= '0' && lyric[*cursor] <= '9'){
        num = num * 10 + (lyric[*cursor] - '0');
        (*cursor)++;
    }
    frpParseSpace(CURSOR_ARGS_CALL);
    return positive ? num : -num;
}
frp_time frpParseTime(CURSOR_ARGS){
    frpParseSpace(CURSOR_ARGS_CALL);
    if(*cursor >= maxlen)
        return 0;
    int positive = 1;
    frp_time num = 0;
    if(lyric[*cursor] == '+')
        (*cursor)++;
    else if(lyric[*cursor] == '-'){
        positive = 0;
        (*cursor)++;
    }
    while(*cursor < maxlen && lyric[*cursor] >= '0' && lyric[*cursor] <= '9'){
        num = num * 10 + (lyric[*cursor] - '0');
        (*cursor)++;
    }
    frpParseSpace(CURSOR_ARGS_CALL);
    return positive ? num : -num;
}
float frpParseNum(CURSOR_ARGS){
    frpParseSpace(CURSOR_ARGS_CALL);
    int positive = 1;
    float num = 0;
    float base = 10;
    if(lyric[*cursor] == '+')
        (*cursor)++;
    else if(lyric[*cursor] == '-'){
        positive = 0;
        (*cursor)++;
    }

    while(*cursor < maxlen && lyric[*cursor] >= '0' && lyric[*cursor] <= '9'){
        num = num * 10 + (lyric[*cursor] - '0');
        (*cursor)++;
    }
    if(*cursor < maxlen && lyric[*cursor]=='.'){
        (*cursor)++;
        while(*cursor < maxlen && lyric[*cursor] >= '0' && lyric[*cursor] <= '9'){
            num += (lyric[*cursor] - '0')/base;
            base *= 10;
            (*cursor)++;
        }
    }
    return positive ? num : -num;
}
void frpParseComment(CURSOR_ARGS){
    while(*cursor < maxlen && frp_in_str(lyric[*cursor],"#\r\n")){
        while(*cursor < maxlen && !frp_in_str(lyric[*cursor] ,"\r\n"))
            (*cursor)++;
        while(*cursor < maxlen && frp_in_str(lyric[*cursor],"\r\n"))
            (*cursor)++;
    }
}
//cursor should point to the title of seg line
//get segment element here
//这个函数将不同类型的segment定位到对应的函数中，并忽略未知的segment
void frpParseSeg(FRPFile * file,frp_str name,frp_size * cursor,frp_size maxlen){
    const frp_uint8 * lyric = file->textpool;
#define NOT_END (*cursor < maxlen)
#define FRP_READ (lyric[*cursor])
#define FRP_MOVE_NEXT() do{if(NOT_END)(*cursor)++;}while(0)

#define WARRING(str) frpWarringMessage(lyric,*cursor,maxlen,str)
    void (*fp)(PARSE_SEG_ARGS);
    FRPSeg * seg = frp_findseg(file,name,&fp);
    if(seg == NULL){
        frpWarringMessage(lyric,*cursor - 1,maxlen,"Skip unsupport segments");
        while (NOT_END && FRP_READ != '['){
            //skip a line
            if(FRP_READ == ' ')
                WARRING("the space may not work as expired.");
            while(NOT_END && !frp_in_str(FRP_READ ,"\r\n"))
                (*cursor)++;
            FRP_MOVE_NEXT();
        }
        return;
    }
    fp(file,cursor,seg,maxlen);
#undef WARRING
#undef NOT_END
#undef FRP_READ
#undef FRP_MOVE_NEXT
}
//begin of flyc parser
//不同的列的类型支持
////如果满了返回FRP_MAX_SEGMENT_PROPERTY_COUNT，此时应该忽略
//frp_size flyc_get_prop_index(const frp_uint8 * textpool,FRFlyc * flyc,frp_str propname){
//    for(frp_size i=0;i<flyc->value_count;i++){
//        if(frpstr_cmp(textpool,flyc->value_names[i],propname) == 0)
//            return i;
//    }
//    if(flyc->value_count < FRP_MAX_SEGMENT_PROPERTY_COUNT){
//        flyc->value_names[flyc->value_count++]=propname;
//        return flyc->value_count - 1;
//    }
//    //failed(out of memory)
//    return FRP_MAX_SEGMENT_PROPERTY_COUNT;
//}
//返回满足条件的列id，不存在返回错误
frp_size flyc_get_prop_index_rstr(const frp_uint8 * textpool,FRFlyc * flyc,const frp_uint8 propname[]){
    for(frp_size i=0;i<flyc->value_count;i++){
        if(frpstr_rcmp(textpool,flyc->value_names[i],propname) == 0)
            return i;
    }
    return FRP_MAX_SEGMENT_PROPERTY_COUNT;
}

static struct frp_flyc_prop_type{
    const char * rule;
    int type;
    struct frp_flyc_prop_type * next;
}*frp_flyc_prop = NULL;

void frp_flyc_add_parse_rule(const char *name,int frp_flyc_ptype){
    struct frp_flyc_prop_type * node = frp_flyc_prop;

    frp_flyc_prop = frpmalloc(sizeof(struct frp_flyc_prop_type));

    if(frp_flyc_prop){
        frp_flyc_prop->next = node;
        frp_flyc_prop->rule = name;
        frp_flyc_prop->type = frp_flyc_ptype;
    }else{
        frp_flyc_prop = node;
    }
}

int flyc_get_parse_rule(const frp_uint8 * pool, frp_str name){
    struct frp_flyc_prop_type*  node = frp_flyc_prop;
    while(node->rule != NULL && frpstr_rcmp(pool,name,ANSI2UTF8(node->rule)) != 0){
        node = node->next;
    }
    return node->type;
}

void frp_flyc_free_values(FRPValue * value){
    frpfree(value);
}

void flyc_release_lines(FRPSeg * seg){
    for(FRPLine * lin = seg->flyc.lines,*next = lin->next;lin;lin = next, next = lin->next){
        frp_flyc_free_values(lin->values);
        for(FRPNode * node = lin->node,*next = node->next;node;node = next,next = node->next){
            if(node->values)
                frp_flyc_free_values(node->values);
            frpfree(node);
        }
        frpfree(lin);
    }
}

void flyc_seg_parser(PARSE_SEG_ARGS){
    const frp_uint8 * lyric = file->textpool;
#define NOT_END (*cursor < maxlen)
#define FRP_READ (lyric[*cursor])
#define FRP_MOVE_NEXT() do{if(NOT_END)(*cursor)++;}while(0)

#define WARRING(str) frpWarringMessage(lyric,*cursor,maxlen,str)


    FRFlyc * flyc = & seg->flyc;

    int ptype_map[FRP_MAX_SEGMENT_PROPERTY_COUNT];

    //skip comment
    frpParseComment(lyric,cursor,maxlen);

    //title here
    if(frp_in_str(FRP_READ ,"\r\n") || !NOT_END ){
        frpErrorMessage(lyric,*cursor,maxlen,"segment need property title here.");
        return;
    }
    while(!frp_in_str(FRP_READ ,"\r\n") && flyc->value_count < FRP_MAX_SEGMENT_PROPERTY_COUNT){
        flyc->value_names[flyc->value_count] = frpParseString(lyric,cursor,maxlen,",\r\n");
        //index_map[prop_count] = flyc_get_prop_index(lyric,flyc,frpParseString(lyric,cursor,maxlen,",\r\n"));
        ptype_map[flyc->value_count] = flyc_get_parse_rule(lyric,flyc->value_names[flyc->value_count]);
        flyc->value_count++;
        FRP_MOVE_NEXT();
    }
    if(FRP_READ == ','){
        WARRING("ignore segment properties after this variable.");
        while(NOT_END && frp_in_str(FRP_READ ,"\r\n"))
            (*cursor)++;
    }
    //特殊的几列id
    frp_size prop_type_id = flyc_get_prop_index_rstr(lyric,flyc,ANSI2UTF8("Type"));
    frp_size prop_type_starttime = flyc_get_prop_index_rstr(lyric,flyc,ANSI2UTF8("StartTime"));

    if(prop_type_id == FRP_MAX_SEGMENT_PROPERTY_COUNT){
        frpErrorMessage(lyric,*cursor,maxlen,"'Type' property should be defined.");
        return;
    }
    if(prop_type_starttime == FRP_MAX_SEGMENT_PROPERTY_COUNT){
        frpErrorMessage(lyric,*cursor,maxlen,"'StartTime' property should be defined.");
        return;
    }

    FRPLine * currentLine = NULL;
    FRPNode * currentNode = NULL;
    FRPValue * lastValue = NULL;
    FRPValue * currentValue = NULL;
    //lines here
    while(NOT_END){
        frpParseComment(lyric,cursor,maxlen);
        if(!NOT_END)
            break;
        currentValue = frpmalloc(sizeof (FRPValue) * flyc->value_count);
        for(frp_size i=0;currentValue != NULL && i<flyc->value_count;i++){
            //read a property
            switch (FRP_READ) {
            case ',':
                FRP_MOVE_NEXT();
            case '\r':
            case '\n':
                //extends
                if(lastValue == NULL)
                    currentValue[i] = frpDefaultValue[ptype_map[i]];
                else
                    currentValue[i] = lastValue[i];
                continue;
            default:
                break;
            }

            if(FRP_READ == ' ')
                FRP_MOVE_NEXT();

            //apply this switch for defferent property type
            switch (ptype_map[i]) {
            case FRP_FLYC_PTYPE_STR:
                currentValue[i].type = FRPVALUE_TYPE_STR;
                currentValue[i].str = frpParseString(lyric,cursor,maxlen,",\r\n");
                break;
            case FRP_FLYC_PTYPE_INT:
                currentValue[i].type = FRPVALUE_TYPE_INT;
                currentValue[i].integer = frpParseInt(lyric,cursor,maxlen);
                if(NOT_END && !frp_in_str(FRP_READ,",\r\n")){
                    frpErrorMessage(lyric,*cursor,maxlen,"invalid integer.");
                    goto ERROR;
                }
                break;
            case FRP_FLYC_PTYPE_NUM:
                currentValue[i].type = FRPVALUE_TYPE_NUM;
                //TODO
                currentValue[i].num = frpParseNum(lyric,cursor,maxlen);
                if(NOT_END && !frp_in_str(FRP_READ,",\r\n")){
                    frpErrorMessage(lyric,*cursor,maxlen,"invalid number.");
                    goto ERROR;
                }
                break;
            case FRP_FLYC_PTYPE_TYPE:
                currentValue[i].type = FRPVALUE_TYPE_STR;
                currentValue[i].str = frpParseString(lyric,cursor,maxlen,",\r\n");
                if(
                        frpstr_rcmp(lyric,currentValue[i].str,ANSI2UTF8("word")) != 0 &&
                        frpstr_rcmp(lyric,currentValue[i].str,ANSI2UTF8("line")) != 0
                        ){
                    frpWarringMessage(lyric,*cursor,maxlen,"unknown type.skip the line.");
                    frpfree(currentValue);
                    //skip the line
                    currentValue = NULL;
                    while(NOT_END && !frp_in_str(FRP_READ ,"\r\n"))
                        (*cursor)++;
                    while(NOT_END && frp_in_str(FRP_READ ,"\r\n"))
                        (*cursor)++;
                }
                break;
            case FRP_FLYC_RELATION_LINE:
                frpParseSpace(lyric,cursor,maxlen);
                currentValue[i].type = FRPVALUE_TYPE_TIM;
                if(FRP_READ == '>' || FRP_READ == '<'){//relative
                    int move_right = FRP_READ == '>';
                    FRP_MOVE_NEXT();
                    frp_time it = frpParseTime(lyric,cursor,maxlen);
                    if(currentLine == NULL){
                        currentValue[i].time = it;
                    }else{
                        if(move_right)
                            currentValue[i].time = currentLine->values[i].time + it;
                        else
                            currentValue[i].time = currentLine->values[i].time - it;
                    }
                }else{
                    currentValue[i].type = FRPVALUE_TYPE_TIM;
                    currentValue[i].time = frpParseTime(lyric,cursor,maxlen);
                }
                break;
            case FRP_FLYC_PTYPE_DURATION:
                frpParseSpace(lyric,cursor,maxlen);
                if(FRP_READ == 'm'){
                    currentValue[i].type = FRPVALUE_TYPE_STR;
                    currentValue[i].str = frpParseString(lyric,cursor,maxlen,",\r\n");
                    if(frpstr_rcmp(lyric ,currentValue[i].str,ANSI2UTF8("mnext")) != 0){
                        //skip the line
                        frpWarringMessage(lyric,*cursor,maxlen,"word here only support 'mnext'.skip the line.");
                        frpfree(currentValue);
                        //skip the line
                        currentValue = NULL;
                        while(NOT_END && !frp_in_str(FRP_READ ,"\r\n"))
                            (*cursor)++;
                        while(NOT_END && frp_in_str(FRP_READ ,"\r\n"))
                            (*cursor)++;
                        FRP_MOVE_NEXT();
                    }
                    //to be done future
                }else{
                    currentValue[i].type = FRPVALUE_TYPE_TIM;
                    currentValue[i].time = frpParseTime(lyric,cursor,maxlen);
                }
                break;
            }
            if(currentValue != NULL && FRP_READ == ',')
                FRP_MOVE_NEXT();
        }

        //end of a line,add current value to data struct
        if(currentValue != NULL){
            lastValue = currentValue;
            if(frpstr_rcmp(lyric,currentValue[prop_type_id].str,ANSI2UTF8("word")) == 0){
                //append to node
                if(currentLine == NULL){
                    frpErrorMessage(lyric,*cursor,maxlen,"you should not start a segment without type of 'line'");
                    goto ERROR;
                }
                //now currentNode is last node
                if(currentNode == NULL){
                    currentNode = currentLine->node = frpmalloc(sizeof(FRPNode));
                }else{
                    currentNode->next = frpmalloc(sizeof(FRPNode));
                    currentNode = currentNode->next;
                }
                if(currentNode != NULL){
                    currentNode->next = NULL;
                    currentNode->values = currentValue;
                    currentValue = NULL;
                }else{
                    frpErrorCompileMessage(ANSI2UTF8("Out of memory"),0,0,NULL);
                    goto ERROR;
                }

            }else{
                //append to new line
                //now currentLine is last line
                if(currentLine == NULL){
                    currentLine = flyc->lines = frpmalloc(sizeof(FRPLine));

                }else{
                    currentLine->next = frpmalloc(sizeof (FRPLine));
                    currentLine = currentLine->next;

                }
                if(currentLine != NULL){
                    currentLine->next = NULL;
                    currentLine->seg = seg;
                    currentLine->values = currentValue;
                    currentLine->node = NULL;
                    currentNode = NULL;
                    currentValue = NULL;
                }else{
                    frpErrorCompileMessage(ANSI2UTF8("Out of memory"),0,0,NULL);
                    goto ERROR;
                }
            }
        }
    }//while(NOT END)

    frp_size process_index[FRP_MAX_SEGMENT_PROPERTY_COUNT];
    frp_size process_count = 0;
#define SELECT_INDEX_OF_PTYPE(type) do{ process_count = 0;\
    for(frp_size i=0;i<flyc->value_count;i++){ \
        if(ptype_map[i] == type) \
            process_index[process_count++]=i; \
    }}while(0)

    //process the 'FRP_FLYC_PTYPE_DURATION's 'mnext'
    SELECT_INDEX_OF_PTYPE(FRP_FLYC_PTYPE_DURATION);

    for(FRPLine * lin = flyc->lines;lin;lin=lin->next){
        frp_time mnistval = 32767;
        if(lin->next)
            mnistval = lin->next->values[prop_type_starttime].time - lin->values[prop_type_starttime].time;
        //process for the line
        for(frp_size i = 0,index;i < process_count;i++){
            index = process_index[i];
            if(lin->values[index].type == FRPVALUE_TYPE_STR){
                if(frpstr_rcmp(lyric,lin->values[index].str,ANSI2UTF8("mnext")) == 0){
                    lin->values[index].type = FRPVALUE_TYPE_TIM;
                    lin->values[index].time = mnistval;
                }
            }
        }
        for(FRPNode * node=lin->node;node;node=node->next){
            //process for the node
            for(frp_size i = 0,index;i < process_count;i++){
                index = process_index[i];
                if(node->values[index].type == FRPVALUE_TYPE_STR){
                    if(frpstr_rcmp(lyric,node->values[index].str,ANSI2UTF8("mnext")) == 0){
                        node->values[index].type = FRPVALUE_TYPE_TIM;
                        node->values[index].time = mnistval;
                    }
                }
            }
        }
    }
    //end of process the 'FRP_FLYC_PTYPE_DURATION's 'mnext'

    return;

ERROR:
    //Todo release all memory plsease
    if(currentValue != NULL)
        frpfree(currentValue);//这里的currentValue并不一定是完整的初始化过的currentValue，直接free即可
    flyc_release_lines(seg);
    return;

#undef WARRING
#undef NOT_END
#undef FRP_READ
#undef FRP_MOVE_NEXT
}
//end of flyc parser

void frpinit(){
    const char * DefaultFloatProperty[] = {
        "ColorR","ColorG","ColorB","TransX","TransY","ScaleX","ScaleY",
        "AnchorX","AnchorY","SelfAnchorX","SelfAnchorY"
    };

    if(frp_flyc_prop == NULL){
        frp_flyc_add_parse_rule(NULL,FRP_FLYC_PTYPE_STR);
        frp_flyc_add_parse_rule("Type",FRP_FLYC_PTYPE_TYPE);
        frp_flyc_add_parse_rule("StartTime",FRP_FLYC_RELATION_LINE);
        frp_flyc_add_parse_rule("Duration",FRP_FLYC_PTYPE_DURATION);
        for(unsigned int i=0;i<sizeof (DefaultFloatProperty)/sizeof(*DefaultFloatProperty);i++){
            frp_flyc_add_parse_rule(DefaultFloatProperty[i],FRP_FLYC_PTYPE_NUM);
        }
    }
}


FRPFile * frpopen(const frp_uint8 * lyric,frp_size maxlength,frp_bool no_copy){

#ifdef FRP_AUTO_CHECK_BOM
    if(maxlength > 3 && lyric[0] == 0xEF && lyric[1] == 0xBB && lyric[2] == 0xBF)
        (void)(lyric += 3), maxlength-=3;
#endif

    FRPFile * file = frpmalloc(sizeof(FRPFile));
    if(!file)
        return NULL;
    file->segcount = 0;

    {
        frp_size ml = 0;
        while(ml < maxlength && lyric[ml])
            ml++;
        maxlength = ml;
    }
    //init the text pool of file
    file->release_textpool = 0;
    if(no_copy){
        file->textpool = lyric;
    }else{
        frp_uint8 * pool = frpmalloc(maxlength);
        if(pool == NULL)
            goto ERROR;

        for(frp_size i = 0;i<maxlength;i++)
            pool[i] = lyric[i];
        file->textpool = pool;
        file->release_textpool = 1;
    }

    //init segs and lines
    frp_size cursor = 0;
    frp_str tempstr;




#define NOT_END (cursor < maxlength)
#define FRP_READ (lyric[cursor])
#define FRP_MOVE_NEXT() do{if(NOT_END)cursor++;}while(0)

    //cursor is the next should be processed in THIS LOOP
    while(NOT_END){
        switch(lyric[cursor]){
        case '#':
            frpParseComment(lyric,&cursor,maxlength);
            break;
        case '[':
            FRP_MOVE_NEXT();
            tempstr = frpParseString(lyric,&cursor,maxlength,"]\r\n");
            if(FRP_READ != ']'){
                frpErrorMessage(lyric,cursor,maxlength,"invalid end of segment name.");
                goto COMPILE_ERROR;
            }else{
                while(NOT_END && frp_in_str(FRP_READ,"\t "))
                    FRP_MOVE_NEXT();
                if(frp_in_str(FRP_READ,"\r\n")){
                    frpErrorMessage(lyric,cursor,maxlength,"return is needed after segment name.");
                    goto COMPILE_ERROR;
                }
                FRP_MOVE_NEXT();
                frpParseSeg(file,tempstr,&cursor,maxlength);
            }
            break;
        case ' ':
            FRP_MOVE_NEXT();
            break;
        case '\r':
            FRP_MOVE_NEXT();
            if(FRP_READ == '\n')
                FRP_MOVE_NEXT();
            break;
        case '\n':
            FRP_MOVE_NEXT();
            break;
        }
    }
#undef NOT_END
#undef FRP_READ
#undef FRP_MOVE_NEXT


    return file;

COMPILE_ERROR:
ERROR:
    if(file != NULL)
        frpdestroy(file);
    return NULL;

}

void frpdestroy(FRPFile * file){
    if(file)
        frpfree(file);
}

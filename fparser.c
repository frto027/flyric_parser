#include "fparser.h"
#include "fparser_platform.h"


#define MIN(a,b) ((a)>(b)?(b):(a))
#define ANSI2UTF8(str) ((const frp_uint8 *)(str))

#define PARSE_SEG_ARGS FRPFile * file,frp_size * cursor,FRPSeg * seg,frp_size maxlen


int flyc_seg_parser(PARSE_SEG_ARGS);
int frp_curve_seg_parser(PARSE_SEG_ARGS);
int frp_anim_seg_parser(PARSE_SEG_ARGS);
void flyc_seg_init(FRPSeg * seg){
    seg->flyc.lines = NULL;
    seg->flyc.value_count = NULL;
}
void frp_curve_seg_init(FRPSeg * seg){
    seg->curve.lines = NULL;
}
void frp_anim_seg_init(FRPSeg * seg){
    seg->anim.lines = NULL;
}

static struct{
    const char *parser;
    int (*fp)(PARSE_SEG_ARGS);
    void (*init)(FRPSeg* seg);
} frp_seg_parserinfo[] = {
{"flyc",flyc_seg_parser,flyc_seg_init},
{"curve",frp_curve_seg_parser,frp_curve_seg_init},
{"anim",frp_anim_seg_parser,frp_anim_seg_init},
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
int frpstr_rrcmp(const frp_uint8 stra[],const frp_uint8 strb[]){
    while(*stra && *strb &&(*stra == *strb)){
        stra++;strb++;
    }
    return *stra - *strb;
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
//only find segment
FRPSeg * frp_findseg_not_create(FRPFile * file,frp_str seg_name){
    for(frp_size i=0;i<file->segcount;i++)
        if(frpstr_cmp(file->textpool,seg_name,file->segs[i].segname) == 0)
            return file->segs + i;
    return NULL;
}
//this will make new segment if not found
FRPSeg * frp_findseg(FRPFile * file,frp_str seg_name,int (**fp)(PARSE_SEG_ARGS)){
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
void frpWarringMessage(const frp_uint8 * lyric,frp_size cursor,frp_size max,const char extra[]){
    frp_size beg = cursor,end = cursor;
    if(beg > 0 && lyric[beg] == '\n')//ensure beg is not in line end
        beg--;
    if(beg > 0 && lyric[beg] == '\r')
        beg--;
    while(beg > 0 && !frp_in_str(lyric[beg],"\r\n"))
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
    frpfree(msg);
}
void frpErrorMessage(const frp_uint8 * lyric,frp_size cursor,frp_size max,const char extra[]){
    frp_size beg = cursor,end = cursor;
    if(beg > 0 && lyric[beg] == '\n')//ensure beg is not in line end
        beg--;
    if(beg > 0 && lyric[beg] == '\r')
        beg--;
    while(beg > 0 && !frp_in_str(lyric[beg],"\r\n"))
        beg--;
    while(beg < end && frp_in_str(lyric[beg],"\r\n"))
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
    frpfree(msg);
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
frp_str frpParseClosedString(CURSOR_ARGS,const char stops[],const char hardstops[]){
    frp_str s;
    s.beg = *cursor;
    frp_size level = 0;

    while(*cursor < maxlen && !frp_in_str(lyric[*cursor],hardstops) &&(level > 0 || !frp_in_str(lyric[*cursor],stops))){
        if(lyric[*cursor] == '\\')
            frpParseSkipEscape(CURSOR_ARGS_CALL);
        if(lyric[*cursor] == '(')
            level++;
        if(lyric[*cursor] == ')')
            level--;
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
int frpParseSeg(FRPFile * file,frp_str name,frp_size * cursor,frp_size maxlen){
    const frp_uint8 * lyric = file->textpool;
#define NOT_END (*cursor < maxlen)
#define FRP_READ (lyric[*cursor])
#define FRP_MOVE_NEXT() do{if(NOT_END)(*cursor)++;}while(0)

#define WARRING(str) frpWarringMessage(lyric,*cursor,maxlen,str)
    int (*fp)(PARSE_SEG_ARGS);

    if(frp_findseg_not_create(file,name) != NULL){
        frpErrorMessage(lyric,*cursor - 1,maxlen,"Duplicate segments define.");
        return -1;
    }
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
        return 1;//1 means skip the sigment
    }
    return fp(file,cursor,seg,maxlen);

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

int flyc_seg_parser(PARSE_SEG_ARGS){
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
        return -1;
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
        return -1;
    }
    if(prop_type_starttime == FRP_MAX_SEGMENT_PROPERTY_COUNT){
        frpErrorMessage(lyric,*cursor,maxlen,"'StartTime' property should be defined.");
        return -1;
    }

    FRPLine * currentLine = NULL;
    FRPNode * currentNode = NULL;
    FRPValue * lastValue = NULL;
    FRPValue * currentValue = NULL;
    //lines here
    while(NOT_END){
        frpParseComment(lyric,cursor,maxlen);
        if(!NOT_END || FRP_READ == '[')
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

    return 0;

ERROR:
    //Todo release all memory plsease
    if(currentValue != NULL)
        frpfree(currentValue);//这里的currentValue并不一定是完整的初始化过的currentValue，直接free即可
    flyc_release_lines(seg);
    return -1;

#undef WARRING
#undef NOT_END
#undef FRP_READ
#undef FRP_MOVE_NEXT
}
//end of flyc parser

//begin of curve seg parser

void frp_curve_exp_progress_property(FRCExpress * exp,FRFlyc * flyc,const frp_uint8 * textpool){
    switch (exp->type) {
    case FRCE_TYPE_FUNC:
        for(frp_size i = 0;i<exp->func.argc;i++)
            frp_curve_exp_progress_property(exp->func.argv + i,flyc,textpool);
        break;
    case FRCE_TYPE_CURVE:
        for(frp_size i = 0;i<exp->curveexp.curveline->argc;i++)
            frp_curve_exp_progress_property(exp->curveexp.argv + i,flyc,textpool);
        break;
    case FRCE_TYPE_PROPERTY_NAME:
        //progress here
    {
        frp_size i;
        for(i = 0;i<flyc->value_count;i++){
            if(frpstr_rcmp(textpool,flyc->value_names[i],exp->propname) == 0){
                frpfree(exp->propname);
                exp->propid = i;
                exp->type = FRCE_TYPE_PROPERTY;
                break;
            }
        }
        if(i == flyc->value_count){
            frpWarringCompileMessage(exp->propname,(frp_size)(-1),0,"property not found in [flyc] segment.");
            exp->type = FRCE_TYPE_CONST;
            exp->con = 0;
            frpfree(exp->propname);
            //property name not found.
        }
    }
    }
}
//please call this after flyc segment
void frp_curve_progress_property(FRCurve * curve,FRFlyc * flyc,const frp_uint8 * textpool){
    for(FRCurveLine *lines = curve->lines;lines;lines = lines->next){
        frp_curve_exp_progress_property(lines->express,flyc,textpool);
    }
}

float frp_curve_expresult_calculate(FRCExpress * express,float * args,FRPValue * values){
    float narg[FRCE_MAX_ARG_COUNT];

    switch (express->type) {
    case FRCE_TYPE_CONST:
        return express->con;
    case FRCE_TYPE_FUNC:
        for(frp_size i = 0;i<express->func.argc;i++){
            narg[i] = frp_curve_expresult_calculate(express->func.argv + i,args,values);
        }
        return express->func.fp(narg);
    case FRCE_TYPE_ARGM:
        return args[express->argid];
    case FRCE_TYPE_CURVE:
        for(frp_size i=0;i<express->curveexp.curveline->argc;i++)
            narg[i] = frp_curve_expresult_calculate(express->curveexp.argv + i,args,values);
        return frp_curve_expresult_calculate(express->curveexp.curveline->express,narg,values);
    case FRCE_TYPE_PROPERTY_NAME:
        //never do this
        return 0;
    case FRCE_TYPE_PROPERTY:
        if(values == NULL)
            return 0;//no property offered
        switch (values[express->propid].type) {
        case FRPVALUE_TYPE_INT:
            return values[express->propid].integer;
        case FRPVALUE_TYPE_NUM:
            return values[express->propid].num;
        case FRPVALUE_TYPE_TIM:
            return values[express->argid].time;
        default:
            //invalid type.
            return 0;
        }
    }
    //unknown expression
    return 0;
}

//this will free express and its relation express
void frp_curveexp_free(FRCExpress * express,frp_size len){
    for(frp_size i = 0;i<len;i++){
        switch (express->type) {
        case FRCE_TYPE_FUNC:
            frp_curveexp_free(express->func.argv,express->func.argc);
            break;
        case FRCE_TYPE_CURVE:
            frp_curveexp_free(express->curveexp.argv,express->curveexp.curveline->argc);\
            break;
        }
    }
    frpfree(express);
}
//this will free line itself(not the next)
void frp_curveline_free(FRCurveLine * line){
    frp_curveexp_free(line->express,1);
    frpfree(line);
}
int frp_curve_seg_parser(PARSE_SEG_ARGS){
#define NOT_END (*cursor < maxlen)
#define FRP_READ (lyric[*cursor])
#define FRP_MOVE_NEXT() do{if(NOT_END)(*cursor)++;}while(0)

#define WARRING(str) frpWarringMessage(lyric,*cursor,maxlen,str)
#define SKIPHALF() do{if(FRP_READ == '\r')FRP_MOVE_NEXT();if(FRP_READ == '\n')FRP_MOVE_NEXT();}while(0)
#define SKIP() do{if(!frp_in_str(FRP_READ,"\r\n")) while(NOT_END && !frp_in_str(FRP_READ,"\r\n"))(*cursor)++;SKIPHALF();}while(0)

    const frp_uint8 * lyric = file->textpool;

    FRCurveLine * lineend = NULL;
    frp_bison_curves_tobeused = NULL;

    //global used
    frp_bison_arg_source = FRP_BISON_ARG_SOURCE_TEXTPOOL;
    frp_flex_textpool = file->textpool;
    while(NOT_END){
        frpParseComment(lyric,cursor,maxlen);
        if(FRP_READ == '[')
            break;

        frp_str name = frpParseString(lyric,cursor,maxlen,"(:\r\n");
        if(name.len == 0){
            WARRING("curve name is need.line skip.");
            goto SKIP_THIS_LINE;
        }
        if(frp_in_str(FRP_READ,"\r\n")){
            WARRING("body is needed.line skip.");
            goto SKIP_THIS_LINE;
        }
        frp_bison_arg_listcount = 0;
        if(FRP_READ == '('){
            FRP_MOVE_NEXT();
            //arg list
            while(NOT_END && frp_bison_arg_listcount < FRCE_MAX_ARG_COUNT && !frp_in_str(FRP_READ,")\r\n")){
                frp_bison_arg_names[frp_bison_arg_listcount] = frpParseString(lyric,cursor,maxlen,",)\r\n");
                if(frp_bison_arg_names[frp_bison_arg_listcount].len == 0){
                    WARRING("var name should not empty.");
                    goto SKIP_THIS_LINE;
                }
                frp_bison_arg_listcount++;

                if(frp_in_str(FRP_READ,")\r\n")){
                    break;
                }else{//','
                    FRP_MOVE_NEXT();
                }
            }
            if(!frp_in_str(FRP_READ,")\r\n")){
                WARRING("Too many arguments here.line skip");
                goto SKIP_THIS_LINE;
            }
            if(FRP_READ == ')'){
                FRP_MOVE_NEXT();
            }else{
                //if(frp_in_str(FRP_READ,"\r\n") || !NOT_END){
                WARRING("argument list should be closed.line skip");
                goto SKIP_THIS_LINE;
            }
        }
        if(FRP_READ != ':'){
            WARRING("body is needed.line skip.");
            goto SKIP_THIS_LINE;
        }
        FRP_MOVE_NEXT();
        //turn to bison and parse now.

        //global used
        frp_str expstr = frpParseString(lyric,cursor,maxlen,"\r\n");

        //frp_yyflex();

        //        frp_bison_task = FRP_BISON_TASK_PRINT;
        //        frp_flex_textpoolstr = expstr;
        //        frp_bisonparse();


        frp_bison_task = FRP_BISON_TASK_CHECK;
        frp_flex_textpoolstr = expstr;
        if(frp_bisonparse()){
            //failed
            frpWarringMessage(lyric,*cursor,maxlen,frp_bison_errormsg);
            goto SKIP_THIS_LINE;
        }else{
            //grammer is ok,calculate expression now
            frp_bison_task = FRP_BISON_TASK_CALC;
            frp_flex_textpoolstr = expstr;
            frp_bisonparse();
            if(frp_bison_result == NULL){
                frpWarringMessage(lyric,*cursor,maxlen,"unknown error.");
                goto ERROR;
            }
            //save result
            FRCurveLine * line = frpmalloc(sizeof(FRCurveLine));
            if(line == NULL){
                frpErrorMessage(lyric,*cursor,maxlen,"no more memory.");
                goto ERROR;
            }
            line->argc = frp_bison_arg_listcount;
            line->express = frp_bison_result;
            line->curvname = name;
            line->next = NULL;
            if(frp_bison_curves_tobeused == NULL){
                frp_bison_curves_tobeused = lineend = line;
            }else{
                lineend->next = line;
                lineend = line;
            }
        }
        SKIPHALF();


        continue;

SKIP_THIS_LINE:
        SKIP();
    }
    seg->curve.lines = frp_bison_curves_tobeused;
    return 0;
ERROR:
    //free lines
    while(frp_bison_curves_tobeused){
        FRCurveLine * nline = frp_bison_curves_tobeused->next;
        frp_curveline_free(frp_bison_curves_tobeused);
        frp_bison_curves_tobeused = nline;
    }
    return -1;
#undef WARRING
#undef NOT_END
#undef FRP_READ
#undef FRP_MOVE_NEXT
}
//end of curve seg parser
//begin of anim seg parser
//add prop to target anim line
void frp_anim_seg_addprop(const frp_uint8 * textpool,FRAnim *anim,frp_str animname,FRAProp * prop){
    FRALine * line;
    if(anim->lines == NULL){
        line = anim->lines = frpmalloc(sizeof(FRALine));
        line->next = NULL;
        line->prop = NULL;
        line->name = animname;
    }else{
        line = anim->lines;
        while(line->next && frpstr_cmp(textpool,animname,line->name) != 0){
            line = line->next;
        }
        if(frpstr_cmp(textpool,animname,line->name) != 0){
            line->next = frpmalloc(sizeof(FRALine));
            line = line->next;
            line->next = NULL;
            line->prop = NULL;
            line->name = animname;
        }else{
            //line is finded
        }
    }

    if(line->prop == NULL)
        line->prop = prop;
    else{
        FRAProp * tail = line->prop;
        while(tail->next)
            tail = tail->next;
        tail->next = prop;
    }
    prop->next = NULL;

}
int frp_anim_seg_parser(PARSE_SEG_ARGS){
#define NOT_END (*cursor < maxlen)
#define FRP_READ (lyric[*cursor])
#define FRP_MOVE_NEXT() do{if(NOT_END)(*cursor)++;}while(0)

#define WARRING(str) frpWarringMessage(lyric,*cursor,maxlen,str)
#define SKIPHALF() do{if(FRP_READ == '\r')FRP_MOVE_NEXT();if(FRP_READ == '\n')FRP_MOVE_NEXT();}while(0)
#define SKIP() do{if(!frp_in_str(FRP_READ,"\r\n")) while(NOT_END && !frp_in_str(FRP_READ,"\r\n"))(*cursor)++;SKIPHALF();}while(0)

    const frp_uint8 * lyric = file->textpool;

    frpParseComment(lyric,cursor,maxlen);

#define COL_UNKNOWN       0
#define COL_NAME        1
#define COL_PROPERTY    2
#define COL_FUNC        3
#define COL_DURING      4
#define COL_OFFSET      5
#define COL_PLAYTIME    6

#define COL_USEDCOUNT 7

#define COL_MAX 40
    frp_bool colgot[COL_USEDCOUNT];

    frp_size colmap[COL_MAX];

    for(int i =0;i<COL_USEDCOUNT;i++)
        colgot[i] = 0;
    for(int i=0;i<COL_MAX;i++){
        colmap[0] = COL_MAX;
    }

    if(!NOT_END || FRP_READ == '['){
        frpWarringMessage(lyric,*cursor,maxlen,"anim segment is empty.");
        return 0;
    }
    frp_size col_count = 0;
    while(NOT_END){
        if(col_count == COL_MAX){
            frpErrorMessage(lyric,*cursor,maxlen,"too many property to be defined.");
            return -1;
        }
        frp_str lname = frpParseString(lyric,cursor,maxlen,",\r\n");
        if(frpstr_rcmp(lyric,lname,ANSI2UTF8("Name")) == 0){
            colmap[col_count] = COL_NAME;
        }else if(frpstr_rcmp(lyric,lname,ANSI2UTF8("Property")) == 0){
            colmap[col_count] = COL_PROPERTY;
        }else if(frpstr_rcmp(lyric,lname,ANSI2UTF8("Func")) == 0){
            colmap[col_count] = COL_FUNC;
        }else if(frpstr_rcmp(lyric,lname,ANSI2UTF8("During")) == 0){
            colmap[col_count] = COL_DURING;
        }else if(frpstr_rcmp(lyric,lname,ANSI2UTF8("Offset")) == 0){
            colmap[col_count] = COL_OFFSET;
        }else if(frpstr_rcmp(lyric,lname,ANSI2UTF8("PlayTime")) == 0){
            colmap[col_count] = COL_PLAYTIME;
        }else{
            colmap[col_count] = COL_UNKNOWN;
        }
        colgot[colmap[col_count]] = 1;
        col_count++;
        if(FRP_READ == ',')
            FRP_MOVE_NEXT();
        if(frp_in_str(FRP_READ,"\r\n")){
            SKIPHALF();
            break;
        }
    }
    //check property
    if(!colgot[COL_NAME])      { frpErrorMessage(lyric,*cursor,maxlen,"Property [Name] is need.");return -1; }
    if(!colgot[COL_PROPERTY])  { frpErrorMessage(lyric,*cursor,maxlen,"Property [Property] is need.");return -1; }
    if(!colgot[COL_FUNC])      { frpErrorMessage(lyric,*cursor,maxlen,"Property [Func] is need.");return -1; }
    if(!colgot[COL_DURING])    { frpErrorMessage(lyric,*cursor,maxlen,"Property [During] is need.");return -1; }
    if(!colgot[COL_OFFSET])    { frpErrorMessage(lyric,*cursor,maxlen,"Property [Offset] is need.");return -1; }
    if(!colgot[COL_PLAYTIME])  { frpErrorMessage(lyric,*cursor,maxlen,"Property [PlayTime] is need.");return -1; }
    //memory of last line
    frp_str linetext[COL_MAX];
    float lastoffset = 0;
    int lastplaytime = 0;
    for(int i=0;i<COL_MAX;i++)
        linetext[i].len = 0;

    //ready for bison parse
    frp_flex_textpool = lyric;
    frp_str col_func_str = {0,0},col_during_str = {0,0};

    while(NOT_END){
        frpParseComment(lyric,cursor,maxlen);
        if(!NOT_END || FRP_READ == '[')
            break;
#define EXTEND_LASTLINE (!NOT_END || frp_in_str(FRP_READ,"\r\n"))
        //read a line
        frp_str newname = {0,0};
        FRAProp *prop = frpmalloc(sizeof(FRAProp));

        frp_bison_arg_source = FRP_BISON_ARG_SOURCE_RAWSTR;
        frp_bison_task = FRP_BISON_TASK_CHECK;

        for(frp_size col = 0;col < col_count;col++){
            switch (colmap[col])
            {
            case COL_NAME:
                if(!EXTEND_LASTLINE)
                    linetext[col] = frpParseString(lyric,cursor,maxlen,",\r\n");
                if(linetext[col].len == 0){
                    frpWarringMessage(lyric,*cursor,maxlen,"Name should not be empty.Skip line.");
                    goto skip_line;
                }
                newname = linetext[col];
                break;
            case COL_PROPERTY:
                if(!EXTEND_LASTLINE)
                    linetext[col] = frpParseString(lyric,cursor,maxlen,",\r\n");

                prop->property_name = linetext[col];//TODO future:change to property_id
                break;
            case COL_FUNC:
                if(!EXTEND_LASTLINE)
                    linetext[col] = frpParseClosedString(lyric,cursor,maxlen,",\r\n","\r\n");
                col_func_str = linetext[col];
                //TODO:check error of expression
                frpAnimFuncArgInit();
                frp_flex_textpoolstr = col_func_str;
                if(frp_bisonparse()){
                    //failed
                    frpWarringMessage(lyric,*cursor,maxlen,frp_bison_errormsg);
                    goto skip_line;
                }
                break;
            case COL_DURING:
                if(!EXTEND_LASTLINE)
                    linetext[col] = frpParseClosedString(lyric,cursor,maxlen,",\r\n","\r\n");

                col_during_str = linetext[col];
                //TODO:check error of expression
                frp_bison_arg_listcount = 0;
                frp_flex_textpoolstr = col_during_str;
                if(frp_bisonparse()){
                    //failed
                    frpWarringMessage(lyric,*cursor,maxlen,frp_bison_errormsg);
                    goto skip_line;
                }
                break;
            case COL_OFFSET:
                if(!EXTEND_LASTLINE)
                    lastoffset = frpParseNum(lyric,cursor,maxlen);
                prop->offset = lastoffset;
                break;
            case COL_PLAYTIME:
                if(!EXTEND_LASTLINE){
                    frp_str s = frpParseString(lyric,cursor,maxlen,",\r\n");
                    if(frpstr_rcmp(lyric,s,ANSI2UTF8("Start")) == 0){
                        lastplaytime = FRAP_PLAYTIME_START;
                    }else if(frpstr_rcmp(lyric,s,ANSI2UTF8("End"))==0){
                        lastplaytime = FRAP_PLAYTIME_END;
                    }else{
                        frpWarringMessage(lyric,*cursor,maxlen,"Playtime only support [Start] and [End].Skip line.");
                        goto skip_line;
                    }
                }
                prop->play_time = lastplaytime;
                break;
            default:
                if(!EXTEND_LASTLINE)
                    frpParseClosedString(lyric,cursor,maxlen,",\r\n","\r\n");
                break;
            }
            if(FRP_READ == ',')
                FRP_MOVE_NEXT();
        }
#undef EXTENT_LASTLINE
        
        //read express to pool.
        frp_bison_task = FRP_BISON_TASK_CALC;

        frpAnimFuncArgInit();
        frp_flex_textpoolstr = col_func_str;
        frp_bisonparse();
        prop->func_exp = frp_bison_result;
        frp_bison_arg_listcount = 0;
        frp_flex_textpoolstr = col_during_str;
        frp_bisonparse();
        prop->during_exp = frp_bison_result;

        frp_anim_seg_addprop(lyric,&seg->anim,newname,prop);

        continue;
skip_line:
        frpfree(prop);
        SKIP();
    }
    return 0;
#undef WARRING
#undef NOT_END
#undef FRP_READ
#undef FRP_MOVE_NEXT
}

void frp_anim_prop_free(FRAProp * prop){
    frp_curveexp_free(prop->func_exp,1);
    frp_curveexp_free(prop->during_exp,1);
    frpfree(prop);
}

//call this to update property id in anim expression
void frp_anim_parse_express(FRAnim *anim,FRFlyc * flyc,const frp_uint8 * textpool,frp_size maxlen){
    
    for(FRALine * line = anim->lines;line;line = line -> next){
        //ensuore line->prop exist
        while(line->prop){
            frp_size pid;
            for(pid = 0;pid < flyc->value_count;pid++){
                if(frpstr_cmp(textpool,flyc->value_names[pid],line->prop->property_name) == 0){
                    break;
                }
            }
            if(pid == flyc->value_count){
                FRAProp * n = line->prop->next;
                frpWarringMessage(textpool,line->prop->property_name.beg,maxlen,"Property is not found in [flyc].");
                frp_anim_prop_free(line->prop);
                line->prop = n;
            }else{
                //line->prop is ok
                line->prop->property_id = pid;
                break;
            }
        }

        for(FRAProp * prop = line->prop;prop;prop = prop->next){
            //ensure prop->next exist
            while(prop->next){
                frp_size pid;
                for(pid = 0;pid < flyc->value_count;pid++){
                    if(frpstr_cmp(textpool,flyc->value_names[pid],prop->next->property_name) == 0){
                        break;
                    }
                }
                if(pid == flyc->value_count){
                    FRAProp * n = prop->next->next;
                    frpWarringMessage(textpool,prop->next->property_name.beg,maxlen,"Property is not found in [flyc].");
                    frp_anim_prop_free(prop->next);
                    prop->next = n;
                }else{
                    //prop->next is ok
                    prop->next->property_id = pid;
                    break;
                }
            }
            if(flyc){
                frp_curve_exp_progress_property(prop->func_exp,flyc,textpool);
                frp_curve_exp_progress_property(prop->during_exp,flyc,textpool);
            }

        }
    }

    
}


//end of anim seg parser
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
                FRP_MOVE_NEXT();
                while(NOT_END && frp_in_str(FRP_READ,"\t "))
                    FRP_MOVE_NEXT();
                if(!frp_in_str(FRP_READ,"\r\n")){
                    frpErrorMessage(lyric,cursor,maxlength,"return is needed after segment name.");
                    goto COMPILE_ERROR;
                }
                FRP_MOVE_NEXT();
                if(frpParseSeg(file,tempstr,&cursor,maxlength) < 0){
                    goto COMPILE_ERROR;
                }
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
        default:
            frpErrorMessage(lyric,cursor,maxlength,"segment should be declared here.");
            goto COMPILE_ERROR;
        }
    }
#undef NOT_END
#undef FRP_READ
#undef FRP_MOVE_NEXT

    //progress [curve] property expression of [flyc]
    FRPSeg * curveseg = frp_getseg(file,ANSI2UTF8("curve"));
    FRPSeg * flycseg = frp_getseg(file,ANSI2UTF8("flyc"));
    FRPSeg * animseg = frp_getseg(file,ANSI2UTF8("anim"));
    if(curveseg && flycseg){
        frp_curve_progress_property(&curveseg->curve,&flycseg->flyc,file->textpool);
    }
    if(animseg){
        if(flycseg)
            frp_anim_parse_express(&animseg->anim,&flycseg->flyc,file->textpool,maxlength);
        else
            frp_anim_parse_express(&animseg->anim,NULL,file->textpool,maxlength);
    }

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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "fparser.h"
//this file is for debug only
#define PERFORMANCE_COUNT_SEC 10

#define NORMAL_TEST
//#define MEMORY_LEAK_TEST
//#define TIMELINE_TEST
frp_uint8 buff[4096];
float frp_curve_expresult_calculate(FRCExpress * express,float * args);
void printString(frp_str str){
    for(unsigned int i=str.beg,j=0;j<str.len;i++,j++)
        putchar(buff[i]);
}

void printFlycPValue(FRPValue * value){
    switch (value->type) {
    case FRPVALUE_TYPE_EXT:
        printf("ext");
        break;
    case FRPVALUE_TYPE_INT:
        printf("int[%d]",value->integer);
        break;
    case FRPVALUE_TYPE_NUM:
        printf("num[%f]",value->num);
        for(FRLanim * anim = value->anim_apply;anim;anim = anim->next){
            printf("-[%d~%lld->%lld]",anim->animprop->property_id,anim->starttime,anim->endtime);
        }
        break;
    case FRPVALUE_TYPE_STR:
        printf("str[");
        printString(value->str);
        printf("]");
        break;
    case FRPVALUE_TYPE_TIM:
        printf("time[%lld]",value->time);
        break;
    }
}
void printFlycNode(FRPNode * node,FRPLine * line){
    if(node == NULL)
        printf("--->empty node\n");
    else{
        for(unsigned int i=0;i<line->seg->flyc.value_count;i++){
            printf("|");
            printString(line->seg->flyc.value_names[i]);
            printf(":");
            printFlycPValue(node->values + i);
        }
        printf("\n");
        printFlycNode(node->next,line);
    }
}
void printFlycLine(FRPLine *line){
    if(line == NULL)
        printf("-->empty line\n");
    else{
        printf("line,beg = %llu,end = %llu\n",line->starttime,line->endtime);
        for(unsigned int i=0;i<line->seg->flyc.value_count;i++){
            printf("|");
            printString(line->seg->flyc.value_names[i]);
            printf(":");
            printFlycPValue(line->values + i);

        }
        printf("\n");
        printf("-->node:\n");
        printFlycNode(line->node,line);
        printFlycLine(line->next);
    }
}

void printFlycSeg(FRPSeg * seg){
    printf("->flyc seg name:");
    printString(seg->segname);
    printf("\n->print lines:\n");
    printFlycLine(seg->flyc.lines);
}

void printFile(FRPFile * file){
    printf("File info:seg count = %d\n",file->segcount);
    for(unsigned int i=0;i<file->segcount;i++){
        if(frpstr_rcmp(file->textpool,file->segs[i].segname,(const unsigned char *)"flyc") == 0){
            printFlycSeg(file->segs + i);
        }
    }
}

int main(int argc,char **argv)
{
    //windows only(change cmd to utf-8)
    //system("@chcp 65001 > nul");

    printf("GO");

    if(argc != 2){
        printf("file name is need.\n");
        return 0;
    }


#ifdef NORMAL_TEST

    FILE * f = fopen(argv[1],"rb");
    //这里要用二进制读入，传入必须是utf-8编码的文件
    fread(buff,sizeof(frp_uint8),4096,f);

    frpinit();
    frp_anim_add_support("ColorR");
    printf("ready to parse.\n");
    FRPFile * file = frpopen(buff,sizeof(buff),1);
    printf("parse over.\n");

    time_t begtim = time(NULL);
    long long tick = 0;

    if(file){
        printFile(file);

        FRPSeg * seg = NULL;
        for(frp_size i=0;i<file->segcount;i++){
            if(frpstr_rcmp(file->textpool,file->segs[i].segname,"curve") == 0){
                seg = file->segs + i;
            }
        }
        if(seg){

            //while(time(NULL) - begtim < PERFORMANCE_COUNT_SEC)
            {
                FRCurveLine * line = seg->curve.lines;
                while(line){
                    frpstr_fill(file->textpool,line->curvname,buff,sizeof(buff));
                    if(line->argc == 0){
                        //注释printf才能测出性能，公式性能完全能满足运算要求
                        printf("calc [%s]",buff);
                        float result = frp_curve_expresult_calculate(line->express,NULL);
                        printf("=>%f.\n",result);
                        tick++;
                    }else{
                        //printf("skip [%s] has %d argument.\n",buff,line->argc);
                    }
                    line = line->next;
                }
            }
            printf("calculate %lld in %d sec.\n",tick,PERFORMANCE_COUNT_SEC);
        }else{
            printf("no curve segment found.\n");
        }

        //debug color r property

        frp_time time = 0;
        frp_size color_r_property_id = frp_play_get_property_id(file,"ColorR");
        //hack for the seconed line's first node's values
        FRPValue * vals = frp_getseg(file,"flyc")->flyc.lines->next->node->values;
        for(time = 2000;time < 3000;time += 50){
            float value = frp_play_property_float_value(time,vals,color_r_property_id);
            printf("[%6llu:%3.5f]\n",time,value);
        }



    }else {
        printf("file is not open.");
    }
#endif

#ifdef MEMORY_LEAK_TEST

    FILE * f = fopen(argv[1],"rb");
    fread(buff,sizeof(frp_uint8),4096,f);
    fclose(f);

    frpinit();
    frp_anim_add_support("ColorR");
    printf("ready to parse.\n");

    printf("test no copy.\n");

    for(int i = 0;i<1;i++){
        printf("before read loop %d.\n",i);
        FRPFile * f = frpopen(buff,4096,1);
        frpdestroy(f);
        printf("end read loop %d.\n",i);
    }


    printf("test copy.\n");
    for(int i = 0;i<0;i++){
        printf("before read loop %d.\n",i);
        FRPFile * f = frpopen(buff,4096,0);
        frpdestroy(f);
        printf("end read loop %d.\n",i);
    }

    frpshutdown();
#endif

#ifdef TIMELINE_TEST
    FILE * f = fopen(argv[1],"rb");
    fread(buff,sizeof(frp_uint8),4096,f);
    fclose(f);

    frpinit();
    frp_anim_add_support("ColorR");
    printf("ready to parse.\n");

    FRPFile * file = frpopen(buff,4096,1);

    FRTNode * node = NULL;
    frp_time curtime = 0;
    //node = frp_play_getline_by_time(file,curtime);

    frp_size pid = frp_play_get_property_id(file,"Text");

    do{
        node = frp_play_getline_by_time(file,curtime);
        char ch[2048];
        printf("playing at %lld:\n",curtime);
        while(node){
            frp_play_fill_line_text(file,node->line,pid,ch,2018);
            printf("lyric:[%s]\n",ch);
            node = node->next;
        }
        curtime = frp_play_next_switchline_time(file,curtime);
    }while(frp_play_has_more_line(file));

#endif
    return 0;
}


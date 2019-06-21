#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "fparser.h"
//this file is for debug only
#define PERFORMANCE_COUNT_SEC 10
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

    if(argc != 2){
        printf("file name is need.\n");
        return 0;
    }

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
        for(int i=0;i<file->segcount;i++){
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
            printf("calculate %lld in %d sec.",tick,PERFORMANCE_COUNT_SEC);
        }else{
            printf("no curve segment found.\n");
        }
    }else {
        printf("file is not open.");
    }
    return 0;
}


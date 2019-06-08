#include <stdio.h>
#include <stdlib.h>

#include "fparser.h"
//this file is for debug only

frp_uint8 buff[4096];

void printString(frp_str str){
    for(unsigned int i=str.beg,j=0;j<str.len;i++,j++)
        putchar(buff[i]);
}

void printFlycPValue(FRPValue * value){
    switch (value->type) {
    case FRPVALUE_TYPE_EXT:
        printf("ext expression");
        break;
    case FRPVALUE_TYPE_INT:
        printf("int expression = %d",value->integer);
        break;
    case FRPVALUE_TYPE_NUM:
        printf("num expression = %f",value->num);
        break;
    case FRPVALUE_TYPE_STR:
        printf("str expression = ");
        printString(value->str);
        break;
    case FRPVALUE_TYPE_TIM:
        printf("time expression = %lld",value->time);
        break;
    }
}
void printFlycNode(FRPNode * node,FRPLine * line){
    if(node == NULL)
        printf("--->empty node\n");
    else{
        for(unsigned int i=0;i<line->seg->flyc.value_count;i++){
            printf("--->");
            printString(line->seg->flyc.value_names[i]);
            printf(":");
            printFlycPValue(node->values + i);
            printf("\n");
        }
        printFlycNode(node->next,line);
    }
}
void printFlycLine(FRPLine *line){
    if(line == NULL)
        printf("-->empty line\n");
    else{
        for(unsigned int i=0;i<line->seg->flyc.value_count;i++){
            printf("-->");
            printString(line->seg->flyc.value_names[i]);
            printf(":");
            printFlycPValue(line->values + i);
            printf("\n");
        }
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
    printf("ready to parse.\n");
    FRPFile * file = frpopen(buff,4096,1);


    if(file){
        printFile(file);
    }else {
        printf("file is not open.");
    }
    return 0;
}

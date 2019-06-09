%{
#include<stdio.h>
#include "fparser.h"
#include "frp_bison.tab.h"

const frp_uint8 * frp_flex_textpool;
frp_str frp_flex_textpoolstr;

#define YY_INPUT(buf,result,max_size) \
    {\
        result = frp_flex_textpoolstr.len ? \
            (*buf = frp_flex_textpool[frp_flex_textpoolstr.beg++],frp_flex_textpoolstr.len--,1):YY_NULL;\
    }
/*
#define YY_INPUT(buf,result,max_size) \
    {\
        frp_size sz = frp_flex_textpoolstr.len < (frp_size)max_size ? frp_flex_textpoolstr.len : max_size;\
        for(frp_size i = 0;i<sz;i++)\
            buf[i]=frp_flex_textpool[frp_flex_textpoolstr.beg++];\
        frp_flex_textpoolstr.len -= sz;\
        result = sz ? sz : YY_NULL;\
    }
*/
%}
OPERATOR        [-+*/()=,:]
NUMBER          (\.[0-9]+)|([0-9]*\.?[0-9]*)
WHITSPACE       [ \t]*
WORD            [^-+*/()=,:0-9 \t\r\n]+
%%
{OPERATOR}      {
    //printf("[%c]",*yytext);
    return *yytext;
}
{NUMBER}   {
    //printf("[int:%s]",yytext);
    int beg = 0;
    float v = 0,b = 10;
    while(yytext[beg]&&yytext[beg] != '.'){
        v = v * 10 + (yytext[beg++] - '0');
    }
    if(yytext[beg] == '.'){
        beg++;
        while(yytext[beg]){
            v += (yytext[beg++] - '0')/b;
            b *= 10;
        }
    }
    frp_bisonlval.num = v;
    return T_NUM;
}
{WHITSPACE} {
    /* ignore */
}
{WORD}      {
    //printf("[word:%s]",yytext);
    frp_size len = 0;
    while(yytext[len])
        len++;
    len++;
    frp_bisonlval.temptext = frpmalloc(sizeof(*yytext) * len); //need to free here
    while(len--)
        frp_bisonlval.temptext[len] = yytext[len];
    return T_WORD;
}
%%

//int frp_yy_eos;

int yywrap(){
    //frp_yy_eos = 1;
    return 1;
}
/*
int frp_yyflex(){
    frp_yy_eos = 0;
    while(!frp_yy_eos){
        frp_bisonlex();
    }
}
*/

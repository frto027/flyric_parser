%{
#include "fparser.h"
#include "fparser_platform.h"

#include <stdio.h>

#define YYMALLOC frpmalloc
#define YYFREE frpfree

int frp_bison_arg_source;
frp_size frp_bison_arg_listcount;
frp_str frp_bison_arg_names[FRCE_MAX_ARG_COUNT];
FRCurveLine *frp_bison_curves_tobeused;
const char * frp_bison_arg_names_raw[FRCE_MAX_ARG_COUNT];

const char * frp_bison_errormsg;
char frp_bison_errormsgbuff[1024];
void frp_bisonerror(const char * msg){
    frp_bison_errormsg = msg;
}

int frp_bisonlex(void);

FRCExpress * frp_bison_result;
int frp_bison_task;

typedef struct FRCExpressNodes{
    FRCExpress express;
    struct FRCExpressNodes * next;
    frp_size len;
}FRCExpressNodes;
#define ANSI2UTF8(s) ((const frp_uint8 *)(s))

void frp_bison_report_error_varstr(const char * stra,const char * strb){
    frp_bison_errormsg = frp_bison_errormsgbuff;
    int mx = sizeof(frp_bison_errormsgbuff) / sizeof(*frp_bison_errormsgbuff) - 1;
    char * b = frp_bison_errormsgbuff;
    while(mx-- && *stra){
        *b++ = *stra++;
    }
    if(mx){
        while(mx-- && *strb){
            *b++ = *strb++;
        }
    }
    *b = '\0';
    return;
}

FRCExpress * frp_bison_get_express_by_name(const char *fname,FRCExpress * express,frp_size argc){
    //curve first
    for(FRCurveLine * line = frp_bison_curves_tobeused;line;line = line->next){
        if(frpstr_rcmp(frp_flex_textpool,line->curvname,ANSI2UTF8(fname)) == 0 && line->argc == argc){
            FRCExpress * exp = frpmalloc(sizeof(FRCExpress));
            exp->type = FRCE_TYPE_CURVE;
            exp->curveexp.curveline = line;
            exp->curveexp.argv = express;
            return exp;
        }
    }
    //built in then
    for(FRCBuiltinFunctionType * fbuiltin = FRCBuiltinFunctions;fbuiltin->name;fbuiltin++){
        if(frpstr_rrcmp(ANSI2UTF8(fbuiltin->name),ANSI2UTF8(fname)) == 0 && fbuiltin->argc == argc){
            FRCExpress * ret = frpmalloc(sizeof (FRCExpress));
            ret->type = FRCE_TYPE_FUNC;
            ret->func.fp = fbuiltin->fp;
            ret->func.argc = argc;
            ret->func.argv = express;
            return ret;
        }
    }
    return NULL;
}
//then b maybe destroyed because of const value is calculated.
FRCExpress * frp_bison_get_mid_expression(const char mid,FRCExpress * a,FRCExpress * b){
    switch (mid) {
    case '+':
        if(a->type == FRCE_TYPE_CONST && b->type == FRCE_TYPE_CONST){
            a->con = frpCurveExpressAdd(a->con,b->con);
            frpfree(b);
            return a;
        }else{
            FRCExpress * ret = frpmalloc(sizeof(FRCExpress));
            FRCExpress * arg = frpmalloc(2 * sizeof(FRCExpress));
            arg[0] = *a;
            arg[1] = *b;
            frpfree(a);
            frpfree(b);
            ret->type = FRCE_TYPE_FUNC;
            ret->func.fp = frpCurveExpressAddW;
            ret->func.argc = 2;
            ret->func.argv = arg;
            return ret;
        }
    case '-':
        if(a->type == FRCE_TYPE_CONST && b->type == FRCE_TYPE_CONST){
            a->con = frpCurveExpressSub(a->con,b->con);
            frpfree(b);
            return a;
        }else{
            FRCExpress * ret = frpmalloc(sizeof(FRCExpress));
            FRCExpress * arg = frpmalloc(2 * sizeof(FRCExpress));
            arg[0] = *a;
            arg[1] = *b;
            frpfree(a);
            frpfree(b);
            ret->type = FRCE_TYPE_FUNC;
            ret->func.fp = frpCurveExpressSubW;
            ret->func.argc = 2;
            ret->func.argv = arg;
            return ret;
        }
    case '*':
        if(a->type == FRCE_TYPE_CONST && b->type == FRCE_TYPE_CONST){
            a->con = frpCurveExpressMul(a->con,b->con);
            frpfree(b);
            return a;
        }else{
            FRCExpress * ret = frpmalloc(sizeof(FRCExpress));
            FRCExpress * arg = frpmalloc(2 * sizeof(FRCExpress));
            arg[0] = *a;
            arg[1] = *b;
            frpfree(a);
            frpfree(b);
            ret->type = FRCE_TYPE_FUNC;
            ret->func.fp = frpCurveExpressMulW;
            ret->func.argc = 2;
            ret->func.argv = arg;
            return ret;
        }
    case '/':
        if(a->type == FRCE_TYPE_CONST && b->type == FRCE_TYPE_CONST){
            a->con = frpCurveExpressDiv(a->con,b->con);
            frpfree(b);
            return a;
        }else{
            FRCExpress * ret = frpmalloc(sizeof(FRCExpress));
            FRCExpress * arg = frpmalloc(2 * sizeof(FRCExpress));
            arg[0] = *a;
            arg[1] = *b;
            frpfree(a);
            frpfree(b);
            ret->type = FRCE_TYPE_FUNC;
            ret->func.fp = frpCurveExpressDivW;
            ret->func.argc = 2;
            ret->func.argv = arg;
            return ret;
        }
    }
    return NULL;
}
FRCExpress * frp_bison_get_arg_express(const char * argname){
    for(frp_size i = 0;i<frp_bison_arg_listcount;i++){
        if((frp_bison_arg_source == FRP_BISON_ARG_SOURCE_TEXTPOOL && frpstr_rcmp(frp_flex_textpool,frp_bison_arg_names[i],ANSI2UTF8(argname)) == 0)
            || (frp_bison_arg_source == FRP_BISON_ARG_SOURCE_RAWSTR && frpstr_rrcmp(ANSI2UTF8(frp_bison_arg_names_raw[i]),ANSI2UTF8(argname)) == 0)){
            FRCExpress * r = frpmalloc(sizeof(FRCExpress));
            r->type = FRCE_TYPE_ARGM;
            r->argid = i;
            return r;
        }
    }
    return NULL;
}
int frp_bison_exist_express_by_name(const char * fname,frp_size argc){
    //Curve first
    for(FRCurveLine * line = frp_bison_curves_tobeused;line;line = line->next){
        if(frpstr_rcmp(frp_flex_textpool,line->curvname,ANSI2UTF8(fname)) == 0 && line->argc == argc){
            return 1;
        }
    }
    //built in then
    for(FRCBuiltinFunctionType * fbuiltin = FRCBuiltinFunctions;fbuiltin->name;fbuiltin++){
        if(frpstr_rrcmp(ANSI2UTF8((fbuiltin->name)),ANSI2UTF8(fname)) == 0 && fbuiltin->argc == argc)
            return 1;
    }
    return 0;
}
int frp_bison_exist_arg_express(const char * argname){
    for(frp_size i = 0;i<frp_bison_arg_listcount;i++){
        if((frp_bison_arg_source == FRP_BISON_ARG_SOURCE_TEXTPOOL && frpstr_rcmp(frp_flex_textpool,frp_bison_arg_names[i],ANSI2UTF8(argname)) == 0)
            || (frp_bison_arg_source == FRP_BISON_ARG_SOURCE_RAWSTR && frpstr_rrcmp(ANSI2UTF8(frp_bison_arg_names_raw[i]),ANSI2UTF8(argname)) == 0)){
            return 1;
        }
    }
    return 0;
}

%}

%token T_NUM
%token T_WORD

%destructor { printf("free[%s]",$$.temptext); frpfree($$.temptext); } T_WORD

%left '+' '-'
%left '*' '/'
%left ','

%%
S   :   E   {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[END]\n");
        break;
    case FRP_BISON_TASK_CALC:
        frp_bison_result = $1.express;
        break;
    }
    YYACCEPT;
}
    ;

LST :   LST ',' LST     {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[list]");
        break;
    case FRP_BISON_TASK_CHECK:
        $$.list_count_check = $1.list_count_check + $3.list_count_check;
        break;
    case FRP_BISON_TASK_CALC:
        $$.listnodes = $1.listnodes;
        while($1.listnodes->next != NULL)
            $1.listnodes = $1.listnodes->next;
        $1.listnodes->next = $3.listnodes;
        $$.listnodes->len += $3.listnodes->len;
        break;
    }
}
    |   E               {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[list_elem]");
        break;
    case FRP_BISON_TASK_CHECK:
        $$.list_count_check = 1;
        break;
    case FRP_BISON_TASK_CALC:
        $$.listnodes = frpmalloc(sizeof(FRCExpressNodes));
        $$.listnodes->express = *($1.express);
        frpfree($1.express);
        $$.listnodes->next = NULL;
        $$.listnodes->len = 1;
        break;
    }
}
    ;

E   :   E '+' E         {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[+]");
        break;
    case FRP_BISON_TASK_CHECK:

        break;
    case FRP_BISON_TASK_CALC:
        $$.express = frp_bison_get_mid_expression('+',$1.express,$3.express);
        break;
    }
}
    |   E '-' E         {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[-]");
        break;
    case FRP_BISON_TASK_CHECK:

        break;
    case FRP_BISON_TASK_CALC:
        $$.express = frp_bison_get_mid_expression('-',$1.express,$3.express);
        break;
    }
 }
    |   E '*' E         {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[*]");
        break;
    case FRP_BISON_TASK_CHECK:

        break;
    case FRP_BISON_TASK_CALC:
        $$.express = frp_bison_get_mid_expression('*',$1.express,$3.express);
        break;
    }
}
    |   E '/' E         {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[/]");
        break;
    case FRP_BISON_TASK_CHECK:
        break;
    case FRP_BISON_TASK_CALC:
        $$.express = frp_bison_get_mid_expression('/',$1.express,$3.express);
        break;
    }
}
    |   T_NUM           {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[1]");
        break;
    case FRP_BISON_TASK_CHECK:
        break;
    case FRP_BISON_TASK_CALC:
        $$.express = frpmalloc(sizeof(FRCExpress));
        $$.express->type = FRCE_TYPE_CONST;
        $$.express->con = $1.num;
        break;
    }
}
    |   '(' E ')'       {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[()]");
        break;
    case FRP_BISON_TASK_CHECK:
        break;
    case FRP_BISON_TASK_CALC:
        $$ = $2;
        break;
    }
 }
    |   '-' E {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[-]");
        break;
    case FRP_BISON_TASK_CHECK:
        break;
    case FRP_BISON_TASK_CALC:
        if($2.express->type == FRCE_TYPE_CONST){
            $2.express->con = frpCurveExpressNegW(&($2.express->con));
            $$ = $2;
        }else{
            $$.express->type = FRCE_TYPE_FUNC;
            $$.express->func.argc = 1;
            $$.express->func.fp = frpCurveExpressNegW;
            $$.express->func.argv = $2.express;
        }
        break;
    }
}
    |   T_WORD '(' ')'{

    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[f]");
        frpfree($1.temptext);
        break;
    case FRP_BISON_TASK_CHECK:
        if(!frp_bison_exist_express_by_name($1.temptext,0)){
            frp_bison_report_error_varstr("parse error:function name not declared -> ",$1.temptext);
            frpfree($1.temptext);
            YYABORT;
        }
        frpfree($1.temptext);
        break;
    case FRP_BISON_TASK_CALC:
        $$.express = frp_bison_get_express_by_name($1.temptext,NULL,0);
        frpfree($1.temptext);
        }
        break;
    }
    |   T_WORD '(' LST ')'{
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[f]");
        frpfree($1.temptext);
        break;
    case FRP_BISON_TASK_CHECK:
        if(!frp_bison_exist_express_by_name($1.temptext,$3.list_count_check)){
            frp_bison_report_error_varstr("parse error:function name not declared -> ",$1.temptext);
            frpfree($1.temptext);
            YYABORT;
        }
        frpfree($1.temptext);
        break;
    case FRP_BISON_TASK_CALC:
        //change list elements to array here
        {
        frp_size len = $3.listnodes->len;
        FRCExpress * exps = frpmalloc($3.listnodes->len * sizeof(FRCExpress));
        for(frp_size i = 0;$3.listnodes;i++){
            exps[i] = $3.listnodes->express;
            FRCExpressNodes * n = $3.listnodes->next;
            frpfree($3.listnodes);
            $3.listnodes = n;
        }
        $$.express = frp_bison_get_express_by_name($1.temptext,exps,len);
        frpfree($1.temptext);
        }
        break;
    }
}
    |   '[' T_WORD ']' { //property name here
    //TOTEST
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[prop]");
        frpfree($2.temptext);
        break;
    case FRP_BISON_TASK_CHECK:
        frpfree($2.temptext);
        break;
    case FRP_BISON_TASK_CALC:
    {
        frp_size len = 0;
        while($2.temptext[len++]);
        $$.express = frpmalloc(sizeof(FRCExpress));
        $$.express->type = FRCE_TYPE_PROPERTY_NAME;
        $$.express->propname = $2.temptext;
        //$$.express->propname = frpmalloc(sizeof(frp_uint8) * len);
        //while(len--)
        //    $$.express->propname[len] = $2.temptext[len];
        //frpfree($2.temptext);
    }
        break;
    }
}
    |   T_WORD          {
    switch(frp_bison_task){
    case FRP_BISON_TASK_PRINT:
        printf("[x]");
        frpfree($1.temptext);
        break;
    case FRP_BISON_TASK_CHECK:
        if(!frp_bison_exist_arg_express($1.temptext)){
            frp_bison_report_error_varstr("parse error:argument name not declared -> ",$1.temptext);
            frpfree($1.temptext);
            YYABORT;
        }
        frpfree($1.temptext);
        break;
    case FRP_BISON_TASK_CALC:
        $$.express = frp_bison_get_arg_express($1.temptext);
        frpfree($1.temptext);
        break;
    }
}
    ;
%%



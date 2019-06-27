# flyric歌词解析库

还在开发中。

现在已经可以使用了

# 歌词文件格式
一个歌词的样例见lrc.txt。歌词的详细格式的定义见file_format_define.txt

# 项目文件描述
## 环境依赖
### 词法分析和语法分析
歌词支持自定义动画，其中曲线解析部分的源代码位于`lex.frp_bison.c` `frp_bison.tab.c` `frp_bison.tab.h`，这些文件需要使用`flex`和`bison`来生成。

编译源代码需要安装flex和bison，请在linux系统下生成这些文件。在`fedora 30`下，使用下面的命令是可行的：
```
yum install -y flex bison
```
执行下面的shell语句，你就能生成需要的`.c`文件了。为了开发方便，这些指令也被放入了`makefile`和qt工程的Debug的`Build steps`里面，执行`make clean`会清除这些文件。
```
flex --prefix=frp_bison frp_flex.l
bison -d --name-prefix=frp_bison frp_bison.y
```
### 跨平台移植
`fparser.c/h`仅使用了`fparser_platform.c/h`和`bison`、`flex`生成的文件中的函数，如果希望跨平台，就移植`fparser_platform.c/h`这个文件。

## QT IDE
你可以使用`QT Creator`打开.pro文件。
## Makefile
编写了`makefile`，执行下面的指令来生成源代码并编译所有的`.o`目标文件
```
make
```
程序中有一个`test.c`是拿来测试的，执行下面的指令可以生成测试程序
```
make test.timeline
```
```
make test.memcheck
```
```
make test.normal
```
执行下面的指令来进行内存泄漏的测试检查
```
./memcheck.sh
```

# 使用说明
TODO，参照test.c，文件的打开、关闭操作、对一个文件进行查询操作不支持多线程，对多个文件分别的查询操作可以是多线程的。
```c
#include "frparser_public.h"
//其它内容
......

//缓冲区
frp_uint8 buff[4096];
//打开文件,文件内容以二进制形式读入buff，这样才能手动处理换行符
FILE * f = fopen(argv[1],"rb");
fread(buff,sizeof(frp_uint8),4096,f);
fclose(f);

//下面两行初始化这个库
//初始化fparser
frpstartup();
//添加对ColorR属性的动画支持
frp_anim_add_support("ColorR");

//解析歌词
FRPFile * f = frpopen(buff,4096,1);

//这里进行frp_play等操作，这期间如果frpopen第三个参数为真，就不要修改buff的值。
//实际上buff的值修改后不会影响歌词解析，这期间除了查询FRPVALUE_TYPE_STR类型的属性的具体值以外是不会去访问buff的。
......


//关闭歌词文件
frpdestroy(f);
//释放全部内存
frpshutdown();
```
# 已知但没修的BUG
- 如果解析出现warring，则内存泄漏
- 如果解析出现Error，则内存泄漏
# 其他
下面是一些文件的作用

想要编译这个库，需要这些文件：

文件名|作用
------|------
fparser.c/h|主要文件，所有的程序都在这里
fparser_platform.c/h|平台相关的东西，做移植时使用
lex.frp_bison.c|flex词法分析器生成的词法分析文件
frp_bison.tab.c/h|bison语法分析器生成的语法文件，用于解析表达式
fparser_public.h|对外接口，文件里面包含这个就可以了

此外还有一些文件是用于词法分析器的

文件名|作用
---|---
frp_flex.l|flex词法描述
frp_bison.y|bison语法描述

一个`test.c`用于测试
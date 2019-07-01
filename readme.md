# flyric歌词解析库

还在开发中。

现在已经可以使用了

# 歌词文件格式
一个歌词的样例见lrc.txt。歌词的格式的初步定义见file_format_define.txt

# 项目文件描述
## 编译依赖
### 库依赖
默认的`fparser_platform.c`实现中使用了`math.h`的`sinf`，因此需要加入`-lm`的编译选项。这是唯一的非默认的第三方库依赖。您可以自行替换这个函数以避免依赖，或直接加入这个编译选项即可。
### bison和flex(可选)
歌词支持自定义动画，其中曲线解析部分的源代码位于`lex.frp_bison.c` `frp_bison.tab.c` `frp_bison.tab.h`，这些文件需要使用`flex`和`bison`来生成。

如果不修改这几个文件，那么无需安装flex和bison，所需的这些源代码已经上传到git。

完全编译源代码需要安装flex和bison，请在linux系统下生成这些文件。在`fedora 30`下，使用下面的命令是可行的：
```
yum install -y flex bison
```

### 跨平台移植
`fparser.c/h`仅使用了`fparser_platform.c/h`和`bison`、`flex`生成的文件中的函数，如果希望跨平台，就移植`fparser_platform.c/h`这个文件。

## Makefile
编写了`makefile`，执行下面的指令来生成源代码并编译所有的`.o`目标文件并生成`libfparser.a`静态库(在obj目录)。
```
make
```
程序中有一个`test.c`是拿来测试的，执行下面的指令可以生成测试程序
```
make test
```
或者分别执行
```
make bin/test.timeline
```
```
make bin/test.memcheck
```
```
make bin/test.normal
```
执行下面的指令来进行内存泄漏的测试检查
```
./memcheck.sh
```
除了`make clean`能清除obj文件和其他文件之外，还可以执行`make cleanall`将bison和flex生成的c文件和h文件一并清除，但下次make需要安装bison和flex才能编译了
# 使用说明
## 链接库的使用
直接执行`make`生成的库文件位于obj目录下，另外需要将include目录下的头文件添加到include路径  
假设在这个git的根目录下面构建，不妨使用如下编译选项
```
-I./include -L./obj -lfparser -lm
```
## API
TODO，参照test.c，文件的打开、关闭操作、对一个文件进行查询操作不支持多线程，对多个文件分别的查询操作可以是多线程的。
## 一个例子
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

## 字符串数据类型
为方便支持utf-8编码，程序内字符串是用`frp_uint8 *`，即8位无符号类型。
程序内部除部分`flex`和`bison`解析外全部使用`frp_size`数据类型，其定义如下：
```c
typedef struct{
    unsigned int beg;
    frp_size len;
}frp_str;
```
`frp_str`表示的是传入`frpopen`的字符串的子串，文件内部无修改地保留或者拷贝这个字符串于结构体`FRPFile`的`textpool`，并当作文字池使用。  
`beg`表示字符串在文字池的起始位置。`len`表示字符串的字节长度。字符串全部都**不是**以NULL结尾的。  
另外也提供下面的函数用来做字符串处理：
```c
extern int frpstr_cmp(const frp_uint8 * textpool,const frp_str stra,const frp_str strb);
extern int frpstr_rcmp(const frp_uint8 * textpool,const frp_str stra,const frp_uint8 strb[]);
extern int frpstr_rrcmp(const frp_uint8 stra[],const frp_uint8 strb[]);
extern frp_size frpstr_fill(const frp_uint8 * textpool,const frp_str str,frp_uint8 buff[],frp_size size);
```
其中`frpstr_fill`将字符串填充到buff和size表示的数组中，并会处理转义符号。

# 已知但没修的BUG
- 如果解析出现warring，则可能内存泄漏
- 如果解析出现Error，则可能内存泄漏
- 动画定义时名称不能省略

# 其他
下面是一些文件的作用


文件名|作用
------|------
src/fparser.c/h|主要文件，所有的程序都在这里
src/fparser_platform.c/h|平台相关的东西，做移植时使用
src/lex.frp_bison.c|flex词法分析器生成的词法分析文件
src/frp_bison.tab.c/h|bison语法分析器生成的语法文件，用于解析表达式
src/fparser_public.h|对外接口，文件里面包含这个就可以了

此外还有一些文件是用于词法分析器的

文件名|作用
---|---
srf/bison_flex/frp_flex.l|flex词法描述
src/bison_flex/frp_bison.y|bison语法描述

一个`test.c`用于测试

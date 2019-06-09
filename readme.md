# flyric歌词解析库

还在开发中。

it is not a complete library

暂时不写什么文档了，一个歌词的样例见lrc.txt。歌词的详细个是定义见file_format_define.txt

本项目工程使用qt编辑器，偷懒的话直接用即可。

歌词支持自定义动画，使用`flex`和`bison`解析

你无需安装flex和bison，直接将能看到的.c文件放在一起编译就行了。

如果想要修改动画语法，那需要安装flex和bison，使用这条指令：
```
flex --prefix=frp_bison frp_flex.l
bison -d --name-prefix=frp_bison frp_bison.y
```
即可生成对应的`lex.frp_bison.c` `frp_bison.tab.c` `frp_bison.tab.h`文件。

下面是一些文件的作用

想要编译这个库，需要这些文件：

文件名|作用
------|------
fparser.c/h|主要文件，所有的程序都在这里
fparser_platform.c/h|平台相关的东西，做移植时使用
lex.frp_bison.c|flex词法分析器生成的此法分析文件
frp_bison.tab.c/h|bison语法分析器生成的语法文件，用于解析表达式

此外还有一些文件是用于此法分析器的

文件名|作用
---|---
frp_flex.l|flex词法描述
frp_bison.y|bison语法描述

一个`main.c`用于测试
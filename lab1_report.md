# lab1 report

## Thinking

### 1.1

#### 第一步

创建 `hello.c`如下
![1679298798047](image/lab1_report/1679298798047.png)
执行如下命令
![1679302036963](image/lab1_report/1679302036963.png)
由于两个文件均在700+行，这里截取末尾几行
![1679302247892](image/lab1_report/1679302247892.png)
两个gcc工具预处理器都将头文件内容进行了展开，所使用的stdio.h路径也不相同。前面的内容均是对应的stdio.h中的内容，并没有函数的具体实现，只有声明，但主函数是一致的。

#### 只编译不链接

执行如下命令：

```bash
mips-linux-gnu-gcc -c hello.c -o hello.o
mv hello.o hello_cross_compile.o
gcc -c hello.c -o hello.o
readelf -h  hello.o
mips-linux-gnu-readelf -h hello_cross_compile.o
```

![1679369899424](image/lab1_report/1679369899424.png)

原生工具链架构和交叉汇编架构完全不同，交叉汇编对应的节更小，但是更多
如下，用各自的objdump反汇编之后得到了x86汇编代码和mips代码（或者使用 `readelf -s`参数得到节信息），x86是变长(惊)，mips是定长，这个例子下交叉汇编SECTION包含x86的SECTION

```bash
mips-linux-gnu-objdump -DS hello_cross_compile.o > hello_cross_compile.txt
objdump -DS hello.o > hello.txt
```

在这里贴出 `FUNC main`的结果：

```x86
0000000000000000 <main>:
   0:   f3 0f 1e fa             endbr64
   4:   55                      push   %rbp
   5:   48 89 e5                mov    %rsp,%rbp
   8:   48 8d 05 00 00 00 00    lea    0x0(%rip),%rax        # f <main+0x    f>
   f:   48 89 c7                mov    %rax,%rdi
  12:   e8 00 00 00 00          call   17 <main+0x17>
  17:   b8 00 00 00 00          mov    $0x0,%eax
  1c:   5d                      pop    %rbp
  1d:   c3                      ret
```

```mips
00000000 <main>:
   0:   27bdffe0        addiu   sp,sp,-32
   4:   afbf001c        sw      ra,28(sp)
   8:   afbe0018        sw      s8,24(sp)
   c:   03a0f025        move    s8,sp
  10:   3c1c0000        lui     gp,0x0
  14:   279c0000        addiu   gp,gp,0
  18:   afbc0010        sw      gp,16(sp)
  1c:   3c020000        lui     v0,0x0
  20:   24440000        addiu   a0,v0,0
  24:   8f820000        lw      v0,0(gp)
  28:   0040c825        move    t9,v0
  2c:   0320f809        jalr    t9
  30:   00000000        nop
  34:   8fdc0010        lw      gp,16(s8)
  38:   00001025        move    v0,zero
  3c:   03c0e825        move    sp,s8
  40:   8fbf001c        lw      ra,28(sp)
  44:   8fbe0018        lw      s8,24(sp)
  48:   27bd0020        addiu   sp,sp,32
  4c:   03e00008        jr      ra
  50:   00000000        nop
```

x86代码很容易看出call指令直接跳转到下一条指令，mips代码从0x0取了一个数进行跳转(很有可能会出现节错误)，而进行正常编译、反汇编后会得到更加庞大的反汇编代码，截取部分如下

```x86
# x86
0000000000001050 <puts@plt>:
    1050:       f3 0f 1e fa             endbr64 
    1054:       f2 ff 25 75 2f 00 00    bnd jmp *0x2f75(%rip)        # 3fd0 <puts@GLIBC_2.2.5>
    105b:       0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
# 此处省略数行
0000000000001149 <main>:
    1149:       f3 0f 1e fa             endbr64 
    114d:       55                      push   %rbp
    114e:       48 89 e5                mov    %rsp,%rbp
    1151:       48 8d 05 ac 0e 00 00    lea    0xeac(%rip),%rax        # 2004 <_IO_stdin_used+0x4>
    1158:       48 89 c7                mov    %rax,%rdi
    115b:       e8 f0 fe ff ff          call   1050 <puts@plt>
    1160:       b8 00 00 00 00          mov    $0x0,%eax
    1165:       5d                      pop    %rbp
    1166:       c3                      ret
```

```mips
004006e0 <main>:
  4006e0:       27bdffe0        addiu   sp,sp,-32
  4006e4:       afbf001c        sw      ra,28(sp)
  4006e8:       afbe0018        sw      s8,24(sp)
  4006ec:       03a0f025        move    s8,sp
  4006f0:       3c1c0042        lui     gp,0x42
  4006f4:       279c9010        addiu   gp,gp,-28656
  4006f8:       afbc0010        sw      gp,16(sp)
  4006fc:       3c020040        lui     v0,0x40
  400700:       24440830        addiu   a0,v0,2096
  400704:       8f828030        lw      v0,-32720(gp)
  400708:       0040c825        move    t9,v0
  40070c:       0320f809        jalr    t9
  400710:       00000000        nop
  400714:       8fdc0010        lw      gp,16(s8)
  400718:       00001025        move    v0,zero
  40071c:       03c0e825        move    sp,s8
  400720:       8fbf001c        lw      ra,28(sp)
  400724:       8fbe0018        lw      s8,24(sp)
  400728:       27bd0020        addiu   sp,sp,32
  40072c:       03e00008        jr      ra
  400730:       00000000        nop
        ...
```

相比于没有连接的，x86下main function跳转到别的节，而交叉汇编下main也从非零地址取数进行跳转操作

#### objdump参数含义

使用 `man objdump`获取参数信息可知

| 参数 | 含义                             |
| ---- | -------------------------------- |
| -D   | 反汇编所有(节)                   |
| -S   | 保留源码(机器码)，使它们交替显示 |

### 1.2

使用自己编写的 `readelf`程序解析之前在 `target`目录下生成的内核 `ELF`文件结果如下
![1679390752501](image/lab1_report/1679390752501.png)

mos是ELF32格式解析正常。
解析自身结果如下
![1679390968635](image/lab1_report/1679390968635.png)

解析失败，使用 `readelf -h` 查看其信息，发现它是ELF64格式，无法解析，打开 `tools/readelf/Makefile`

```makefile
%.o: %.c
        $(CC) -c $<

.PHONY: clean

readelf: main.o readelf.o
        $(CC) $^ -o $@

hello: hello.c
        $(CC) $^ -o $@ -m32 -static -g

clean:
        rm -f *.o readelf hello
```

对比可知编译 `hello.c`使用了 `-m32`参数控制其按照ELF32编译，而 `readelf`按照默认的ELF64编译

### 1.3

真实情况下，stag1初始化硬件设备，将stag2的代码加载到对应位置并跳转移交运行权限；stag2也包含自己的初始化，设置内核参数，并跳转到指定内核入口使内核开始运行启动
但是实验环境下，我们开始就具备正常的程序运行环境，内存和一些外设，只需要使用 `linker script`(`kernel.lds`)将指定源文件编译到正确位置再通过 `init/start.S`跳转到 `mos`入口即可

## 实验难点

- 对ELF文件格式陌生
- printk难度并不是很大就不提了，只是注意 `gcc`的 `printf`同时支持 `%0-d`和 `%-0d`

![1679393057851](image/lab1_report/1679393057851.png)
ELF三类文件:可执行，可重定位置，共享目标。当看完三个结构体时才知道上图包含很多信息

1. ELF文件内容包含三个表，许多节段
2. 通过节段表信息查询其在文件中的位置进行分析

在明白这两个的基础上，再使用 `readelf`、`objdump`等工具就能加深对ELF文件格式的理解
理解了这个的基础上才知道我们的内核(mos也是ELF文件)的数据组织形式，以及为什么要使用 `.lds`指定加载节的位置

### 体会与感想

  Lab1主要让我恶补了linux可执行文件的组织形式以及认识内核的加载启动过程，最终的printk实战相比前两个显得没那么重要。本次lab实际编写时间不长(约2~3个小时，并且大部分在写printk)，但是看资料和看指导书以及体验各种编译工具花费时间大概时2天左右

  其次就是认识到这么课对C语言指针大量依赖，需要认真学习C语言指针

  接下来应该需要大量恶补理论知识以及看源码

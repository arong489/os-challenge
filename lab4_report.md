# lab4 report

## 思考题

### Think4.1

#### Q1: 内核在保存现场的时候是如何避免破坏通用寄存器的？

具体系统调用过程在[实验难点](#系统调用过程)中体现，具体的保存逻辑在SAVE_ALL(inlcude/stackframe.h)中

先用k0寄存器保存sp寄存器的值再改变sp寄存器的值，并在下次改变k0寄存器之前首先将k0(sp的值)存进TF_REG29，此后整个现场保存过程只涉及对sp, k0的修改，而k0寄存器本来就是约定好内核用的，不需要保存

#### Q2: 系统陷入内核调用后可以直接从当时的 \$a0-\$a3 参数寄存器中得到用户调用 msyscall留下的信息吗？

\$a0不行，\$a1-\$a3可以，\$a0会在`handle_sys`函数中被修改为(KSTACKTOP-TF_SIZE)用来给`do_syscall`传tf参数

#### Q3: 我们是怎么做到让 sys 开头的函数“认为”我们提供了和用户调用 msyscall 时同样的参数的？

将\$a0-$\a3以及\$sp在进入内核时存入trap frame中，并在`do_syscall`调用`sys_`时将对应的参数取出来传参给它

#### Q4: 内核处理系统调用的过程对 Trapframe 做了哪些更改？这种修改对应的用户态的变化是什么？

将cp0_epc的值加了4，这样在系统调用结束回到用户态时会进入系统调用的下一条指令，不会同一地方反复进行系统调用

### Think4.2

#### 思考 envid2env 函数: 为什么 envid2env 中需要判断 e->env_id != envid的情况？如果没有这步判断会发生什么情况？

同一个Env结构体在被占用释放又被占用的过程中，每一次的envid是不一样的，但是e的确定是根据Env结构体确定的，也就是说同一个Env结构体在多次被占用的时候会有不同的envid

### Think4.3

#### 思考下面的问题，并对这个问题谈谈你的理解：请回顾 kern/env.c 文件中 mkenvid() 函数的实现，该函数不会返回 0，请结合系统调用和 IPC 部分的实现与envid2env() 函数的行为进行解释

当envid的值是0时，函数会通过形参penv返回指向当前进程控制块的指针。当某些系统调用（如IPC功能）函数需要访问当前进程的进程控制块时，可以直接通过向envid2env传0来会获得指向当前进程控制块的指针，然后通过指针对进程控制块进行访问。

### Think4.4

#### 关于 fork 函数的两个返回值，下面说法正确的是

C、fork 只在父进程中被调用了一次，在两个进程中各产生一个返回值

### Think4.5

#### 我们并不应该对所有的用户空间页都使用 duppage 进行映射。那么究竟哪些用户空间页应该映射，哪些不应该呢？请结合 kern/env.c 中 env_init 函数进行的页面映射、include/mmu.h 里的内存布局图以及本章的后续描述进行思考

ULIM到UVPT存放的是页表和页目录，不是共享页面。

UVPT到UTOP存放的是pages和envs，这些在env_init中已经映射好了，不是共享页面。

UTOP和USTACKTOP之间是异常处理栈（user exception stack），进行异常处理时的数据存放点，不需要共享这部分的内存。

所以需要被映射的页面只有USTACKTOP之下有效(有PTE_V权限)的页面。

### Think4.6

#### 在遍历地址空间存取页表项时你需要使用到 vpd 和 vpt 这两个指针，请参考 user/include/lib.h 中的相关定义，思考并回答这几个问题

#### Q1: vpt 和 vpd 的作用是什么？怎样使用它们？

vpt 和 vpd 分别是指向用户页表和用户页目录的指针，可以用来对用户页表和页目录进行访问。
vpd 使用时用虚拟地址的一级页号作为偏移量即可访问对应页目录项  e.g.vpd[PDX(va)]
vpt 使用时需要将一二级页号一起作为偏移量进行页表项访问  e.g.vpt[VPN(va)]

#### Q2:从实现的角度谈一下为什么进程能够通过这种方式来存取自身的页表？

vpt 和 vpd 分别指向了两个值 UVPT 和(UVPT+(UVPT>>12)*4)，这两个值分别是用户地址空间中页表的首地址和页目录的首地址。所以我们可以直接通过vpt和vpd访问到用户进程页表和页目录。

#### Q3:它们是如何体现自映射设计的？

vpd 的值是(UVPT+(UVPT>>12)*4)，而这个地址正好在UVPT和ULIM之间，说明页目录被映射到了某一个页表的位置。而每一个页表都被页目录中的一个页表项所映射。因此页目录中一定有一个页表项映射到了页目录本身。

#### Q4:进程能够通过这种方式来修改自己的页表项吗？

不能，用户态不具备写权限，页表是内核态程序维护的，用户进程只能对页表项进行访问

### Think4.7

#### 在 do_tlb_mod 函数中，你可能注意到了一个向异常处理栈复制 Trapframe运行现场的过程，请思考并回答这几个问题

#### Q1: 这里实现了一个支持类似于“异常重入”的机制，而在什么时候会出现这种“异常重入”？

用户发生写时复制导致异常并处理的过程中，有可能还会发生写时复制，所以要 “中断重入”，类似于函数嵌套递归调用的方式重复处理，直到不再缺页异常

#### Q2: 内核为什么需要将异常的现场 Trapframe 复制到用户空间？

因为最终实际是用户态`cow_entry`函数在处理写时复制，所以需要将 tf 复制到用户空间

### Think4.8

#### 在用户态处理页写入异常，相比于在内核态处理有什么优势？

减少内核代码的工作量，即使出错也不至于系统崩溃，但是如果内核出现错误，就会导致系统崩溃

### Think4.9

#### Q1: 为什么需要将syscall_set_tlb_mod_entry 的调用放置在 syscall_exofork 之前？

我认为还可以可以放在 syscall_exofork 之后，duppage 之前

在duppage中也会发生页写时复制，所以要先放置好页面写时复制的异常处理函数地址

#### Q2: 如果放置在写时复制保护机制完成之后会有怎样的效果？

可能出现在 duppage 的时候发生写时复制，但是没有异常处理地址的情况

## 实验难点

本次实验我遇到的难点在于

- C语言和mips之间交替出现，对参数和寄存器的存取处理不清晰
- 调度队列加入方式唯一
- vpt使用(用VPN宏)
- 对用户态各个数据段用途的掌握不清

### 系统调用过程

```puml

skinparam ComponentStyle rectangle
package user{
    [user syscall function\n in "user/lib/c_file"] as user_func
    [ function: "syscall_*"\nin "user/lib/syscall_lib"] as syscall
    [    function: "msyscall"\nin "user/lib/syscall_wrap"] as msyscall
}
package kern{
    package exception_part{
        [function: "exc_gen_entry"\n         in "kern/entry"] as exc_gen_entry
    }
    package syscall_part{
        [function: "handle_sys"\n      in "kern/genex"] as handle_sys
        [function: "do_syscall"\n in "kern/syscall_all"] as do_syscall
        [   function: "sys_*"\n in "kern/syscall_all"] as sys
    }
    note right of exc_gen_entry
    save
    all the 32 regs 
    cp0_status
    hi
    lo
    cp0_badvaddr
    cp0_cause
    cp0_epc
    to struct trapframe
    end note
}

user_func -right-> syscall
syscall --> msyscall

msyscall --> exc_gen_entry : syscall
exc_gen_entry --> handle_sys : through handler_exception array
handle_sys --> do_syscall
do_syscall -right-> sys

```

### fork过程

```puml

skinparam ComponentStyle rectangle

package fork{
    [syscall_set_tlb_mod_entry(curenv)] as set_entry_self
    [syscall_exofork] as exofork
    [duppage] as duppage
    [syscall_set_env_status] as set_status
    [syscall_set_tlb_mod_entry(child)] as set_entry_child
    set_entry_self --> exofork
    exofork --> duppage
    duppage --> set_entry_child
    set_entry_child --> set_status
    note right of exofork
        env_alloc
        set child env
    end note
    note right of duppage
        use "syscall_mem_map" to
        set copy-on-write pages
    end note
    note right of set_status
        set child status to runnable
        and add it to schedule list
    end note
}

```

## 实验体会

本次实验用了一天时间debug，甚至倒逼着掌握了基本的gxe调试。此外还花了半天各种跳转看代码，摸清几个过程和数据段用途
结论是：多看代码有好处
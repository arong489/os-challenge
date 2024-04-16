# lab3 report

## 思考题

### Think3.1

#### 请结合 MOS 中的页目录自映射应用解释代码中 e->env_pgdir[PDX(UVPT)]= PADDR(e->env_pgdir) | PTE_V 的含义。

UVPT是pages空间的顶端，PDX(x)=(x>>22)

一个页框大小是4MiB，UVPT所在页表是第$UVPT>>22$个页表
映射到页目录的第$UVPT>>22$项

也就是将页目录所在页目录项映射到其物理地址上，并且设置权限为仅读

### Think3.2

#### elf_load_seg 以函数指针的形式，接受外部自定义的回调函数 map_page。请你找到与之相关的 data 这一参数在此处的来源，并思考它的作用。没有这个参数可不可以？为什么？

data来自load_icode函数中的参数e，该参数是env_create函数中通过env_alloc()获取的一个新的PCB结构体，env_create调用load_icode将binary处size大小的elf文件与PCB结构体e建立映射关系，如果没有这个data参数，回调函数就无法对对应PCB结构体进行修改，所以不能没有这个参数

### Think3.3

#### 结合 elf_load_seg 的参数和实现，考虑该函数需要处理哪些页面加载的情况。

代码中体现了四种情况，根据是va是否和page对齐，以及segment是否只涉及一页。按道理还有va+bin_size是否和page对齐，但是用求bin_size和余下空间最小值将其避免了。
elf_load_seg第一个if将最初一页va不与page对齐的部分完成映射，随后的for循环将余下的页完成映射

### Think3.4

#### 思考上面这一段话，并根据自己在 Lab2 中的理解，回答：你认为这里的 env_tf.cp0_epc 存储的是物理地址还是虚拟地址?

是虚拟地址，该地址参与取指令和跳转等操作，代码中访问的都是虚拟地址

### Think3.5

#### 试找出 0、1、2、3 号异常处理函数的具体实现位置。8 号异常（系统调用）涉及的 do_syscall() 函数将在 Lab4 中实现。 

输入`grep -inr handle *`查看全部的handle函数
![1681823057891](image/lab3_report/1681823057891.png)
0号是int异常，1号是reserved异常，2、3号是tlb异常
再查看全部do相关函数
![1681823130339](image/lab3_report/1681823130339.png)

handle_int写在`kern/genex.S`中，直接跳转到`kern/sched.c`中的`schedule`函数进行处理

handle_reserved跳转到`kern/traps.c`中的`do_reserved`函数进行处理

handle_tlb跳转到`kern/tlb_asm.S`中的`do_tlb_refill`处进行处理

### Think3.6

#### 阅读 init.c、kclock.S、env_asm.S 和 genex.S 这几个文件，并尝试说出enable_irq 和 timer_irq 中每行汇编代码的作用。 

```c
//in file kern/env_asm.S
LEAF(enable_irq)
	li      t0, (STATUS_CU0 | STATUS_IM4 | STATUS_IEc)  //预设置时钟中断参数到t0
	mtc0    t0, CP0_STATUS                              //将参数传入CP0使之相应时钟中断
	jr      ra                                          //返回调用处
END(enable_irq)

//in file kern/genex.S
timer_irq:
	sw      zero, (KSEG1 | DEV_RTC_ADDRESS | DEV_RTC_INTERRUPT_ACK) //将0写入GXemul的实时钟，关闭时钟中断
	li      a0, 0                                                   //设置schedule函数参数值为0
	j       schedule                                                //跳转到schedule进行调度
END(handle_int)

```

### Think3.7

#### 阅读相关代码，思考操作系统是怎么根据时钟中断切换进程的。

进程(PCB块)装在两个队列中，在env_sched_list中的是可调度执行的进程，当每次产生时钟中断时会产生int异常，在`exc_gen_entry`中储存上下文，然后从`exception_handlers`数字组中取出对应异常的处理函数进行跳转，这里是跳转到`handle_int`中，`handle_int`跳转到`schedule`进行最终的调度

## 难点分析

### env与elf文件的映射建立

env与elf文件的映射建立流程

- env_init(初始化两个队列) 
  - page_alloc(获取页目录) 
  - map_segment(建立pages和envs的虚拟地址和物理地址页表)
- env_create(创建PCB表，初始化并扔进sched_list)
  - page_alloc
  - 初始化priority和status
  - load_icode(将elf文件与env相关联)
    - elf_from(elf幻数验证)
    - elf_load_seg(装填elf文件到env)
      - load_icode_mapper(复制并插入新的page)
  - 上述处理完之后扔进sched_list

### 中断处理过程

在think3.7中已经把我理解的都说了。
最初不理解的原因是`exception_handlers`是一个无参函数数组，但是实际调用到最后都是有参处理函数，看了很久才知道汇编文件handler_build宏定义了处理的模板，在这里加载参数再调用下层的处理函数

## 体会

写的时候十分迷茫，要写完了返回来找着调用一个一个读函数才能读出些东西，并且对之前对页表-虚拟地址-物理地址的关联不够熟练，会延长理解时间，这次作业对代码中变量访问取值的逻辑不理解给我带来了极大困难，甚至现在还不甚理解

结论是多读代码，调用太多了
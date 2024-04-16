# lab2 challenge

## 仪式的事前准备

这个例程实现了MIPS架构师为类unix操作系统中的用户地址所考虑的转换机制。它依赖于在内存中为每个地址空间构建一个页表。页表由一个单字条目的线性数组组成，由VPN索引，其格式与EntryLo寄存器的位域相匹配。这样的方案很简单，但有一个问题。由于每4kb的用户地址空间占用4字节的表空间，所以整个2gb的用户空间需要一个2Mbyte的表，这是一个很大的数据块。此外，大多数用户地址空间都在底部(用于代码和数据)和顶部(用于向下增长的堆栈)使用，两者之间有很大的间隙。所采用的解决方案受到Digital的VAX架构的启发，并将页表本身定位在虚拟内存中的kseg2区域中。这一下子巧妙地解决了两个问题:

    - 节省物理内存，因为页表中间未使用的空白将永远不会被引用。
    - 提供了一个简单的机制，当改变上下文时重新映射一个新的用户页表，而不必在操作系统中找到足够的虚拟地址来一次映射所有的页表。

MIPS体系结构以上下文寄存器的形式为这种机制提供了积极的支持。如果页表从1Mbyte的边界开始(因为它在虚拟内存中，任何创建的间隙都不会耗尽物理内存空间)，并且Context PTEBase字段被页表起始地址的高序位填充，那么在用户重新填充异常之后，Context寄存器将包含重新填充所需的条目的地址，无需进一步计算。

生成的例程如下所示

```s
.set    noreorder 
.set    noat 
xcpt_vecfastutlb: 
    mfc0    k1, C0_CONTEXT 
    mfc0    k0, C0_EPC  # mfc0 delay slot 
    lw      k1, 0(k1)   # may double fault (k0 = orig EPC) 
    nop 
    mtc0    k1, C0_ENTRYLO 
    nop 
    tlbwr 
    jr      k0 
    rfe 
    xcpt_endfastutlb: 
.set    at 
.set    reorder
```

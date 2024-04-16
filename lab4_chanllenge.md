# lab4 challenge

## 实现思路

### 变量

由于每个进程有自己独立的信号处理注册表、待处理信号队列以及屏蔽掩码，所以将三者放到PCB头中。
而为了实现信号的用户态轮询处理需要像`tlb_mod`一样有一个用户态处理接口，负责处理后回调到内核态。

```c
struct Env {
    struct Trapframe env_tf;  // Saved registers
    LIST_ENTRY(Env) env_link; // Free list
    u_int env_id;           // Unique environment identifier
    u_int env_asid;         // ASID
    u_int env_parent_id;    // env_id of this env's parent
    u_int env_status;       // Status of the environment
    Pde *env_pgdir;         // Kernel virtual address of page dir
    TAILQ_ENTRY(Env) env_sched_link;
    u_int env_pri;
    // Lab 4 IPC
    u_int env_ipc_value;   // data value sent to us
    u_int env_ipc_from;    // envid of the sender
    u_int env_ipc_recving; // env is blocked receiving
    u_int env_ipc_dstva;   // va at which to map received page
    u_int env_ipc_perm;    // perm of page mapping received

    // Lab 4 fault handling
    u_int env_user_tlb_mod_entry; // user tlb mod handler

    // Lab 6 scheduler counts
    u_int env_runs; // number of times been env_run'ed

    //lab 4 challenge
    sig_entry_func do_signal_entry; // do_signal entry in user text
    signo_node *signoHeader;    // signal wait list
    sigset_t env_sigprocmask;   // process bit mask
    struct sigaction sighandler_table[64];  // handler regist table
};
```

使用的变量类型定义如下

```c
typedef struct{
    int sig[2]; //最多 32*2=64 种信号
} sigset_t;//信号掩码

struct sigaction{
    void (*sa_handler)(int);
    sigset_t sa_mask;
};//信号量注册表表项，包含运行时掩码和处理函数地址

typedef struct signoListNode{
    char signo;
    struct signoListNode *next;
} signo_node;//信号等待队列链表节点

typedef void(*sig_entry_func)(struct Trapframe *, int, void (*)(int), sigset_t*);//用户态信号处理接口函数类型
```

### 信号辅助操作函数(与系统的实现无关)

内核态辅助函数：
为了用掩码改变掩码，以及查询掩码存在性

```c
#define sigset_add(a, b, c) ({(a).sig[0] = ((b).sig[0] | (c).sig[0]) & (~0x40100); (a).sig[1] = (b).sig[1] | (c).sig[1];}) //信号量添加，a = b & c，由于9和18号无法被屏蔽故过滤指
#define sigset_del(a, b, c) ({(a).sig[0] = (b).sig[0] & (~(c).sig[0]); (a).sig[1] = (b).sig[1] & (~(c).sig[1]);}) //信号量删除，a = b - c
#define sigset_set(a, b) ({(a).sig[0] = (b).sig[0] & (~0x40100); (a).sig[1] = (b).sig[1];}) // 信号量设置，同样过滤9和18
#define sigset_ctn(set, signo) ((set).sig[signo/32] & (1 << (signo&0x1f)))// 查找掩码是否包含signo
```

用户态下实现实验要求的几个就好

```c
void sigemptyset(sigset_t *set){ // 清空信号集，将所有位都设置为 0
    set->sig[0] = set->sig[1] = 0;
}
void sigfillset(sigset_t *set){ // 设置信号集，即将所有位都设置为 1
    set->sig[0] = set->sig[1] = -1;
}
void sigaddset(sigset_t *set, int signum){ // 向信号集中添加一个信号，即将指定信号的位设置为 1
    set->sig[signum / 32] |= 1 << (signum & 0x1f);
}
void sigdelset(sigset_t *set, int signum){ // 从信号集中删除一个信号，即将指定信号的位设置为 0
    set->sig[signum / 32] &= ~(1 << (signum & 0x1f));
}

int sigismember(const sigset_t *set, int signum){ // 检查一个信号是否在信号集中，如果在则返回 1，否则返回 0
    return (set->sig[signum / 32] & (1 << (signum & 0x1f))) != 0;
}
```

### 信号系统的实现

系统调用逻辑，以kill为例子

```c
// in user/lib/signal.c
int kill(u_int envid, int signo) {
    if (signo < 1 || 64 < signo) return -1;     //预判断非法信号
    return syscall_kill(envid, signo);          //调用lib函数
}
// in user/lib/syscall_lib
int syscall_kill(u_int envid, int signo) {
    return msyscall(SYS_kill, envid, signo);    //调用msyscall汇编码
}
```

在`user/lib/syscall_wrap.S`中有msyscall的定义

```s
LEAF(msyscall)
    // Just use 'syscall' instruction and return.

    /* Exercise 4.1: Your code here. */
    syscall
    jr  ra
END(msyscall)
```

其调用`syscall`指令遁入内核中（mos中是被当作一种异常对待），异常处理函数在`0x80000080`处的`exc_gen_entry`

```s
.section .text.exc_gen_entry
exc_gen_entry:
    SAVE_ALL
/* Exercise 3.9: Your code here.*/
mfc0    t0, CP0_CAUSE
andi    t0, 0x7c
lw      t0, exception_handlers(t0)
jr      t0
```

`SAVE_ALL`宏的定义在`include/stackframe.h`中，作用就是将现场相关寄存器存起来，与之相对的`RESTORE_SOME`（同一文件中）会将现场还原，注意，SAVE_ALL后`$sp`寄存器的值已经变到内核栈，这意味着接下来的变量和压栈都会开在内核栈中
保留现场后，会从CP0_CAUSE取出原因然后通过`kern/traps.c`中的一张表

```c
void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};
```

进行异常的分发。这里，我们进入`kern/syscall_all.c`中的`do_syscall`。
它的作用是将保存在内核栈中的用户态环境中的参数取出传给内核中的调用实现，详细时实现请自查，而实现函数的分发也是靠同文件中的`syscall_table`表实现的。

经历一番曲折，终于来到内核实现

```c
int sys_kill(int envid, int signo) {
    struct Env *e;
    if (envid2env(envid, &e, 0)) {
        return -1;
    }
    // printk("env-%d add signo-%d\n", envid, signo);
    //TODO: allocate space for insert
    signo_node *insert;
    if (signo_alloc(&insert, signo) < 0) {
        panic("add signo fail\n");
        return -1;
    }
    insert->next = e->signoHeader;
    e->signoHeader = insert;
    return 0;
}
```

这里我们先查`envid`的合法性，然后再声明并初始化一个信号链表节点，将其插入为头节点

系统调用详细流程至此

给出余下两个调用

```c
// in user/lib/signal.c

int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact){
    if (signo < 1 || 64 < signo) return -1;
    syscall_sigaction(signo, act, oldact);
    return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset){
    if(how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK) return -1;
    syscall_sigprocmask(how, set, oldset);
    return 0;
}

// in user/lib/syscall_all.c
void syscall_sigaction(int signo, const struct sigaction *act, struct sigaction *oldact) {
    msyscall(SYS_sigaction, signo, act, oldact);
}

void syscall_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    msyscall(SYS_sigprocmask, how, set, oldset);
}

// in kern/syscall_all.c
void sys_sigaction(int signo, const struct sigaction *act, struct sigaction *oldact) {
    struct sigaction feedback = curenv->sighandler_table[signo - 1];
    if (act) curenv->sighandler_table[signo - 1] = *act;
    if (oldact) *oldact = feedback;
}
void sys_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    sigset_t feedback = curenv->env_sigprocmask;
    if (set) {
        switch (how)
        {
        case SIG_BLOCK:
            sigset_add(curenv->env_sigprocmask, curenv->env_sigprocmask, *set);
            break;
        case SIG_UNBLOCK:
            sigset_del(curenv->env_sigprocmask, curenv->env_sigprocmask, *set);
            break;
        case SIG_SETMASK:
            sigset_set(curenv->env_sigprocmask, *set);
            break;
        default:
            return;
        }
    }
    if (oldset) *oldset = feedback;
}
```

### 信号处理(重难点)

信号处理应该在跨越到用户态后立马进行处理，实际上用户态拿不到进程头的数据，在跨越到用户态后不知道该处理哪些信号，如果说将应当处理的信号全部拿出来放到用户栈中，这确实可以，但是会带来一个问题，子进程也会复制这些信号然后执行，而子进程的复制不应该复制等待的信号，所以，我选择一次处理一个，类似`do_tlb_mod`的循环调用。

```c
// 用户态处理函数
// in user/lib/signal.c
static void do_signal(struct Trapframe *tf, int signo, void (*sa_handler)(int), sigset_t *originMask) {
    if (sa_handler) {
        sa_handler(signo);
    } else {
        if (signo == SIGKILL || signo == SIGSEGV || signo == SIGTERM) {
            exit();
        }
    }
    int r = msyscall(SYS_back_do_signal, 0, tf, originMask);//重点
    user_panic("syscall_set_trapframe returned %d", r);
}
```

重点在用`msyscall(SYS_back_do_signal, 0, tf, originMask)`回调恢复

```c
// in kern/syscall_all.c
int sys_back_do_signal(u_int envid, struct Trapframe *tf, sigset_t *originMask) {
    if (envid && envid != curenv->env_id) {
        panic("bad envid in do signal return function\n");
    }

    int r;
    if ((r = sys_set_trapframe(envid, tf)) < 0) {
        return r;
    }

    curenv->env_sigprocmask = *originMask;
    return r;
}
```

`sys_back_do_signal`没有在`user/syscall.c`和`user/signal.c`中留下定义，因为我认为这两者是用户不该使用的函数，其作用分别是从用户态信号处理接口返回，上文说过，进入`kern/syscall_all.c`前早就变了内核栈，其数据还是上次回调时产生用户态数据，而我们这时应该当作没有上次信号处理一样操作，所以我们使用`sys_set_trapframe`直接将当前内存栈环境改回第一次信号处理前的环境。

而内核态处理函数能收到这些参数，实际上还得在出内核态退出前给它传参

```c
void _do_signal(struct Trapframe *tf) {
    if (tf == 0) {
        panic("bad tf addr\n");
    }
    signo_node *signo_node_ptr = curenv->signoHeader;
    signo_node *signo_node_frm = NULL;
// find unblocked signal
find_unblocked_signo:
    while (signo_node_ptr != NULL && sigset_ctn(curenv->env_sigprocmask, signo_node_ptr->signo)) {
        signo_node_frm = signo_node_ptr;
        signo_node_ptr = signo_node_ptr->next;
    }
    if (signo_node_ptr != NULL) {
        int signo;
        signo_node *signo_node_del = NULL;
        signo = signo_node_ptr->signo;
        // remove signo
        signo_node_del = signo_node_ptr;
        if (signo_node_frm) signo_node_frm->next = signo_node_ptr->next;
        else curenv->signoHeader = NULL;
        signo_node_ptr = signo_node_ptr->next;
        signo_free(&signo_node_del);

        if (curenv->sighandler_table[signo - 1].sa_handler == NULL && !(signo == SIGKILL || signo == SIGSEGV || signo == SIGTERM)) 
            goto find_unblocked_signo;
        
        // load args
        if (curenv->do_signal_entry == NULL) {
            if (signo != SIGSEGV) {
                panic("only SIGSEGV can exit in kern\n");
            } else {
                panic("address too low\n");
            }
        }

        sigset_t originMask = curenv->env_sigprocmask;
        curenv->env_sigprocmask = curenv->sighandler_table[signo - 1].sa_mask;

        *(((struct Trapframe *)tf->regs[29]) - 1) = *tf;//备份tf，运行环境
        tf->regs[29] -= sizeof(struct Trapframe);       //入栈
        tf->regs[4] = tf->regs[29];                     //传参

        tf->regs[29] -= sizeof(originMask);             //入栈
        *(sigset_t *)tf->regs[29] = originMask;         //备份mask，信号掩码
        tf->regs[7] = tf->regs[29];                     //传参

        tf->regs[5] = signo;                            //传参
        tf->regs[6] = (long unsigned int)curenv->sighandler_table[signo - 1].sa_handler;//传参
        tf->regs[29] -= sizeof(tf->regs[4]) * 4;        //参数入栈

        tf->cp0_epc = (long unsigned int)curenv->do_signal_entry;//设置do_signal地址
    }
}
```

这段设置函数前半部分(load args注释前)是在处理PCB头中未阻塞信号的查找和删除问题
后半部分，将运行时环境 和 处理函数运行前进程信号掩码备份到用户栈中，同时将用户态处理接口`do_signal`需要的参数导入，最后设好返回用户态的地址

至此，最核心的循环处理部分结束

### 后记(细枝末节)

1. 用户态入口函数的设置

```c
// in kern/syscall_all.c
int sys_set_sig_entry(int envid , sig_entry_func do_signal_entry) {
    struct Env *env;
    try(envid2env(envid, &env, 0));
    env->do_signal_entry = do_signal_entry;
    return 0;
}
```

没有在用户态设置函数的理由如`sys_back_do_signal`
何时调用`msyscall(SYS_set_sig_entry, 0, do_signal);`进行设置？
应该在用户态`fork`函数中调用进行设置
但是我的代码实在调用`sigaction`、`kill`、`sigprocmask`中调用设置的，主要是为了和之前的代码一致，便于回归测试，这样带来的问题很明显，一旦有 空指针访问 和 中断 发生，可能找不到处理函数，只能在内核进行`panic`

2. 信号链表节点的管理

```c
// in kern/signal.c
void signo_init(void) {
    signo_free_list = signos;
    for (u_int i = 0; i < MAX_SIG_NUM - 1; i++) {
        signos[i].next = signos + i + 1;
    }
    if (signo_free_list)
        printk("signo space init success\nsigno begin vaddr 0x%x\n", signo_free_list);
}

int signo_alloc(signo_node **signo_ptr, int signo) {
    if (signo_free_list == NULL) return -1;
    *signo_ptr = signo_free_list;
    signo_free_list = signo_free_list->next;
    (*signo_ptr)->signo = signo;
    return 0;
}

void signo_free(signo_node **signo_ptr) {
    (*signo_ptr)->next = signo_free_list;
    signo_free_list = *signo_ptr;
}
```

初始化放在`env_init`中即可，信号链表节点实际上开在`Kernel Text`中，采用`env`一样的方式进行管理

3. 进程的信号注册表继承和掩码继承

```c
// in kern/syscall_all.c
int sys_exofork(void) {
    
    ...

    e->env_sigprocmask = curenv->env_sigprocmask;
    for (u_int i = 0; i < 64; i++) {
        e->sighandler_table[i] = curenv->sighandler_table[i];
    }
    return e->env_id;
}
```

继承就不言而喻了

## 测试程序：对于功能的详细测试程序，以及运行测试程序得到的运行结果

测试基础

```c
#include <lib.h>

int global = 0;
void handler(int num) {
    debugf("Reach handler, now the signum is %d!\n", num);
    global = 1;
}

#define TEST_NUM 2
int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sig.sa_handler = handler;
    sig.sa_mask = set;
    panic_on(sigaction(TEST_NUM, &sig, NULL));
    sigaddset(&set, TEST_NUM);
    panic_on(sigprocmask(0, &set, NULL));
    kill(0, TEST_NUM);
    int ans = 0;
    for (int i = 0; i < 10000000; i++) {
        ans += i;
    }
    panic_on(sigprocmask(1, &set, NULL));
    debugf("global = %d.\n", global);
    return 0;
}
```

![1686917854590](image/lab4_chanllenge_report/1686917854590.png)

测试信号处理重入

```c
#include<lib.h>


#define TEST_NUM 5
int global = 0;
int id;

void inner_handler(int num) {
    debugf("Reach inner handler, now the signum is %d!\n", num);
    sigset_t temp;
    sigprocmask(0, NULL, &temp);
    debugf("father[in inner handler] has 35: %d\n", sigismember(&temp, 35));
}

void handler(int num)
{
    debugf("Reach handler, now the signum is %d!\n", num);
    sigset_t temp;
    sigprocmask(0, NULL, &temp);
    debugf("father[in handler] has 35: %d\n", sigismember(&temp, 35));
    sigaddset(&temp, 36);
    
    sigdelset(&temp, 35);
    sigprocmask(SIG_SETMASK, &temp, NULL);

    struct sigaction sig;
    sig.sa_handler = inner_handler;
    sig.sa_mask = temp;

    sigaction(36, &sig, NULL);
    kill(0, 36);
    global = 1;
}

int main(int argc, char** argv)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGKILL);
    sigaddset(&set, SIGSTOP);
    sigaddset(&set, TEST_NUM);
    sigaddset(&set, 35);
    struct sigaction sig;
    sig.sa_handler = handler;
    sig.sa_mask = set;

    sigaction(TEST_NUM, &sig, NULL);

    sigprocmask(SIG_BLOCK, &set, NULL);

    // debugf("fork at 0x%x\n", fork);
    kill(0, TEST_NUM);
    int ans = 0;
    for (int i = 0; i < 10000000; i++) {
        ans += i;
    }
    sigset_t oldset;
    sigemptyset(&oldset);
    sigdelset(&set, TEST_NUM);
    // int id = fork();
    sigprocmask(1, &set, &oldset);
    sigset_t temp;
    sigprocmask(0, NULL, &temp);
    debugf("father has 35: %d\n", sigismember(&temp, 35));
    sigaddset(&set, TEST_NUM);
    sigprocmask(1, &set, NULL);

    sigprocmask(0, NULL, &temp);
    debugf("father has 35: %d\n", sigismember(&temp, 35));
    debugf("father has 36: %d\n", sigismember(&temp, 36));

    debugf("global = %d.\n", global);
        
    return 0;
}
```

![1686917879329](image/lab4_chanllenge_report/1686917879329.png)

测试写时复制

```c
#include <lib.h>

sigset_t set2;

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, 1);
    sigaddset(&set, 2);
    panic_on(sigprocmask(0, &set, NULL));
    sigdelset(&set, 2);
    int ret = fork();
    if (ret != 0) {
        panic_on(sigprocmask(0, &set2, &set));
        debugf("Father: %d.\n", sigismember(&set, 2));
    } else {
        debugf("Child: %d.\n", sigismember(&set, 2));
    }
    return 0;
}
```

![1686917902778](image/lab4_chanllenge_report/1686917902778.png)

测试空指针访问错误

```c
#include <lib.h>

int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int *test = NULL;
void sgv_handler(int num) {
    debugf("Segment fault appear!\n");
    test = &a[0];
    debugf("test = %d.\n", *test);
    exit();
}

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sig.sa_handler = sgv_handler;
    sig.sa_mask = set;
    panic_on(sigaction(11, &sig, NULL));
    *test = 10;
    debugf("test = %d.\n", *test);
    return 0;
}
```

![1686917935119](image/lab4_chanllenge_report/1686917935119.png)

综合测试fork和写时复制，信号处理

```c
#include <lib.h>

#define TEST_NUM 2

int id;
int global = 0;

int deepth = 5;

void inner_handler(int num) {
    debugf("deepth: %d\n", deepth);
    deepth--;
    if (deepth > 0) {
        kill(0, 17);
    }
}

void handler(int num) {
    debugf("Reach handler, now the signum is %d!\n", num);
    id = fork();
    if (id != 0) {
        debugf("father[in handler]:");
        sigset_t temp;
        sigprocmask(0, NULL, &temp);
        debugf("has TEST_NUM: %d\n", sigismember(&temp, TEST_NUM));
    } else {
        debugf("child[in handler]:");
        sigset_t temp;
        sigprocmask(0, NULL, &temp);
        debugf("has TEST_NUM: %d\n", sigismember(&temp, TEST_NUM));
        sigemptyset(&temp);
        sigprocmask(SIG_SETMASK, &temp, NULL);
        struct sigaction tempaction;
        tempaction.sa_handler = inner_handler;
        tempaction.sa_mask = temp;
        sigaction(17, &tempaction, NULL);
        debugf("child[in handler] send new kill\n");
        kill(0, 17);
    }
    global = 1;
}

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sigaddset(&set, TEST_NUM);
    sig.sa_handler = handler;
    sig.sa_mask = set;
    panic_on(sigaction(TEST_NUM, &sig, NULL));
    panic_on(sigprocmask(0, &set, NULL));
    kill(0, TEST_NUM);
    int ans = 0;
    for (int i = 0; i < 10000000; i++) {
        ans += i;
    }
    panic_on(sigprocmask(1, &set, NULL));

    
    if (id != 0) {
        debugf("father:");
        sigset_t temp;
        sigprocmask(0, NULL, &temp);
        debugf("has TEST_NUM: %d\n", sigismember(&temp, TEST_NUM));
    } else {
        debugf("child:");
        sigset_t temp;
        sigprocmask(0, NULL, &temp);
        debugf("has TEST_NUM: %d\n", sigismember(&temp, TEST_NUM));
    }

    debugf("global = %d.\n", global);
    return 0;
}
```

![1686919061078](image/lab4_chanllenge_report/1686919061078.png)

## 实现过程中遇到的问题及相应的解决方案

遇到的最大问题是，一开始照着tlb_mod将do_signal时备份的数据放在内核异常栈内(`UXSTACKTOP`)下，于是出现在handler内`fork`子进程发生地址访问错误

解决方案：
经过彻夜思考，do_tlb_mod能正确的原因是不会在do_tlb_mod循环到内核态出现fork，do_tlb_mod是在一条语句中发生的，而 信号处理程序 是实实在在发生在用户态的函数过程，所以将 信号处理程序 的参数放在用户栈中才符合情理

从GXemul调试中看，子进程都是在若干次回到用户态的过程中发生了pc回归NULL的问题，细究发现其实是，在使用父进程复制在`UXSTACKTOP`的环境时出现了问题，fork并不拷贝内核态，所以将其放到用户栈中，问题顺利解决

## 感受

GXemul的使用体验很差，主要是没有源映射，得通过 看汇编识C 和 elf文件头查函数名 等方式得知源码中运行PC的去向
#include<env.h>
#include<trap.h>
#include<printk.h>
#include<mmu.h>

extern struct Env *curenv;
signo_node signos[MAX_SIG_NUM];
static signo_node *signo_free_list = signos;

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

void _do_signal(struct Trapframe *tf) {
    if (tf == 0) {
        panic("bad tf addr\n");
    }
    signo_node *signo_node_ptr = curenv->signoHeader;
    signo_node *signo_node_frm = NULL;

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
        
        if (curenv->do_signal_entry == NULL) {
            if (signo != SIGSEGV) {
                panic("only SIGSEGV can exit in kern\n");
            } else {
                panic("address too low\n");
            }
        }

        sigset_t originMask = curenv->env_sigprocmask;
        curenv->env_sigprocmask = curenv->sighandler_table[signo - 1].sa_mask;

        *(((struct Trapframe *)tf->regs[29]) - 1) = *tf;//备份tf
        tf->regs[29] -= sizeof(struct Trapframe);       //入栈
        tf->regs[4] = tf->regs[29];                     //传参

        tf->regs[29] -= sizeof(originMask);             //入栈
        *(sigset_t *)tf->regs[29] = originMask;         //备份mask
        tf->regs[7] = tf->regs[29];                     //传参

        tf->regs[5] = signo;                            //传参
        tf->regs[6] = (long unsigned int)curenv->sighandler_table[signo - 1].sa_handler;//传参
        tf->regs[29] -= sizeof(tf->regs[4]) * 4;        //参数入栈

        tf->cp0_epc = (long unsigned int)curenv->do_signal_entry;//设置do_signal地址
    }
}
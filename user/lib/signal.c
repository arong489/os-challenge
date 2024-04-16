#include<env.h>
#include<lib.h>

// 用户态处理函数
static void do_signal(struct Trapframe *tf, int signo, void (*sa_handler)(int), sigset_t *originMask) {
    // debugf("do signal received signo%d\n", signo);
    if (sa_handler) {
        sa_handler(signo);
    } else {
        if (signo == SIGKILL || signo == SIGSEGV || signo == SIGTERM) {
            exit();
        }
    }
    // debugf("leave handler now\n");
    int r = msyscall(SYS_back_do_signal, 0, tf, originMask);
	user_panic("syscall_set_trapframe returned %d", r);
}



extern volatile struct Env *env;
int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact){
    if (signo < 1 || 64 < signo) return -1;
    if (env->do_signal_entry != do_signal) {
        msyscall(SYS_set_sig_entry, 0, do_signal);
    }
    syscall_sigaction(signo, act, oldact);
    return 0;
}

// 向 envid 传送 signo
int kill(u_int envid, int signo) {
    if (signo < 1 || 64 < signo) return -1;
    if (env->do_signal_entry != do_signal) {
        msyscall(SYS_set_sig_entry, envid, do_signal);
    }
    return syscall_kill(envid, signo);
}
// 
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset){
    if(how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK) return -1;
    if (env->do_signal_entry != do_signal) {
        msyscall(SYS_set_sig_entry, 0, do_signal);
    }
    syscall_sigprocmask(how, set, oldset);
    return 0;
}

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

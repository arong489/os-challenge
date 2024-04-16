#ifndef _SIGNAL_H_
#define _SIGNAL_H_

// how
#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2
// 64种信号
# define SIGHUP     1
# define SIGINT     2
# define SIGQUIT    3
# define SIGILL     4
# define SIGTRAP    5
# define SIGABRT    6
# define SIGBUS     7
# define SIGFPE     8
# define SIGKILL    9
# define SIGUSR1    10
# define SIGSEGV    11
# define SIGUSR2    12
# define SIGPIPE    13
# define SIGALRM    14
# define SIGTERM    15
# define SIGSTKFLT  16
# define SIGCHLD    17
# define SIGCONT    18
# define SIGSTOP    19
# define SIGTSTP    20
# define SIGTTIN    21
# define SIGTTOU    22
# define SIGURG     23
# define SIGXCPU    24
# define SIGXFSZ    25
# define SIGVTALRM  26
# define SIGPROF    27
# define SIGWINCH   28
# define SIGIO      29
# define SIGPWR     30
# define SIGSYS     31
# define SIGRTMIN   34
# define SIGRTMAX   64


#define MAX_SIG_NUM 1024

typedef struct{
    int sig[2]; //最多 32*2=64 种信号
} sigset_t;

struct sigaction{
    void (*sa_handler)(int);
    sigset_t sa_mask;
};

typedef struct signoListNode{
    char signo;
    struct signoListNode *next;
} signo_node;

typedef void(*sig_entry_func)(struct Trapframe *, int, void (*)(int), sigset_t*);

#define sigset_add(a, b, c) ({(a).sig[0] = ((b).sig[0] | (c).sig[0]) & (~0x40100); (a).sig[1] = (b).sig[1] | (c).sig[1];})
#define sigset_del(a, b, c) ({(a).sig[0] = (b).sig[0] & (~(c).sig[0]); (a).sig[1] = (b).sig[1] & (~(c).sig[1]);})
#define sigset_set(a, b) ({(a).sig[0] = (b).sig[0] & (~0x40100); (a).sig[1] = (b).sig[1];})
#define sigset_ctn(set, signo) ((set).sig[signo/32] & (1 << (signo&0x1f)))

void signo_init(void);
int signo_alloc(signo_node**, int);
void signo_free(signo_node**);

#endif
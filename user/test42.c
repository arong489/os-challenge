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
    

    // for (size_t i = 1; i < 64; i++) {
    //     if (!sigismember(&oldset, i)) {
    //         debugf("former has not %ld\n", i);
    //     }
    // }

	debugf("global = %d.\n", global);
	
    return 0;
}
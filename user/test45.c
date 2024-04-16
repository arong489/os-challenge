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
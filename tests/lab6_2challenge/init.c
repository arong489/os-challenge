#include <lib.h>
#define SIZE 4 * 1024 * 1024 // 开辟 4MB 的空间
#define STEP 4096
int get_tlength(u_int start_s, u_int start_us)
{
    u_int now_s, now_us;
    now_s = get_time(&now_us);
    int d = (((int)now_s) - ((int)start_s)) * 1000000;
    return ((int)now_us) - ((int)start_us) + d;
}
int main()
{
    int nums = 100;
    u_int start_s, start_us; // 用于计时的秒和毫秒
    start_s = get_time(&start_us);
    long pid = fork();
    if (pid == 0) {
        int time = 0;
        while (time < 1000000) {
            time = get_tlength(start_s, start_us);
        }
        char ch1[SIZE];
        memset(ch1, 0, SIZE);
        for (int i = 0; i < SIZE; i += STEP) {
            ch1[i] = 1;
            printk("%d", ch1[i]);
        }
        printk("\n");
        int time = get_tlength(start_s, start_us);
        printk("TLB重填%d遍 4MB 花费时长:%dus\n", nums, time);
    } else {
        int time = 0;
        char ch2[SIZE];
        memset(ch2, 0, SIZE);
        while (time < 5000000) {
            time = get_tlength(start_s, start_us);
        }
        for (int i = 0; i < SIZE; i += STEP) {
            printk("%d", ch2[i]);
        }
        printk("\n");
    }
    return 0;
}
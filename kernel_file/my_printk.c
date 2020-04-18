#include <linux/linkage.h>
#include <linux/kernel.h>
asmlinkage void sys_my_printk(int pid, unsigned long long start, unsigned long long end){
    unsigned long long pa_sec = start / 1000000000;
   	unsigned long long pa_nsec = start % 1000000000;
    unsigned long long pb_sec = end / 1000000000;
   	unsigned long long pb_nsec = end % 1000000000;
    printk("[Project1] %d %llu.%09llu %llu.%09llu\n", pid, pa_sec, pa_nsec, pb_sec, pb_nsec);
    return;
}
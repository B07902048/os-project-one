#include <linux/linkage.h>
#include <linux/kernel.h>
asmlinkage void sys_my_printk(int pid, long long start, long long end){
    long long pa_sec = start / 1000000;
   	long long pa_nsec = start % 1000000;
    long long pb_sec = end / 1000000;
   	long long pb_nsec = end % 1000000;
    printk("[Project1] %d %lld.%09lld %lld.%09lld\n", pid, pa_sec, pa_nsec, pb_sec, pb_nsec);
    return;
}
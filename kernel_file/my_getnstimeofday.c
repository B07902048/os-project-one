#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
asmlinkage long long sys_my_getnstimeofday(void){
    struct timespec now;
    getnstimeofday(&now);
    return (long long)now.tv_sec * 1000000 + now.tv_nsec;
}
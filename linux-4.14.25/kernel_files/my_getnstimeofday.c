#include <linux/time.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
asmlinkage long sys_my_getnstimeofday(struct timespec *ts){
    return getnstimeofday(ts);
}
asmlinkage long sys_print_time(pid_t pid, struct timespec *start, struct timespec *end){
	double time = double(end->tv_sec - start->tv_sec) + double(end->tv_nsec - start->tv_nsec) / 1000 / 1000;
	printk("%lf\n", time);
}
static int __init hello_init(void)
{
    printk(KERN_INFO "你好！核心模組！\n");
    return 0;
}

static void __exit hello_cleanup(void)
{
  printk(KERN_INFO "再見！核心模組！\n");
}

module_init(hello_init);
module_exit(hello_cleanup);
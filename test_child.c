#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//进程2 读取共享数据
int main(){
    int fd = shm_open("example", O_RDWR, 0777);
    if(fd < 0){//不存在共享内存对象
         printf("error open shm object\n");
         return 0;
    }
    char* ptr = (char*)mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    //getchar();
    printf("%s\n", ptr);
    return 0;
}
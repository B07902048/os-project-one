#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
//进程1，写入数据:write by t08
int main(){
    int fd = shm_open("example", O_CREAT|O_RDWR, 0777);
    if(fd < 0){ // 说明这个共享内存对象已经存在了
        fd = shm_open("example", O_RDWR, 0777);
        printf("open ok\n");
        if(fd < 0){
            printf("error open shm object\n");
            return 0;
        }
    }
    else{
        printf("create ok\n");
        // 说明共享内存对象是刚刚创建的
        ftruncate(fd, 1024); // 设置共享内存的尺寸
    }
    char* ptr = (char*)mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    strcpy(ptr, "write by t08");
    printf("print finish\n");
    //shm_unlink("/sz02-shm");
    return 0;
}
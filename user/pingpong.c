#include "kernel/types.h"
#include "user/user.h"

int main(){ 
    int p1[2],p2[2];//打开文件的文件描述符
    pipe(p1);//父to子管道
    pipe(p2);//子to父管道
    char outPipe1[10],outPipe2[10];
    int pid;
    pid=fork();//创建子进程
    if(pid>0){//父进程向p1管道写，从p2读
        write(p1[1],"y",1);//父进程写入1个字节到管道p1
        wait(0);//等待子进程退出
        if(read(p2[0],outPipe2,sizeof(outPipe2))==1)//从pipe2管道读出1个字节到outPipe2
            printf("%d:received pong\n",getpid());
        exit(0);
    }
    else{//子进程向p2管道写，从p1读
        write(p2[1],"y",1);//子进程写入1个字节到管道p1
        if(read(p1[0],outPipe1,sizeof(outPipe1))==1)//从pipe1管道读出1个字节到outPipe1
            printf("%d:received ping\n",getpid());
        exit(0);
    }
    return 0;
}
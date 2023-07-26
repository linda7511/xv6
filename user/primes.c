#include "kernel/types.h"
#include "user/user.h"
#include <stdbool.h>


bool if_prime(int num)
{
    for(int i=2;i<num/2;i++){
        if(num%i==0)
            return 0;
    }
    return 1;
}
int prime[36];
int main() {
    int p1[2];
    pipe(p1);
    int i;
    for (i = 2; i <= 35; i++) {
        write(p1[1], &i, sizeof(i));
        if(if_prime(i))
            prime[i]=1;
        else
            prime[i]=0;
    }
    i=0;
    write(p1[1], &i, sizeof(i));
    //close(p1[1]);
    while(1){
        int i,p2[2];
        pipe(p2);
        read(p1[0],&i,sizeof(i));
        printf("prime %d\n",i);
        if(read(p1[0],&i,sizeof(i))&&i){
            if (fork() == 0) {
                //子进程会复制一遍父进程的两个管道，其中父进程用来与祖父进程交流的管道应该被关闭
                close(p1[0]);
                close(p1[1]);
                //将父进程放到祖父位置，子进程放到两管道中间的原父进程位置
                p1[0] = p2[0];//这会把管道p2的内容复制给p1吗
                p1[1] = p2[1];
                continue;
            }
            do{//读左边的管道
                if(prime[i]){
                    write(p2[1],&i,sizeof(i));//写右边的管道
                }
            }while(read(p1[0],&i,sizeof(i))&&i);
            write(p2[1], &i, sizeof(i));//将0写入作为此次写入的结尾标志
        }
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        break;
    }
    wait(0);
    exit(0);
}
/*
bool if_prime(int num)
{
    for(int i=1;i<num/2;i++){
        if(num%i==0)
            return 0;
    }
    return 1;
}
int prime[36];
int main()
{
    int p1[2],p2[2];
    pipe(p1);
    for (int i = 2; i <= 35; i++) {
        write(p1[1], &i, sizeof(i));
        if(if_prime(i))
            prime[i]=1;
        else
            prime[i]=0;
    }
    close(p1[1]);
    
    while(1){
        int i;
        pipe(p2);
        read(p1[0],&i,sizeof(i));
        printf("prime %d\n",i);
        if(read(p1[0], &i, sizeof(i))!=0){
            if (fork() == 0) {
                //子进程会复制一遍父进程的两个管道，其中父进程用来与祖父进程交流的管道应该被关闭
                close(p1[0]);
                close(p1[1]);
                //将父进程放到祖父位置，子进程放到两管道中间的原父进程位置
                p1[0] = p2[0];//这会把管道p2的内容复制给p1吗
                p1[1] = p2[1];
                continue;
            }
            while(read(p1[0],&i,sizeof(i))!=0){//读左边的管道
                if(prime[i]){
                    write(p2[1],&i,sizeof(i));//写右边的管道
                }
            }
        }
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        break;
    }
    wait(0);
    exit(0);
    
    pid=fork();
    while(x!=31){
        if(pid>0){
            int printed=0,my_p_num=p_num,i;
            while(read(p[my_p_num][0],&i,sizeof(i))!=0){//读左边的管道
                if(prime[i]){
                    if(!printed){
                        printf("prime %d\n",i);
                        printed=1;
                        x=i;
                        pipe(p[++p_num]);
                        pid=fork();
                    }
                    write(p[my_p_num+1][1],&i,sizeof(i));//写右边的管道
                }
            }
            close(p[my_p_num][0]);
            close(p[my_p_num][1]);
            close(p[my_p_num+1][0]);
            close(p[my_p_num+1][1]);
            exit(0);
        }
        else{
            int printed=0,my_p_num=p_num,i;
            wait(0);
            while(read(p[my_p_num][0],&i,sizeof(i))!=0){
                if(!printed){
                    printf("prime %d\n",i);
                    printed=1;
                    x=i;
                    pipe(p[++p_num]);
                }
                write(p[my_p_num+1][1],&i,sizeof(i));
            }
            close(p[my_p_num][0]);
            close(p[my_p_num][1]);
            close(p[my_p_num+1][0]);
            close(p[my_p_num+1][1]);
            exit(0);
        }
    }
    return 0;
}
*/
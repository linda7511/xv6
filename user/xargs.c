#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include <stddef.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "Usage: xargs command [-n numargs]\n");
        exit(1);
    }
    char *command;
    int n_set = 0; // 默认为 0,读取
    if (argc > 3 && strcmp(argv[1], "-n") == 0) {//获取可选的 -n 参数
        n_set = 2;
        command=argv[3];
    }
    else
        command=argv[1];
    char line[64], *xargs[32];//分别用于存储从标准输入读取的行的字符数组和存储命令及参数
    for (int i = 1+n_set; i < argc; i++){//将命令行参数拷贝到xargs数组，用于后续的exec调用
        xargs[i - 1-n_set] = argv[i];
        //printf("%d-%s-%s\n",i,argv[i],xargs[i-1-n_set]);
    }
    char *p=line;
    while ((gets(line, 64) &&line[0])||*p) {
        // 执行命令
        //printf("line:%s  p:%c\n",line,*p);
        if(!*p)
            p=line;
        int i=argc-1-n_set;
        xargs[i++]=p;
        xargs[i] = NULL;  // 在参数列表的末尾设置为 NULL，表示参数结束
        //printf("xargs%d-%s\n",i-1,xargs[i-1]);
        for (; *p;p++ ) {
            //printf("p0:%c\n",*p);
            while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') {
                if(*p=='\\'&&*(p+1)=='n'){
                    *p++=0;
                    break;
                }
                if(*p=='"'&&n_set){
                    if(*(p+1)==0)//结尾处的"
                        *p=0;
                    else if(i==argc-n_set)//是从输入行中获得的第一个参数前的"
                        xargs[i-1] = p + 1; 
                }
                //printf("p:%c\n",*p);
                p++;
            }
            if (*p == '\0') {
                break;
            }
            if(*p=='n'){
                *p = '\0';
                //printf("xargs%d-%s\n",i-1,xargs[i-1]);
                p++;
                break;
            }
            *p=0;
            xargs[i++] = p + 1;//指向下一个参数
            //printf("xargs%d-%s\n",i-2,xargs[i-2]);
            //printf("xargs%d-%s\n",i-1,xargs[i-1]);
            /*
            if (*p == ' ') {
                *p = 0;//分隔参数
                if(!n_set){
                    xargs[i++] = p + 1;//指向下一个参数
                    printf("xargs0%d-%s\n",i-2,xargs[i-2]);
                    printf("xargs0%d-%s\n",i-1,xargs[i-1]);
                }
            } else if (*p == '\n') {
                *p = '\0';
                printf("p:%s\n",*p);
                p++;
                printf("p+1:%s\n",*p);
                //xargs[i++] = p + 1;//指向下一个参数
                printf("xargs1%d-%s\n",i-1,xargs[i-1]);
                //printf("xargs1%d-%s\n",i-1,xargs[i-1]);
                if(n_set){
                    printf("p+1-%s\n",*(p+1));
                    xargs[i++] = p + 1;//指向下一个参数
                    printf("xargs1%d-%s\n",i-1,xargs[i-1]);
                }
            }
            */
        }
        xargs[i] = NULL;  // 在参数列表的末尾设置 NULL 结束符
        int pid = fork();
        if (pid < 0) {
            fprintf(2, "fork failed\n");
            exit(1);
        }else if (pid == 0) {
            // 子进程
            //printf("pid==0");
            exec(command, xargs);
            fprintf(2, "exec failed\n");
            exit(0);
        }
    }
    wait(0);
    exit(0);
}
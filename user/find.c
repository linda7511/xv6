#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* target;

//从给定的路径中提取文件或目录的名称，并对名称进行格式化处理。
char* fmtname(char *path)
{
    static char buf[DIRSIZ+1];//用于存储格式化后的名称
    char *p;

    // 从路径的末尾向前搜索，直到找到最后一个斜杠字符（/）为止
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    //将名称复制到 buf 中，并在末尾填充空格，使其长度达到 DIRSIZ
    memmove(buf, p, strlen(p));
    buf[strlen(p)]='\0';
    return buf;
}

void find(char *path) {
    int fd;
    struct stat st;//用于获取文件或目录的详细信息
    struct dirent de;//用于存储目录项的信息
    char buf[512], *p;
    //尝试打开指定路径的目录，如果失败则打印错误消息并返回
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    //获取目录的状态信息，包括类型、inode 号和大小
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
        case T_FILE:
            //当前路径的文件名就是目标文件名，则输出当前路径
            if (strcmp(fmtname(path), target) == 0) {
                printf("%s\n", path);
            }
            break;
        case T_DIR:
            //检查拼接路径后是否超过缓冲区大小，如果是则打印错误消息并跳出循环
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);//指向路径末尾
            *p++ = '/';//为路径添加一个斜杠字符，表示进入该目录
            //从打开的目录文件描述符中读取目录项信息，直到读取完所有的目录项
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                //排除非空目录项和当前目录项、上级目录的项
                    continue;
                memmove(p, de.name, DIRSIZ);//将读到的项的名称拷贝到buf的末尾
                p[DIRSIZ] = 0;//设置'\0'终止符，形成以null终止的字符串
                find(buf);//递归处理该目录
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find path filename\n");
        exit(1);
    }
    target = argv[2];
    find(argv[1]);
    exit(0);
}
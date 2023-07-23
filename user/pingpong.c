#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[])
{
    int fd[2];//存放，fd[0]读取数据，fd[1]写入数据

    //创建pipe
    if (pipe(fd) == -1) {
        fprintf(2, "Error: pipe(fd) error.\n");
    }
    else {
        if (fork() != 0) {  //判断当前是父进程，如果是父进程会返回进程id，子进程返回0
            char buffer[1];
            buffer[0] = 'a';
            write(fd[1], buffer, 1);
            close(fd[1]);
            read(fd[0], buffer, 1);
            fprintf(0, "%d: received pong\n", getpid());
        }
        else {  //当前是子进程
            char buffer[1]; //存储从管道中读取或写入的数据
            read(fd[0], buffer, 1);
            close(fd[0]);
            fprintf(0, "%d: received ping\n", getpid());
            write(fd[1], buffer, 1); // 将数据写入管道
        }
    }
    exit(0);
}
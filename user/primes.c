#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void create_number_process(int p[2]) {
    int prime;
    int n;
    close(p[1]);   //关闭写端
    if (read(p[0], &prime, 4) != 4) {
        fprintf(2, "Error in read.\n");
        exit(1);
    }
    printf("prime %d\n", prime);
    if (read(p[0], &n, 4) == 4) {  //返回的不是0，说明还要继续传送筛选，创建新进程
        int new_pipe_fd[2];
        pipe(new_pipe_fd);
        if (fork() != 0) {
            close(new_pipe_fd[0]);
            if (n % prime) write(new_pipe_fd[1], &n, 4);
            while (read(p[0], &n, 4) == 4) {
                if (n % prime) write(new_pipe_fd[1], &n, 4);
            }
            close(p[0]);
            close(new_pipe_fd[1]);
            wait(0);
        }
        else {
            create_number_process(new_pipe_fd);
        }
    }
}

int
main(int argc, char* argv[])
{
    int p[2];
    pipe(p);
    if (fork() != 0) {
        close(p[0]);  //关闭读端
        for (int i = 2; i <= 35; ++i) {
            if (write(p[1], &i, 4) != 4) {
                fprintf(2, "failed write %d into the pipe\n", i);
                exit(1);
            }
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
    else {
        create_number_process(p);        
    }
    return 0;
}
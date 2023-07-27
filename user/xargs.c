#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char* argv[])
{
    char* command = argv[1];  //存储要接收所传递的参数的指令（子进程）
    char rec[MAXARG];
    memset(rec, 0, sizeof(rec));   //存储读取的字符
    int index = 0;  //索引
    char ch;
    while (read(0, &ch, sizeof(char))){
        if (ch == '\n'){  //换行       
            char* child_argv[4];
            rec[index] = 0;  //最后一个元素设置为0，表示字符串结束
            //将读到的部分传给子进程
            child_argv[0] = command;
            child_argv[1] = argv[2];
            child_argv[2] = rec;
            child_argv[3] = 0;
            int pid = fork();  //创建子进程
            if (pid != 0){            
                wait(0);
            }
            else{           
                exec(child_argv[0], child_argv);  //执行指令               
            }
            index = 0;  // 重置索引以便下一次读取
        }
        else{
            rec[index++] = ch;
        }
    }
    exit(0);
}

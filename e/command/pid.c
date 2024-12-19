#include "stdio.h"

int main() {
    int pid = getpid();  // 获取当前进程的 PID
    printf("Current process PID: %d\n", pid);  // 打印 PID
    return 0;
}

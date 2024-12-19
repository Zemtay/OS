#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

PUBLIC int kill(int pid) {
    if (pid < 0 || pid >= NR_TASKS + NR_PROCS) {
        printf("Invalid PID: %d\n", pid);
        return -1;
    }

    MESSAGE msg;
    msg.PID = pid;
    msg.type = KILL;

    send_recv(BOTH, TASK_MM, &msg);  // 发送请求给 MM

    if (msg.type == SYSCALL_RET) {
        printf("Process %d killed successfully.\n", pid);
        return 0;
    } else {
        printf("Failed to kill process %d.\n", pid);
        return -1;
    }
}
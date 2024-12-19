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

PUBLIC void getpinfo() {
    MESSAGE msg;
    struct proc p;

    // 打印表头
    printf("%5s %8s %5s %10s\n", "PID", "NAME", "PPID", "STAT");

    for (int i = 0; i < NR_TASKS + NR_PROCS; i++) {
        msg.PID = i;
        msg.type = GET_PROC_INFO;
        msg.BUF = &p;
        send_recv(BOTH, TASK_SYS, &msg);  // 与系统进行消息传递

        if (p.p_flags != FREE_SLOT) {
            // 输出PID和NAME
            printf("%5d %8s ", i, p.name);

            // 输出PPID
            if (p.p_parent == NO_TASK) {
                printf("%5s ", "?");
            } else {
                printf("%5d ", p.p_parent);
            }

            // 输出STAT
            if (p.p_flags == SENDING) {
                printf("%10s\n", "Sending");
            } else if (p.p_flags == RECEIVING) {
                printf("%10s\n", "Receiving");
            } else if (p.p_flags == WAITING) {
                printf("%10s\n", "Waiting");
            } else if (p.p_flags == HANGING) {
                printf("%10s\n", "Hanging");
            } else {
                printf("%10s\n", "Unknown");
            }
        }
    }
}

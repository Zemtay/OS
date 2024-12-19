#include "type.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: kill <PID>\n");
        return -1;
    }

    int target_pid = atoi(argv[1]);  // 将传入的 PID 转为整数

    kill(target_pid);

    return 0;
}
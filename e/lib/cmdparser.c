// clang-format off
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

#include "cmdparser.h"
// clang-format on
int parse_cmdline(int argc, char *argv[], CmdOption *options, int opt_count, CmdParserResult *result) {
    result->positional_count = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { // 检查是否为选项
            int matched = 0;
            for (int j = 0; j < opt_count; j++) {
                if ((options[j].short_opt && strcmp(argv[i], options[j].short_opt) == 0) ||
                    (options[j].long_opt && strcmp(argv[i], options[j].long_opt) == 0)) {
                    matched = 1;
                    if (options[j].type == OPT_TYPE_FLAG) {
                        *(int *)(options[j].value) = 1; // 设置标志位为 1
                    } else if (options[j].type == OPT_TYPE_VALUE) {
                        if (i + 1 < argc) {
                            *(char **)(options[j].value) = argv[++i]; // 获取值
                        } else {
                            printf("Error: Missing value for option %s\n", argv[i]);
                            return -1;
                        }
                    }
                    break;
                }
            }
            if (!matched) {
                printf("Error: Unknown option %s\n", argv[i]);
                return -1;
            }
        } else {
            // 非选项，存储为位置参数
            if (result->positional_count < 16) {
                result->positional_args[result->positional_count++] = argv[i];
            } else {
                printf("Error: Too many positional arguments (maximum 16).\n");
                return -1;
            }
        }
    }
    return 0;
}

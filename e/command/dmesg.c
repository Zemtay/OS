// clang-format off
#include "cmdparser.h"
#include "stdio.h"
#include "string.h"
#include "logfilament.h"
// clang-format on

int main(int argc, char *argv[]) {
	char dir[256]; // 目前的工作路径

	// 解析参数
	char option_lev[8]  = "\0"; // 递归删除
	option_lev[0]       = '0' - 1;
	CmdOption options[] = {
		{"-l", "--level", OPT_TYPE_VALUE, option_lev},
	};

	CmdParserResult result;
	if (parse_cmdline(argc, argv, options, 2, &result) != 0) {
		printf("Error parsing command line arguments.\n");
		return 1;
	}

    int level = -1;
    if (argc == 3) {
        level = argv[2][0] - '0';
    }


	if (level < 0) {
		// 执行显示的逻辑
		int tail = 10;
		// 寻找位置参数
		if (result.positional_count == 1) {
			char* num_str = result.positional_args[0];
			tail         = atoi(num_str);
		}
		PrintLogTail_User(tail);
	} else {
		// 执行切换记录级别的逻辑
		SwitchLogLevel_User(level);
        printf("log level has switched to %d\n", level);
	}
}

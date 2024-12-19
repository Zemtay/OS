#include "cmdparser.h"
#include "mdirent.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

#ifndef NULL
#define NULL 0
#endif

int main(int argc, char *argv[]) {
	char dir[256]; // 目前的工作路径
	getcwd(dir, 256);

	// 解析参数
	int option_l = 0, option_a = 0; // 详细 / 显示隐藏文件
	CmdOption options[] = {
		{"-l", "--long", OPT_TYPE_FLAG, &option_l},
		{"-a", "--all", OPT_TYPE_FLAG, &option_a},
	};

	CmdParserResult result;
	if (parse_cmdline(argc, argv, options, 3, &result) != 0) {
		printf("Error parsing command line arguments.\n");
		return 1;
	}

	if (result.positional_count > 1) {
		printf("Error `ls` command only support 1 position arg at most\n");
		return 1;
	} else if (result.positional_count == 1) {
		get_full_path(result.positional_args[0], dir);
	}

	// 业务
	printf("Directory: %s\n", dir);
	DIR d; // 目录结构体
	// struct dirent entry; // 当前目录项

	char buff[13] = {"\0"}; // 文件名
	int ret;

	// 打开目录
	ret = opendir(dir, &d);
	if (ret != 0) {
		printf("Failed to open directory: %s\n", dir);
		return 1;
	}

	// 遍历并打印目录中的所有文件名
	size_t cnt = 0; // count entry number
	while (1) {
		// 读取下一个目录项
		readdir(&d, buff);

		// 如果没有更多目录项，退出循环
		if (buff[0] == '\0') {
			break;
		}

		if (buff[0] != '.' || option_a) { // 是否显示
			cnt++;
			if (option_l) { // 详细显示
				// fs stat
				struct stat f_stat;
				char f_path[256] = {'\0'};
				get_full_path(buff, f_path);
				printf("%s", f_path);
				stat(f_path, &f_stat);
				int f_size      = f_stat.st_size;
				char mode_str[] = "...";
				if (f_stat.st_mode == 040000) {
					mode_str[0] = 'd';
					mode_str[1] = 'w';
					mode_str[2] = 'r';
				}
				if (f_stat.st_mode == 0100000) {
					mode_str[1] = 'w';
					mode_str[2] = 'r';
				}
				printf("%s %d %s", mode_str, f_size, buff);
				printf("\n");
			} else {
				printf("%s  ", buff);
				if (cnt % 5 == 4) {
					printf("\n");
				}
			}
		}
	}

	// 关闭目录
	closedir(&d);
	printf("\n");
	return 0;
}
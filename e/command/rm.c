// clang-format off
#include "cmdparser.h"
#include "mdirent.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

// clang-format on
int recursive_delete(char *dir);

int main(int argc, char *argv[]) {
	char dir[256]; // 目前的工作路径

	// 解析参数
	int option_r        = 0; // 递归删除
	CmdOption options[] = {
		{"-r", "--recursive", OPT_TYPE_FLAG, &option_r},
	};

	CmdParserResult result;
	if (parse_cmdline(argc, argv, options, 2, &result) != 0) {
		printf("Error parsing command line arguments.\n");
		return 1;
	}

	if (result.positional_count != 1) {
		printf("Error `rm` command only support 1 position arg\n");
		return 1;
	} else if (result.positional_count == 1) {
		get_full_path(result.positional_args[0], dir);
	}

	struct stat f_stat;
	int ret = stat(dir, &f_stat);
	if (ret != 0) {
		printf("ERROR stat failed\n");
	}

	switch (f_stat.st_mode) {
	case 040000:
		// dir
		if (option_r) {
			// 递归删除
			return recursive_delete(dir);
		} else {
			printf("ERROR It is a directory, use `-r` arg\n");
		}
		break;
	case 0100000:
		// file
		return unlink(dir);
		break;

	default:
		// 不敢删
		printf("ERROR Not a regular file\n");
		break;
	}
}

int recursive_delete(char *dir) {
	DIR d;
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

		if (strcmp(buff, ".") != 0) {
			cnt++;
			// fs stat
			struct stat f_stat;
			char f_path[256] = {'\0'};
			strcat(f_path, dir);
            strcat(f_path, "/");
            strcat(f_path, buff);
			stat(f_path, &f_stat);

			if (f_stat.st_mode == 040000) {
				recursive_delete(f_path);
			} else if (f_stat.st_mode == 0100000) {
                printf("1");
				unlink(f_path);
			} else {
				printf("ERROR irregular entry %s=%d\n", f_path, f_stat.st_mode);
				closedir(&d);
				return 1;
			}

			printf("delete %s\n", f_path);
		}
	}
	closedir(&d);
    // printf("now delete dir %s\n", dir);
    // unlink(dir); //this file
	return 0;
}

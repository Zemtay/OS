#include "stdio.h"
#include "string.h"
#include "type.h"
#include "unistd.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("ERROR: `mkdir` allows invocation with only one parameter.\n");
		return 1;
	}

	char dir[MAX_PATH] = {'\0'};
	char buf[128]      = {'\0'};
	GetCwd(buf);

	// 拼接目录生成即将的新地址
	get_full_path(argv[1], dir);

	struct stat f_stat;
	int ret = stat(dir, &f_stat);
	if (ret != 0 || f_stat.st_ino == 0) {
		// error
		printf("ERROR stat() returns error. %s\n", dir);
		return -1;
	}
	// os.stat一下看看是否为directory
	if (f_stat.st_mode != 040000) {
		printf("ERROR target entry is not a directory\n");
		return -1;
	}

	// 切换
	chdir(dir);
	return 0;
}
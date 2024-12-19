#include "stdio.h"
#include "string.h"
#include "type.h"
#include "unistd.h"

int main(int argc, char *argv[]) {
	// only accept one arg
	if (argc != 2) {
		printf("ERROR: `mkdir` allows invocation with only one parameter.\n");
		return 1;
	}

	char dir[256];
	// 处理第一个参数即软件路径

	get_full_path(argv[1], dir);

	int res = mkdir(dir, O_CREAT);
	if (res == -1) {
		printf("ERROR create directory %s fail\n", dir);
		return 1;
	}
	return 0;
}
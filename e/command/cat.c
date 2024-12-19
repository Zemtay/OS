// clang-format off
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "keywise.h"

// clang-format on
static char buf[8192];     // 文件缓冲


int filesize; // 文件大小,可以被拓展但是不能超过8190

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("ERROR need a file");
		return 1;
	}
	char filepath[256] = "";
	get_full_path(argv[1], filepath);

	struct stat f_stat;
	int ret = stat(filepath, &f_stat);

	if (ret != 0 || f_stat.st_mode != 0100000) {
		printf("ERROR can not get target file or not a regular file: %s\n", filepath);
        return 1;
	}

	int fd = open(filepath, O_RDWR);
	if (fd == 0) {
		printf("can not open file: %s\n", filepath);
        return 1;
	}

	ret = read(fd, buf, 8190);

	if (ret == -1) {
		printf("ERROR can not read file: %s\n", filepath);
        return 1;
	} else {
		printf("read %d bytes from %s\n", ret, filepath);
	}

	filesize = strlen(buf);

	printf("%s", buf);

	close(fd);

	// PrintLogTail_User(2048);
	return 0;
}
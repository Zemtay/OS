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
#include "keyboard.h"
#include "proto.h"
#include "shell.h"

// clang-format on

/**
 * @return ret value
 */
PUBLIC int do_shell() {
	int type     = mm_msg.FLAGS; // 处理方法
	int src      = mm_msg.source;
	int name_len = mm_msg.NAME_LEN;
	int ret;

	char target[BYTES_SHELL_WORKING_DIRECTORY];
	switch (type) {
	case SHELL_INIT:
		phys_copy((void *)va2la(TASK_MM, target), (void *)va2la(src, mm_msg.PATHNAME), name_len);
		target[name_len] = 0;
		return initShell(target);
		break;
	case SHELL_CHDIR:
		phys_copy((void *)va2la(TASK_MM, target), (void *)va2la(src, mm_msg.PATHNAME), name_len);
		target[name_len] = 0; /* terminate the string */
		return changeCWD(target);
		break;
	case SHELL_GETCWD:
		ret      = getCWD(target);
		name_len = strlen(target) + 1;
		phys_copy((void *)va2la(src, mm_msg.BUF), (void *)va2la(TASK_MM, target), name_len);
		return ret;
	default:
		return 1;
	}

	return 0;
}

int initShell(char *root) {
	_strcpy(Working_Directory, root);
	return 0;
}

/**
 * @return status
 *
 *  - 1: 无目标entry
 *
 *  - 2: 目标非directory
 */
int changeCWD(char *path) {
	char* buf = path;
	// 先获得目标路径的绝对路径
	printx("{mm_SHELL} CD=");
	printx(Working_Directory);

	// 判断 buf 是否为目录
	if (strcmp(buf, "/") == 0) {
		_strcpy(Working_Directory, "/");
		return 0;
	}
	struct stat f_stat;
	int ret = stat(buf, &f_stat);
	if (ret != 0) {
		printx("{mm_SHELL} can not find target dir entry");
		return 1;
	}
	if (f_stat.st_mode != I_DIRECTORY) {
		return 2;
	}
	_strcpy(Working_Directory, buf);
	return 0;
}

int getCWD(char *buf) {
	_strcpy(buf, Working_Directory);
}
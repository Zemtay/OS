/**
 * 提供接口unistd.h
 * 底层与mm通信交换TYPESHELL信息
 */

// clang-format off
#include "type.h"
#include "mdirent.h"
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
#include "hd.h"
#include "fs.h"

#include "unistd.h"
// clang-format on
#ifndef NULL
#define NULL 0
#endif

PUBLIC void Init_Shell(char *workd) {
	MESSAGE mm_msg;
	mm_msg.type     = TYPESHELL;
	mm_msg.FLAGS    = SHELL_INIT;
	mm_msg.PATHNAME = workd;
	mm_msg.NAME_LEN = strlen(workd);

	send_recv(BOTH, TASK_MM, &mm_msg);
}

PUBLIC int chdir(char *path) {
	MESSAGE mm_msg;
	mm_msg.type     = TYPESHELL;
	mm_msg.FLAGS    = SHELL_CHDIR;
	mm_msg.PATHNAME = path;
	mm_msg.NAME_LEN = strlen(path);

	send_recv(BOTH, TASK_MM, &mm_msg);
	printl("{chdir status}%d\n", mm_msg.RETVAL);

	return mm_msg.RETVAL;
}

PUBLIC char *getcwd(char *buf, size_t size) {
	MESSAGE mm_msg;
	mm_msg.type  = TYPESHELL;
	mm_msg.FLAGS = SHELL_GETCWD;
	mm_msg.BUF   = buf;

	send_recv(BOTH, TASK_MM, &mm_msg);

	return buf;
}

PUBLIC void extractExecDir(char *full_path, char *buf) {
	// 处理第一个参数即软件路径

	char *last_slash = NULL; // 用于存储最后一个 '/' 的位置

	// 遍历字符串找到最后一个 '/' 字符
	for (char *ptr = full_path; *ptr != '\0'; ptr++) {
		if (*ptr == '/') {
			last_slash = ptr;
		}
	}
	// 如果找到了 '/'，则截取目录部分
	if (last_slash != NULL) {
		// 用指针操作来截取路径

		for (int i = 0; i < last_slash - full_path; i++) {
			buf[i] = full_path[i];
		}
		buf[last_slash - full_path] = '\0'; // 添加字符串结束符
	} else {
		// 如果没有找到 '/', 说明路径没有目录部分
		printl("{SHELL} No directory in path\n");
	}
	if (buf[0] == '\0') { // 根目录'/'
		buf[0] = '/';
		buf[1] = '\0';
	}
}

PUBLIC void get_full_path(char *path, char *buf) {
	const int MAX_LEN        = 4096;
	const int MAX_COMPONENTS = 1024;
	char path_copy[MAX_LEN];
	char working_copy[MAX_LEN];
	char *components[MAX_COMPONENTS];
	int top = 0;

	char working_path[MAX_LEN];
	getcwd(working_path, MAX_LEN);

	// 拷贝输入参数到本地缓冲区
	_strncpy(path_copy, path, MAX_LEN - 1);
	path_copy[MAX_LEN - 1] = '\0';

	if (path[0] == '/') {
		// 绝对路径，从根目录开始
		top = 0;
	} else {
		// 相对路径，从working_path开始
		_strncpy(working_copy, working_path, MAX_LEN - 1);
		working_copy[MAX_LEN - 1] = '\0';

		char *token = strtok(working_copy, "/");
		while (token && top < MAX_COMPONENTS) {
			components[top++] = token;
			token             = strtok(NULL, "/");
		}
	}

	// 分解输入path进行处理
	{
		char *token = strtok(path_copy, "/");
		while (token) {
			if (strcmp(token, ".") == 0) {
				// "."不处理
			} else if (strcmp(token, "..") == 0) {
				// ".."回到上一级目录
				if (top > 0) {
					top--;
				} else {
					// 已经在根目录还要向上返回，则结果为"/"
					_strcpy(buf, "/");
					return buf;
				}
			} else {
				// 普通目录名
				if (top < MAX_COMPONENTS) {
					components[top++] = token;
				} else {
					// 超出最大目录分量数量，简单处理
					_strcpy(buf, "/");
					return buf;
				}
			}
			token = strtok(NULL, "/");
		}
	}

	// 根据components重构绝对路径
	if (top == 0) {
		// 空表示根目录
		_strcpy(buf, "/");
	} else {
		buf[0] = '\0';
		for (int i = 0; i < top; i++) {
			strcat(buf, "/");
			strcat(buf, components[i]);
		}
	}

	return buf;
}
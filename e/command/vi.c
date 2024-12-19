// clang-format off
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "keywise.h"

// clang-format on

#define MAXFILESIZEBUF 8190
#define BUF_SIZE 8190

// 全局变量
char file_buf[BUF_SIZE]              = {0}; // 文件缓冲区
char disp_buf[SCR_HEIGHT][SCR_WIDTH] = {0}; // 屏幕显示缓冲区
int cursor_x = 0, cursor_y = 0;             // 光标位置
int file_size  = 0;                         // 文件实际大小
int fpos_first = 0;                         // 当前显示区域在file_buf中的第一个字符位置
int mode       = 0;                         // 模式: 0=普通模式, 1=insert模式

int exit_status = 0;

void refresh_screen();
void display_status();
void update_display();
void insert_char(int ch);
void update_cursor();

// 初始化屏幕
void init_vi() {
	memset(disp_buf, 0, sizeof(disp_buf));
	mode       = 0;
	cursor_x   = 0;
	cursor_y   = 0;
	file_size  = 0;
	fpos_first = 0;
	update_display();
}

// void dump_to_file_buf() {
// 	int pos = 0;

// 	for (int y = 0; y < SCR_HEIGHT - 1; y++) {
// 		for (int x = 0; x < SCR_WIDTH; x++) {
// 			if (disp_buf[y][x] == 0) {
// 				break;
// 			}
// 			file_buf[pos++] = disp_buf[y][x];
// 		}
// 		file_buf[pos++]= '\n';
// 	}

// }

int cursorToPos() {
	int ret     = fpos_first;
	int okenter = cursor_y;
	for (; okenter > 0 && ret < 8190; ret++) {
		if (file_buf[ret] == '\n') {
			okenter--;
		}
	}

	int wid = cursor_x;
	for (; wid > 0 && ret < 8190; ret++) {
		wid--;
	}
	return ret;
}

int cursorToPosW(int cx, int cy) {
	int ret     = fpos_first;
	int okenter = cy;
	for (; okenter > 0 && ret < 8190; ret++) {
		if (file_buf[ret] == '\n') {
			okenter--;
		}
	}

	int wid = cx;
	for (; wid > 0 && ret < 8190; ret++) {
		wid--;
	}
	return ret;
}

// 重置显示区域
void update_display() {
	memset(disp_buf, 0, sizeof(disp_buf));
	int pos = fpos_first;
	for (int y = 0; y < SCR_HEIGHT - 1; y++) {
		for (int x = 0; x < SCR_WIDTH; x++) {
			if (file_buf[pos] == '\0') {
				file_size = pos;
				refresh_screen();
				return;
			}
			if (file_buf[pos] == '\n') {
				pos++;
				break;
			}
			disp_buf[y][x] = file_buf[pos++];
		}
	}
	refresh_screen();
}

// 刷新屏幕内容到控制台
void refresh_screen() {
	_scr_putscr(disp_buf);
	display_status();
	update_cursor();
}

// 显示状态栏 (最下面一行)
void display_status() {
	if (mode == 0)
		_scr_setbottom(3);
	else
		_scr_setbottom(1);
}

// 更新光标位置
void update_cursor() {
	_scr_cursor_set(cursor_x, cursor_y);
}

void nextN(int aspect) {
	int start = fpos_first + aspect;
	while (start == -1 || file_buf[start] != '\n') {
		start += aspect;
	}
	fpos_first = start + 1;
}

// 在insert模式中插入字符
void insert_char(int ch) {
	int pos = cursorToPos();
	if (file_size >= BUF_SIZE - 1)
		return; // 文件大小限制

	if (ch == ENTER) { // ENTER键处理
		for (int i = file_size; i > pos; i--) {
			file_buf[i] = file_buf[i - 1];
		}
		file_buf[pos] = '\n';
		file_size++;
		cursor_x = 0;
		cursor_y++;
		if (cursor_y >= SCR_HEIGHT - 1) {
			nextN(+1);
			cursor_y--;
		}
	} else if (ch == BACKSPACE) { // BACKSPACE处理
		if (pos > 0) {
			// 删除当前位置的前一个字符
			for (int i = pos - 1; i < file_size - 1; i++) {
				file_buf[i] = file_buf[i + 1];
			}
			file_buf[file_size - 1] = '\0'; // 清除最后一个字符
			file_size--;

			if (cursor_x > 0) {
				cursor_x--;
			} else if (cursor_y > 0) {
				cursor_y--;
				cursor_x = SCR_WIDTH - 1;
				while (cursor_x > 0 && disp_buf[cursor_y][cursor_x] == 0) {
					cursor_x--;
				}
			} else if (fpos_first > 0) {
				nextN(-1);
				cursor_y = 0;
				cursor_x = SCR_WIDTH - 1;
			}
		}
	} else if (ch >= 32 && ch <= 126) { // 普通字符处理
		for (int i = file_size; i > pos; i--) {
			file_buf[i] = file_buf[i - 1];
		}
		file_buf[pos] = (char)ch;
		file_size++;
		cursor_x++;
		if (cursor_x >= SCR_WIDTH) { // 换行处理
			cursor_x = 0;
			cursor_y++;
			if (cursor_y >= SCR_HEIGHT - 1) {
				nextN(+1);
				cursor_y--;
			}
		}
	}
	update_display();
	// update_cursor();
}

// 处理光标移动逻辑
void move_cursor(char direction) {
	switch (direction) {
	case 'h': // 左移
		if (cursor_x > 0)
			cursor_x--;
		break;
	case 'l': // 右移
		if (cursor_x < SCR_WIDTH - 1 && disp_buf[cursor_y][cursor_x + 1] != 0)
			cursor_x++;
		break;
	case 'j': // 下移
		if (cursor_y < SCR_HEIGHT - 2) {
			cursor_y++;
			if (file_buf[cursorToPos()] == '\0') { // TODO
				cursor_y--;
			}
		} else if (cursor_y == SCR_HEIGHT - 2) {
			int next_line_pos = cursorToPosW(SCR_WIDTH - 1, SCR_HEIGHT - 1);
			while (next_line_pos < file_size && file_buf[next_line_pos] != '\n') {
				next_line_pos++;
			}
			if (next_line_pos < file_size) {
				next_line_pos++;
				fpos_first = next_line_pos;
				update_display();
			}
		}
		break;
	case 'k': // 上移
		if (cursor_y > 0) {
			cursor_y--;
		} else if (fpos_first > 0) {
			int prev_line_pos = fpos_first - 1;
			while (prev_line_pos > 0 && file_buf[prev_line_pos - 1] != '\n') {
				prev_line_pos--;
			}
			fpos_first = prev_line_pos;
			update_display();
		}
		break;
	}
	update_cursor();
}

// 运行vi逻辑
void run_vi() {
	int ch;
	while (1) {
		ch = _getch();       // 获取输入字符，阻塞等待
		if (mode == 0) {     // NORMAL模式
			if (ch == ';') { // 示例退出命令
				char cmd[5] = "\0\0\0\0\0";
				int cmd_pos = 0;
				_scr_setbottom(2); // 底线显示':'
				int status = 0;    // 2=normal, 1=exec cmd

				while (status == 0) {
					ch = _getch();
					if (IsExt(ch)) {
						switch (ch) {
						case ENTER:
							status = 1;
							break;
						case ESC:
						case BACKSPACE:
							status = 2;
							break;
						default:
							status = 0;
							break;
						}
					}
					if (status != 0) {
						break;
					}
					cmd[cmd_pos++] = ch;
					_scr_putch_bottom(ch);
				}
				if (status == 2) {
					mode = 0;
					_scr_setbottom(3); // 底线显示'normal'
					continue;
				} else if (status == 1) {
					if (strcmp(cmd, "q1") == 0) {
						exit_status = 0;
						return;
					} else if (strcmp(cmd, "wq") == 0) {
						exit_status = 1;
						return;
					}
					_scr_setbottom(2); // 底线显示':'
				}
			} else if (ch == 'h' || ch == 'j' || ch == 'k' || ch == 'l') {
				move_cursor(ch); // 移动光标
			} else if (ch == 'i') {
				mode = 1; // 切换到insert模式
				display_status();
			} else if (ch == 'a') {
				mode = 1; // 切换到insert模式
				do {
					if (file_buf[cursorToPos()] == '\0') {
						break;
					}
					if (cursor_x < SCR_WIDTH - 1)
						cursor_x++;
				} while (0);

				display_status();
			}
		} else if (mode == 1) { // INSERT模式
			if (ch == ESC) {    // ESC键
				mode = 0;       // 退回普通模式
				display_status();
			} else {
				insert_char(ch);
			}
		}
	}
}

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

	memset(file_buf, 0, 8189);
	ret = read(fd, file_buf, 8190);

	if (ret == -1) {
		printf("ERROR can not read file: %s\n", filepath);
		return 1;
	} else {
		printf("read %d bytes from %s\n", ret, filepath);
	}

	file_size = strlen(file_buf);
	printf("\n%d", file_size);
	_getch();

	_scr_clear(); // 清屏
	init_vi();    // 初始化vi编辑器
	run_vi();     // 运行vi逻辑
	_scr_init();  // 恢复初始状态

	file_buf[file_size] = '\0';

	if (exit_status == 1) {
		lseek(fd, 0, SEEK_SET);
		ret = write(fd, file_buf, file_size + 1);
		if (ret == -1) {
			printf("ERROR can not write file: %s\n", filepath);
			return 1;
		} else {
			printf("write %d bytes from %s\n", ret, filepath);
		}
	}

	close(fd);
	return 0;
}

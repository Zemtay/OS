// clang-format off
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "keywise.h"

// clang-format on

#define SCR_SIZE (80 * 25)
#define SCR_WIDTH 80
#define SCR_HEIGHT 25
#define MAXFILESIZEBUF 8190

static char buf[8192];     // 文件缓冲
int fpos_first, fpos_last; // 文件在显示区域中显示的第一个字符在文件缓冲的偏移与最后一个字符在文件缓冲中的偏移

char disp_buf[SCR_HEIGHT][SCR_WIDTH]; // 显示区域
int cursor_x, cursor_y;               // 光标,x 为height, y为width

int filesize; // 文件大小,可以被拓展但是不能超过8190

void putchar(char ch) {
	printf("%c", ch);
	buf[fpos_first] = ch;
	fpos_first++;
}

// 清屏函数，打印26个换行符
void clearScreen() {
	for (int i = 0; i < 26; i++) {
		printf("\n");
	}
}

// 显示文件内容到屏幕
void showFile() {
	clearScreen();
	// 计算当前显示的起始和结束位置
	int start = fpos_first * SCR_WIDTH;
	int end   = start + SCR_SIZE;
	if (end > filesize) {
		end = filesize;
	}

	// 打印显示区域的内容
	for (int i = start; i < end; i++) {
		putchar(buf[i]);
		if ((i - start + 1) % SCR_WIDTH == 0) {
			putchar('\n');
		}
	}

	// 打印光标的位置提示
	printf("\ncursor: line %d, col %d\n", fpos_first + cursor_x + 1, cursor_y + 1);
}

// 移动光标函数
void moveCursor(int direction) {
	switch (direction) {
	case UP:
		if (cursor_x > 0) {
			cursor_x--;
		} else if (fpos_first > 0) { // 向上滚动
			fpos_first--;
		}
		break;
	case DOWN:
		if (cursor_x < SCR_HEIGHT - 1 && (fpos_first + cursor_x + 1) * SCR_WIDTH < filesize) {
			cursor_x++;
		} else if ((fpos_first + SCR_HEIGHT) * SCR_WIDTH < filesize) { // 向下滚动
			fpos_first++;
		}
		break;
	case LEFT:
		if (cursor_y > 0) {
			cursor_y--;
		} else if (cursor_x > 0) { // 向左移动到上一行末尾
			cursor_x--;
			cursor_y = SCR_WIDTH - 1;
		}
		break;
	case RIGHT:
		if (cursor_y < SCR_WIDTH - 1 && (fpos_first + cursor_x) * SCR_WIDTH + cursor_y + 1 < filesize) {
			cursor_y++;
		} else if ((fpos_first + cursor_x + 1) * SCR_WIDTH < filesize) { // 向右移动到下一行开头
			cursor_x++;
			cursor_y = 0;
		}
		break;
	default:
		break;
	}
}

// 插入字符到缓冲区
void insertChar(int ch) {
	if (filesize >= MAXFILESIZEBUF) {
		printf("\nbuffer is full\n");
		return;
	}

	// 计算插入位置
	int pos = (fpos_first + cursor_x) * SCR_WIDTH + cursor_y;
	if (pos > filesize)
		pos = filesize;

	// 将后面的字符向后移动
	memmove(&buf[pos + 1], &buf[pos], filesize - pos);
	buf[pos] = (char)ch;
	filesize++;

	// 更新光标位置
	cursor_y++;
	if (cursor_y >= SCR_WIDTH) {
		cursor_y = 0;
		cursor_x++;
		if (cursor_x >= SCR_HEIGHT) {
			cursor_x = SCR_HEIGHT - 1;
			fpos_first++;
		}
	}
}

// 删除当前光标位置的字符
void deleteChar() {
	if (filesize == 0)
		return;

	// 计算删除位置
	int pos = (fpos_first + cursor_x) * SCR_WIDTH + cursor_y;
	if (pos >= filesize)
		return;

	// 将后面的字符向前移动
	memmove(&buf[pos], &buf[pos + 1], filesize - pos - 1);
	filesize--;

	// 更新光标位置
	if (cursor_y > 0) {
		cursor_y--;
	} else if (cursor_x > 0) {
		cursor_x--;
		cursor_y = SCR_WIDTH - 1;
	}
}

// 编辑函数
void edit() {
	int ch;
	// showFile(); // 初始显示文件内容
	// clearScreen();

	fpos_first = 0;

	while (1) {
		ch = _getch();

		if (IsExt(ch)) {
			// 处理扩展按键
			switch (ch) {
			case ESC:
				putchar('\0');
				return;
			case UP:
			case DOWN:
			case LEFT:
			case RIGHT:
				// moveCursor(ch);
				// showFile();
				// break;
			case ENTER:
				putchar('\n');
				break;
			default:
				break;
			}
			continue;
		} else {
			// 处理普通字符输入
			// if (ch == 127 || ch == 8) { // 处理删除键（Backspace）
			// 	deleteChar();
			// 	showFile();
			// } else if (ch >= 32 && ch <= 126) { // 可打印字符
			// 	insertChar(ch);
			// 	showFile();
			// }
			// 可以添加更多字符处理逻辑，例如回车等

			putchar((char)ch);
		}
	}
	buf[fpos_first] = 0;
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

	ret = read(fd, buf, 8190);

	if (ret == -1) {
		printf("ERROR can not read file: %s\n", filepath);
        return 1;
	} else {
		printf("read %d bytes from %s\n", ret, filepath);
	}

	filesize = strlen(buf);

	printf("%s\nV-V-V-V-V-V\n", buf);

	edit();
	printf("\n---!!final!!---\n");

	lseek(fd, 0, SEEK_SET);
	ret = write(fd, buf, fpos_first);
	if (ret == -1) {
		printf("ERROR can not write file: %s\n", filepath);
        return 1;
	} else {
		printf("write %d bytes from %s\n", ret, filepath);
	}

	close(fd);
	return 0;
}
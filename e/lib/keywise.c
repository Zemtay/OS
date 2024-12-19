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
#include "proto.h"

#include"keywise.h"
#include "logfila.h"
// clang-format on

// keyboard.c中一个变量原始为0
// 每次正常读取键盘会设置为1
// 用户send后,看看这个值,然后将其充值为原始0
int _kbhit() {
	MESSAGE msg;
	msg.type = TTY_KHIT;
	send_recv(BOTH, TASK_TTY, &msg);
	return msg.u.m1.m1i2;
}

// 用户send,阻塞
// keyboard.c设置一个变量
// keyboard.c下次正常运行读取键盘会send回来
int _getch() {
	MESSAGE msg;
	msg.type = TTY_GETCH;
	send_recv(BOTH, TASK_TTY, &msg);
	return msg.u.m1.m1i2;
}

int IsFlag(int flag) {
	MESSAGE msg;
	msg.type      = TTY_FLAGTEST;
	msg.u.m1.m1i2 = flag;
	send_recv(BOTH, TASK_TTY, &msg);
	return msg.u.m1.m1i2;
}

int IsExt(int ch) {
	return (ch & FLAG_EXT);
}

void _scr_clear() {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m1.m1i1 = SCRCTL_CLEAR;
	msg.u.m1.m1i2 = 0;
	send_recv(SEND, TASK_TTY, &msg);
}

void _scr_init() {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m1.m1i1 = SCRCTL_INIT;
	msg.u.m1.m1i2 = 0;
	send_recv(SEND, TASK_TTY, &msg);
	_scr_setbottom(0);
}

void _scr_putch(int ch) {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m1.m1i1 = SCRCTL_PUTCH;
	msg.u.m1.m1i2 = ch;
	LogFuncEntry("keywise", LEVEL_INFO, "putch %d", ch);
	send_recv(SEND, TASK_TTY, &msg);
}
void _scr_setbottom(int type) {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m1.m1i1 = SCRCTL_SETBTM;
	msg.u.m1.m1i2 = type;
	send_recv(SEND, TASK_TTY, &msg);
}
void _scr_putch_bottom(int ch) {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m1.m1i1 = SCRCTL_PUTCH_BTM;
	msg.u.m1.m1i2 = ch;
	send_recv(SEND, TASK_TTY, &msg);
}
void _scr_cursor_move(int asp) {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m1.m1i1 = SCRCTL_CUR_MV;
	msg.u.m1.m1i2 = asp;
	send_recv(SEND, TASK_TTY, &msg);
}

void _scr_cursor_set(int cursor_x, int cursor_y) {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m1.m1i1 = SCRCTL_CUR_SET;
	msg.u.m1.m1i2 = cursor_x;
	msg.u.m1.m1i3 = cursor_y;
	send_recv(SEND, TASK_TTY, &msg);
}

void _scr_putscr(char **buf) {
	MESSAGE msg;
	msg.type      = TTY_SCRCTL;
	msg.u.m3.m3i1 = SCRCTL_SCRSET;
	msg.u.m3.m3p1 = buf;
	send_recv(SEND, TASK_TTY, &msg);
}
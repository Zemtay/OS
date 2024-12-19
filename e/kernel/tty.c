// clang-format off

/*************************************************************************//**
 *****************************************************************************
 * @file   kernel/tty.c
 * @brief  The terminal driver.
 *
 * As a common driver, TTY accepts these MESSAGEs:
 *   - DEV_OPEN
 *   - DEV_READ
 *   - DEV_WRITE
 *
 * Besides, it accepts the other two types of MESSAGE from clock_handler() and
 * a PROC (who is not FS):
 *
 *   - MESSAGE from clock_handler(): HARD_INT
 *      - Every time clock interrupt occurs, the clock handler will check whether
 *        any key has been pressed. If so, it'll invoke inform_int() to wake up
 *        TTY. It is a special message because it is not from a process -- clock
 *        handler is not a process.
 *
 *   - MESSAGE from a PROC: TTY_WRITE
 *      - TTY is a driver. In most cases MESSAGE is passed from a PROC to FS then
 *        to TTY. For some historical reason, PROC is allowed to pass a TTY_WRITE
 *        MESSAGE directly to TTY. Thus a PROC can write to a tty directly.
 *
 * @note   Do not get confused by these function names:
 *           - tty_dev_read() vs tty_do_read()
 *             - tty_dev_read() reads chars from keyboard buffer
 *             - tty_do_read() handles DEV_READ message
 *           - tty_dev_write() vs tty_do_write() vs tty_write()
 *             - tty_dev_write() returns chars to a process waiting for input
 *             - tty_do_write() handles DEV_WRITE message
 *             - tty_write() handles TTY_WRITE message
 *
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

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

#include "logfila.h"


#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)


PRIVATE void	init_tty	(TTY* tty);
PRIVATE void	tty_dev_read	(TTY* tty);
PRIVATE void	tty_dev_write	(TTY* tty);
PRIVATE void	tty_do_read	(TTY* tty, MESSAGE* msg);
PRIVATE void	tty_do_write	(TTY* tty, MESSAGE* msg);
PRIVATE void	put_key		(TTY* tty, u32 key);

PRIVATE void do_khit(MESSAGE *msg);
PRIVATE void do_getch(MESSAGE *msg);
PRIVATE void do_testflag(MESSAGE *msg);

PRIVATE void do_itsmyscreen(MESSAGE *msg);

static int IsHit = 0; // 有新的键按下
static int WaitToRead = -1; // 是否有别的进程在等着读键盘,如果无-1,如果有=进程号

/*****************************************************************************
 *                                task_tty
 *****************************************************************************/
/**
 * <Ring 1> Main loop of task TTY.
 *****************************************************************************/
PUBLIC void task_tty()
{
	TTY *	tty;
	MESSAGE msg;

	init_keyboard();

	for (tty = TTY_FIRST; tty < TTY_END; tty++)
		init_tty(tty);

	select_console(0);

	while (1) {
		for (tty = TTY_FIRST; tty < TTY_END; tty++) {
			do {
				tty_dev_read(tty);
				tty_dev_write(tty);
			} while (tty->ibuf_cnt);
		}

		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;
		assert(src != TASK_TTY);

		TTY* ptty = &tty_table[msg.DEVICE];

		switch (msg.type) {
		case TTY_KHIT:
			do_khit(&msg);
			break;
		case TTY_GETCH:
			do_getch(&msg);
			break;
		case TTY_FLAGTEST:
			do_testflag(&msg);
			break;
		case TTY_SCRCTL:
			do_itsmyscreen(&msg);
			break;
		case DEV_OPEN:
			reset_msg(&msg);
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
			break;
		case DEV_READ:
			tty_do_read(ptty, &msg);
			break;
		case DEV_WRITE:
			tty_do_write(ptty, &msg);
			break;
		case HARD_INT:
			/**
			 * waked up by clock_handler -- a key was just pressed
			 * @see clock_handler() inform_int()
			 */
			key_pressed = 0;
			continue;
		default:
			dump_msg("TTY::unknown msg", &msg);
			break;
		}
	}
}

// clang-format on

PRIVATE void do_khit(MESSAGE *msg) {
	int source = msg->source;
	// k hit
	msg->u.m1.m1i2 = IsHit;
	IsHit          = 0;
	send_recv(SEND, source, msg);
}

PRIVATE void do_getch(MESSAGE *msg) {
	int source = msg->source;
	// get ch
	WaitToRead = source;
}

PRIVATE void do_testflag(MESSAGE *msg) {
	int source = msg->source;
	// is pressed
	msg->u.m1.m1i2 = IsFlag(msg->u.m1.m1i2);
	send_recv(SEND, source, msg);
}

PRIVATE void do_itsmyscreen(MESSAGE *msg) {
	TTY *tty   = &tty_table[1];
	int method = msg->u.m1.m1i1;
	int arg    = msg->u.m1.m1i2;
	int arg2   = msg->u.m1.m1i3;
	LogFuncEntry("TTY-sc", LEVEL_INFO, "method=%d, arg=%d", method, arg);
	CONSOLE *con = tty->console;
	assert(con != NULL);
	u8 *pch       = (u8 *)(V_MEM_BASE + con->cursor * 2);
	u8 *last_line = (u8 *)(V_MEM_BASE + (con->orig + (SCR_SIZE - SCR_WIDTH)) * 2);

	switch (method) {
	case SCRCTL_SCRSET: {
		char **srcbuf = (char **)msg->u.m3.m3p1;
		char buf[25][80];
		phys_copy((void *)va2la(TASK_TTY, buf), (void *)va2la(msg->source, srcbuf), 80 * 25 + 1);
		LogFuncEntry("tty-scr", LEVEL_INFO, "%s", buf[0]);
		for (int tcy = 0; tcy < SCR_HEIGHT - 1; tcy++) {
			for (int tcx = 0; tcx < SCR_WIDTH; tcx++) {
				pch    = (u8 *)(V_MEM_BASE + (con->orig + tcy * SCR_WIDTH + tcx) * 2);
				*pch++ = (buf[tcy][tcx] ? buf[tcy][tcx] : ' ');
				*pch   = DEFAULT_CHAR_COLOR;
			}
		}
	} break;
	case SCRCTL_PUTCH:
		// 光标处放置字符
		*pch++ = arg;
		*pch++ = DEFAULT_CHAR_COLOR;
		break;
	case SCRCTL_SETBTM: {
		for (int i = 0; i < SCR_WIDTH; i++) {
			*(last_line + i * 2) = '\0';
		}
		if (arg == 0) {
		} else if (arg == 1) {
			char s[] = "--INSERT--";
			for (int i = 0; s[i] != '\0'; i++) {
				*(last_line + i * 2) = s[i];
			}
		} else if (arg == 2) {
			char s[] = ":";
			for (int i = 0; s[i] != '\0'; i++) {
				*(last_line + i * 2) = s[i];
			}
		} else if (arg == 3) {
			char s[] = "--NORMAL--";
			for (int i = 0; s[i] != '\0'; i++) {
				*(last_line + i * 2) = s[i];
			}
		}
	} break;
	case SCRCTL_PUTCH_BTM: {
		// 找到last_line的第一个'\0'
		int i;
		for (i = 0; i < SCR_WIDTH; i++) {
			if (*(last_line + i * 2) == '\0')
				break;
		}
		*(last_line + i * 2)       = arg;
		*(last_line + (i + 1) * 2) = '\0';
	} break;
	case SCRCTL_CUR_MV: {
		int cursor_x = (con->cursor - con->orig) % SCR_WIDTH; // 当前光标的x坐标
		int cursor_y = (con->cursor - con->orig) / SCR_WIDTH; // 当前光标的y坐标
		switch (arg) {
		case 4:
			if (cursor_x > 0)
				cursor_x--; // 如果不在最左边界，则向左移动
			break;
		case 8:
			if (cursor_y > 0)
				cursor_y--; // 如果不在最上边界，则向上移动
			break;
		case 6:
			if (cursor_x < SCR_WIDTH - 1)
				cursor_x++; // 如果不在最右边界，则向右移动
			break;
		case 2:
		default:
			if (cursor_y < SCR_HEIGHT - 1)
				cursor_y++; // 如果不在最下边界，则向下移动
			break;
		}
		// 更新光标的位置
		con->cursor = con->orig + cursor_y * SCR_WIDTH + cursor_x;

		// 刷新显示
		flush_cursor(con);
	} break;
	case SCRCTL_CUR_SET: {
		con->cursor = con->orig + arg2 * SCR_WIDTH + arg;
		// 刷新显示
		flush_cursor(con);
	} break;
	case SCRCTL_CLEAR:
		con->cursor = con->orig;
		for (int i = 0; i < SCR_SIZE; i++) {
			out_char(tty->console, ' ');
		}
		con->cursor = con->orig;
	case SCRCTL_INIT:
	default:
		init_tty(tty);
		break;
	}
}

// clang-format off

/*****************************************************************************
 *                                init_tty
 *****************************************************************************/
/**
 * Things to be initialized before a tty can be used:
 *   -# the input buffer
 *   -# the corresponding console
 * 
 * @param tty  TTY stands for teletype, a cool ancient magic thing.
 *****************************************************************************/
PRIVATE void init_tty(TTY* tty)
{
	tty->ibuf_cnt = 0;
	tty->ibuf_head = tty->ibuf_tail = tty->ibuf;

	tty->tty_caller = NO_TASK;
	tty->tty_procnr = NO_TASK;
	tty->tty_req_buf = 0;
	tty->tty_left_cnt = 0;
	tty->tty_trans_cnt = 0;

	init_screen(tty);
}


/*****************************************************************************
 *                                in_process
 *****************************************************************************/
/**
 * keyboard_read() will invoke this routine after having recognized a key press.
 * 
 * @param tty  The key press is for whom.
 * @param key  The integer key with metadata.
 *****************************************************************************/
PUBLIC void in_process(TTY* tty, u32 key)
{
	// 放进缓冲区
	IsHit = 1;
	if (WaitToRead != -1) {
		// TODO:send
		MESSAGE key_msg;
		key_msg.u.m1.m1i2 = key;
		send_recv(SEND, WaitToRead, &key_msg);
		WaitToRead = -1;
	}
	if (!(key & FLAG_EXT)) {
		put_key(tty, key);
	}
	else {
		int raw_code = key & MASK_RAW;
		switch(raw_code) {
		case ENTER:
			put_key(tty, '\n');
			break;
		case BACKSPACE:
			put_key(tty, '\b');
			break;
		case UP:
			if ((key & FLAG_SHIFT_L) ||
			    (key & FLAG_SHIFT_R)) {	/* Shift + Up */
				scroll_screen(tty->console, SCR_DN);
			}
			break;
		case DOWN:
			// printl("press down");
			if ((key & FLAG_SHIFT_L) ||
			    (key & FLAG_SHIFT_R)) {	/* Shift + Down */
				scroll_screen(tty->console, SCR_UP);
			}
			break;

		/** 切换tty
		 * Since alt + FN will invoke ubuntu's trigger, our fork os will change to another keybinds
		 */
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:

			// alt 不行,和ubuntu冲突
			// if ((key & FLAG_ALT_L) ||
			//     (key & FLAG_ALT_R)) {	/* Alt + PAD1~12 */
				// printl("switch");
				select_console(raw_code - F1);
			// }
			break;
		default:
		// 实际上是能接受到的
			break;
		}
	}
}


/*****************************************************************************
 *                                put_key
 *****************************************************************************/
/**
 * Put a key into the in-buffer of TTY.
 *
 * @callergraph
 * 
 * @param tty  To which TTY the key is put.
 * @param key  The key. It's an integer whose higher 24 bits are metadata.
 *****************************************************************************/
PRIVATE void put_key(TTY* tty, u32 key)
{
	if (tty->ibuf_cnt < TTY_IN_BYTES) {
		*(tty->ibuf_head) = key;
		tty->ibuf_head++;
		if (tty->ibuf_head == tty->ibuf + TTY_IN_BYTES)
			tty->ibuf_head = tty->ibuf;
		tty->ibuf_cnt++;
	}
}


/*****************************************************************************
 *                                tty_dev_read
 *****************************************************************************/
/**
 * Get chars from the keyboard buffer if the TTY::console is the `current'
 * console.
 *
 * @see keyboard_read()
 * 
 * @param tty  Ptr to TTY.
 *****************************************************************************/
PRIVATE void tty_dev_read(TTY* tty)
{
	if (is_current_console(tty->console))
		keyboard_read(tty);
}


/*****************************************************************************
 *                                tty_dev_write
 *****************************************************************************/
/**
 * Echo the char just pressed and transfer it to the waiting process.
 * 
 * @param tty   Ptr to a TTY struct.
 *****************************************************************************/
PRIVATE void tty_dev_write(TTY* tty)
{
	while (tty->ibuf_cnt) {
		char ch = *(tty->ibuf_tail);
		tty->ibuf_tail++;
		if (tty->ibuf_tail == tty->ibuf + TTY_IN_BYTES)
			tty->ibuf_tail = tty->ibuf;
		tty->ibuf_cnt--;

		if (tty->tty_left_cnt) {
			if (ch >= ' ' && ch <= '~') { /* printable */
				out_char(tty->console, ch);

				assert(tty->tty_req_buf);
				void * p = tty->tty_req_buf +
					   tty->tty_trans_cnt;
				phys_copy(p, (void *)va2la(TASK_TTY, &ch), 1);

				tty->tty_trans_cnt++;
				tty->tty_left_cnt--;
			}
			else if (ch == '\b' && tty->tty_trans_cnt) {
				out_char(tty->console, ch);
				tty->tty_trans_cnt--;
				tty->tty_left_cnt++;
			}

			if (ch == '\n' || tty->tty_left_cnt == 0) {
				out_char(tty->console, '\n');

				assert(tty->tty_procnr != NO_TASK);
				MESSAGE msg;
				msg.type = RESUME_PROC;
				msg.PROC_NR = tty->tty_procnr;
				msg.CNT = tty->tty_trans_cnt;
				send_recv(SEND, tty->tty_caller, &msg);
				tty->tty_left_cnt = 0;
			}
		}
	}
}


/*****************************************************************************
 *                                tty_do_read
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_READ message.
 *
 * @note The routine will return immediately after setting some members of
 * TTY struct, telling FS to suspend the proc who wants to read. The real
 * transfer (tty buffer -> proc buffer) is not done here.
 * 
 * @param tty  From which TTY the caller proc wants to read.
 * @param msg  The MESSAGE just received.
 *****************************************************************************/
PRIVATE void tty_do_read(TTY* tty, MESSAGE* msg)
{
	/* tell the tty: */
	tty->tty_caller   = msg->source;  /* who called, usually FS */
	tty->tty_procnr   = msg->PROC_NR; /* who wants the chars */
	tty->tty_req_buf  = va2la(tty->tty_procnr,
				  msg->BUF);/* where the chars should be put */
	tty->tty_left_cnt = msg->CNT; /* how many chars are requested */
	tty->tty_trans_cnt= 0; /* how many chars have been transferred */

	msg->type = SUSPEND_PROC;
	send_recv(SEND, tty->tty_caller, msg);
}


/*****************************************************************************
 *                                tty_do_write
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_WRITE message.
 * 
 * @param tty  To which TTY the calller proc is bound.
 * @param msg  The MESSAGE.
 *****************************************************************************/
PRIVATE void tty_do_write(TTY* tty, MESSAGE* msg)
{
	char buf[TTY_OUT_BUF_LEN];
	char * p = (char*)va2la(msg->PROC_NR, msg->BUF);
	int i = msg->CNT;
	int j;

	while (i) {
		int bytes = min(TTY_OUT_BUF_LEN, i);
		phys_copy(va2la(TASK_TTY, buf), (void*)p, bytes);
		for (j = 0; j < bytes; j++)
			out_char(tty->console, buf[j]);
		i -= bytes;
		p += bytes;
	}

	msg->type = SYSCALL_RET;
	send_recv(SEND, msg->source, msg);
}


/*****************************************************************************
 *                                sys_printx
 *****************************************************************************/
/**
 * System calls accept four parameters. `printx' needs only two, so it wastes
 * the other two.
 *
 * @note `printx' accepts only one parameter -- `char* s', the other one --
 * `struct proc * proc' -- is pushed by kernel.asm::sys_call so that the
 * kernel can easily know who invoked the system call.
 *
 * @note s[0] (the first char of param s) is a magic char. if it equals
 * MAG_CH_PANIC, then this syscall was invoked by `panic()', which means
 * something goes really wrong and the system is to be halted; if it equals
 * MAG_CH_ASSERT, then this syscall was invoked by `assert()', which means
 * an assertion failure has occured. @see kernel/main lib/misc.c.
 * 
 * @param _unused1  Ignored.
 * @param _unused2  Ignored.
 * @param s         The string to be printed.
 * @param p_proc    Caller proc.
 * 
 * @return  Zero if success.
 *****************************************************************************/
PUBLIC int sys_printx(int _unused1, int _unused2, char* s, struct proc* p_proc)
{
	const char * p;
	char ch;

	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;

	/**
	 * @note Code in both Ring 0 and Ring 1~3 may invoke printx().
	 * If this happens in Ring 0, no linear-physical address mapping
	 * is needed.
	 *
	 * @attention The value of `k_reenter' is tricky here. When
	 *   -# printx() is called in Ring 0
	 *      - k_reenter > 0. When code in Ring 0 calls printx(),
	 *        an `interrupt re-enter' will occur (printx() generates
	 *        a software interrupt). Thus `k_reenter' will be increased
	 *        by `kernel.asm::save' and be greater than 0.
	 *   -# printx() is called in Ring 1~3
	 *      - k_reenter == 0.
	 */
	if (k_reenter == 0)  /* printx() called in Ring<1~3> */
		p = va2la(proc2pid(p_proc), s);
	else if (k_reenter > 0) /* printx() called in Ring<0> */
		p = s;
	else	/* this should NOT happen */
		p = reenter_err;

	/**
	 * @note if assertion fails in any TASK, the system will be halted;
	 * if it fails in a USER PROC, it'll return like any normal syscall
	 * does.
	 */
	if ((*p == MAG_CH_PANIC) ||
	    (*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS])) {
		disable_int();
		char * v = (char*)V_MEM_BASE;
		const char * q = p + 1; /* +1: skip the magic char */

		while (v < (char*)(V_MEM_BASE + V_MEM_SIZE)) {
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (!*q) {
				while (((int)v - V_MEM_BASE) % (SCR_WIDTH * 16)) {
					/* *v++ = ' '; */
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}

		__asm__ __volatile__("hlt");
	}

	while ((ch = *p++) != 0) {
		if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue; /* skip the magic char */

		/* TTY * ptty; */
		/* for (ptty = TTY_FIRST; ptty < TTY_END; ptty++) */
		/* 	out_char(ptty->console, ch); /\* output chars to all TTYs *\/ */
		out_char(TTY_FIRST->console, ch);
	}

	//__asm__ __volatile__("nop;jmp 1f;ud2;1: nop");
	//__asm__ __volatile__("nop;cli;1: jmp 1b;ud2;nop");

	return 0;
}

/*****************************************************************************
 *                                dump_tty_buf
 *****************************************************************************/
/**
 * For debug only.
 * 
 *****************************************************************************/
PUBLIC void dump_tty_buf()
{
	TTY * tty = &tty_table[1];

	static char sep[] = "--------------------------------\n";

	printl(sep);

	printl("head: %d\n", tty->ibuf_head - tty->ibuf);
	printl("tail: %d\n", tty->ibuf_tail - tty->ibuf);
	printl("cnt: %d\n", tty->ibuf_cnt);

	int pid = tty->tty_caller;
	printl("caller: %s (%d)\n", proc_table[pid].name, pid);
	pid = tty->tty_procnr;
	printl("caller: %s (%d)\n", proc_table[pid].name, pid);

	printl("req_buf: %d\n", (int)tty->tty_req_buf);
	printl("left_cnt: %d\n", tty->tty_left_cnt);
	printl("trans_cnt: %d\n", tty->tty_trans_cnt);

	printl("--------------------------------\n");

	strcpy(sep, "\n");
}


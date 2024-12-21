// clang-format off

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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
#include "unistd.h"

// clang-format on

/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start.
 *
 *****************************************************************************/
PUBLIC int kernel_main() {
	disp_str(
		"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
	u8 rpl;
	u8 priv; /* privilege */

	struct task *t;
	struct proc *p = proc_table;

	char *stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++, p++, t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

		if (i < NR_TASKS) { /* TASK */
			t      = task_table + i;
			priv   = PRIVILEGE_TASK;
			rpl    = RPL_TASK;
			eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio   = 15;
		} else { /* USER PROC */
			t      = user_proc_table + (i - NR_TASKS);
			priv   = PRIVILEGE_USER;
			rpl    = RPL_USER;
			eflags = 0x202; /* IF=1, bit 2 is always 1 */
			prio   = 5;
		}

		strcpy(p->name, t->name); /* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		} else { /* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C], 0, /* bytes before the entry point
												 * are useless (wasted) for the
												 * INIT process, doesn't matter
												 */
				(k_base + k_limit) >> LIMIT_4K_SHIFT, DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW], 0, /* bytes before the entry point
												  * are useless (wasted) for the
												  * INIT process, doesn't matter
												  */
				(k_base + k_limit) >> LIMIT_4K_SHIFT, DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 | SA_TIL | rpl;
		p->regs.ds = p->regs.es = p->regs.fs = p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs                                        = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip                                       = (u32)t->initial_eip;
		p->regs.esp                                       = (u32)stk;
		p->regs.eflags                                    = eflags;

		p->ticks = p->priority = prio;

		p->p_flags      = 0;
		p->p_msg        = 0;
		p->p_recvfrom   = NO_TASK;
		p->p_sendto     = NO_TASK;
		p->has_int_msg  = 0;
		p->q_sending    = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks     = 0;

	p_proc_ready = proc_table;

	init_clock();
	init_keyboard();

	restart();

	while (1) {
	}
}

/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks() {
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header { /* byte offset */
	char name[100];       /*   0 */
	char mode[8];         /* 100 */
	char uid[8];          /* 108 */
	char gid[8];          /* 116 */
	char size[12];        /* 124 */
	char mtime[12];       /* 136 */
	char chksum[8];       /* 148 */
	char typeflag;        /* 156 */
	char linkname[100];   /* 157 */
	char magic[6];        /* 257 */
	char version[2];      /* 263 */
	char uname[32];       /* 265 */
	char gname[32];       /* 297 */
	char devmajor[8];     /* 329 */
	char devminor[8];     /* 337 */
	char prefix[155];     /* 345 */
						  /* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 *
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char *filename) {
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	// printf("fd:%d", fd);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);

	while (1) {
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0)
			break;

		struct posix_tar_header *phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char *p   = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		printf("size%d", bytes_left);

		/**
		 * 原来,这里的phdr->name是文件的名字,而不是标准的地址,即为'kernel.bin'而非'/kernel.bin'
		 * 需要改成 "lib/get_pwd" + "filename"
		 */
		char filepath[256] = "";
		get_full_path(phdr->name, filepath);
		// printf("currently file path: %s\n", filepath);
		int fdout = open(filepath, O_CREAT | O_RDWR);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]");
			return;
		}
		printf("    %s (%d bytes)\n", phdr->name, f_len);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf, ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	close(fd);

	printf(" done]\n");
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 *
 * 实现对多级路径的支持
 * 包括当前的pwd显示
 *
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shell_permission(){
	MESSAGE m_msg;
	m_msg.type	= PERMIT_P;
	send_recv(BOTH, TASK_M, &m_msg);
	assert(m_msg.RETVAL == 0);
}

void set_permission(){
	MESSAGE m_msg;
	m_msg.type	= PERMIT_F;
	send_recv(BOTH, TASK_M, &m_msg);
	assert(m_msg.RETVAL == 0);
};

PUBLIC void add_permission(int pid, char *filepath)
{
	MESSAGE msg;
	msg.type	    = ADD;
	msg.PATHNAME	= filepath;
	msg.PROC_NR 	= pid;
	msg.NAME_LEN	= strlen(filepath);

	// DEBUG_PRINT("execv", "current pid:%d", proc2pid(pcaller));
	send_recv(BOTH, TASK_M, &msg);
}

PUBLIC void clear_permission(int pid, char *filepath)
{
	MESSAGE msg;
	msg.type	    = CLEAR;
	msg.PATHNAME	= filepath;
	msg.PROC_NR 	= pid;
	msg.NAME_LEN	= strlen(filepath);

	// DEBUG_PRINT("execv", "current pid:%d", proc2pid(pcaller));
	send_recv(BOTH, TASK_M, &msg);
}

void shabby_shell(const char *tty_name) {
	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char curpath[BYTES_SHELL_WORKING_DIRECTORY];

	while (1) {
		getcwd(curpath,BYTES_SHELL_WORKING_DIRECTORY);
		write(1, curpath, strlen(curpath));
		write(1, "$ ", 2);
		int r    = read(0, rdbuf, 70);
		rdbuf[r] = 0;

		int argc = 0;
		char *argv[PROC_ORIGIN_STACK];
		char *p = rdbuf;
		char *s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s    = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word         = 0;
				argv[argc++] = s;
				*p           = 0;
			}
			p++;
		} while (ch);
		
		
		
		// 检查是否包含 `&`，并设置后台标志
		int is_background = 0;
		if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
			is_background = 1;
			argv[argc - 1] = NULL; // 删除 `&`，使参数正确
			argc--;
		}

		argv[argc] = 0; // 确保参数列表以 NULL 结束
		// DEBUG_PRINT("shell", "argc %d", argc);

		if (argc == 0)
			continue;

		char full_path[256] = "";
		get_full_path(argv[0], full_path);
		// printf("parsing %s, %s, %s", argv[0], argv[1], argv[2]);
		if(!strcmp(argv[0], "pro\0")){
			int pid = (argv[1][0]-'0')*10+(argv[1][1]-'0');  //注意要从字符串转化
			add_permission(pid, argv[2]);
		}
		else if(!strcmp(argv[0], "low\0")){
			int pid = (argv[1][0]-'0')*10+(argv[1][1]-'0');
			clear_permission(pid, argv[2]);
		}
		else{
			printf("[shell] want to exec %s \n", full_path);
			int fd = open(full_path, O_RDWR);
			if (fd == -1) {
				if (rdbuf[0]) {
					write(1, "{", 1);
					write(1, rdbuf, r);
					write(1, "}\n", 2);
				}
			} else {
				int ok = close(fd);
				int pid = fork();  // 创建子进程
				if (pid != 0) { /* parent */
					if (!is_background) {
						int s;
						wait(&s); // 前台进程等待子进程完成
					} else {
						printf("[shell] Background process started. PID: %d\n", pid);
					}
				} else { /* child */
					// set_permission();
					if(!strcmp(full_path, "rm")){
						MESSAGE m_msg;
						m_msg.type	= CLEAR;
						char pathname[2] = "/\0";
						m_msg.PATHNAME	= pathname;
						m_msg.NAME_LEN	= strlen(pathname);
						send_recv(BOTH, TASK_M, &m_msg);
					}
					argv[0] = full_path;
					execv(full_path, argv); // 执行命令
					// ret_permission(); 在execv中
				}
			}
		}
	}

	close(1);
	close(0);
}

void permission_ls(char *full_path){
	// ls需要读根目录
if(strcmp(full_path, "/ls\0")){
					MESSAGE m_msg;
					m_msg.type	= ADD;
					char pathname[2] = "/\0";
					m_msg.PATHNAME	= pathname;
					m_msg.NAME_LEN	= strlen(pathname);

					send_recv(BOTH, TASK_M, &m_msg);
					printl("the permission for ls add is: %d\n", m_msg.RETVAL);
					// assert(m_msg.RETVAL == 0);
				}
if(strcmp(full_path, "/ls\0")){
					MESSAGE m_msg;
					m_msg.type	= CLEAR;
					char pathname[2] = "/\0";
					m_msg.PATHNAME	= pathname;
					m_msg.NAME_LEN	= strlen(pathname);

					send_recv(BOTH, TASK_M, &m_msg);
					printl("the permission for ls clear is: %d\n", m_msg.RETVAL);
					// assert(m_msg.RETVAL == 0);
				}
}
/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 *
 *****************************************************************************/
void test_cipher()
{
	// 文件加密存储
	printl("===Create and Encrypt a file===\n");
	int fd;
	int n;
	const char filename[] = "password";
	const char bufw[] = "abcde\0";
	const int rd_bytes = 4;
	char bufr[rd_bytes];

	assert(rd_bytes <= strlen(bufw));

	/* create */
	// printl("File not created. fd: %d\n", fd);
	fd = open(filename, O_CREAT | O_RDWR);
	// while(!fd){
	// 	;
	// }
	assert(fd != -1);
	// printl("===TestA: File created. fd: %d===\n", fd);
	n = write(fd, bufw, strlen(bufw));
	assert(n == strlen(bufw));
	close(fd);
	printl("===TestA: file \"%s\", write_enc: %s, len: %d===\n", filename, bufw, n);
	// add_permission("07\0", "password\0");
	/* open */
	fd = open(filename, O_RDWR);
	// printl("open: %d\n", fd);
	assert(fd != -1);
	n = read(fd, bufr, rd_bytes);
	assert(n == rd_bytes);
	// printl("rd_bytes: %d %d\n", n, rd_bytes);
	bufr[n] = 0;
	// printl("===TestA: file %s, read: %s, %d, %d, %d===\n", filename, bufr, n, tmp, fd);
	printl("===TestA: file \"%s\", read: %s, len: %d===\n", filename, bufr, n);
	close(fd);  // 注意，这里如果没有关文件描述符，会从下一个字符开始读取；与buf无关


	// fd = open(filename, O_RDWR);
	// n = read_dec(fd, bufr, rd_bytes);
	// assert(n == rd_bytes);
	// bufr[n] = 0;
	// printl("===TestA: file \"%s\", read_dec: %s, len: %d===\n", filename, bufr, n);
	// close(fd);

}

void set_cipherkey(char *rdbuf, int r){
	MESSAGE m_msg;
	// int pid = getpid();
	// phys_copy((void*)va2la(src, buf + bytes_rw),
	// 					(void*)va2la(pid, rdbuf),
	// 					bytes);
	m_msg.type	= SETKEY;
	m_msg.BUF = (void*)rdbuf;
	m_msg.CNT	= strlen(rdbuf);

	send_recv(BOTH, TASK_M, &m_msg);
	// printl("\n**permission %d, m_msg.src: %d, pathname: %s\n", m_msg.RETVAL, m_msg.source, pathname);
	if(m_msg.RETVAL == 0){
		printl("***Successfully set the primkey\n");
		return 0;
	}
}

void Init() {
	int fd_stdin = open("/dev_tty0", O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");
	Init_Shell("/");

	/* extract `cmd.tar' */
	untar("/cmd.tar");

	// // test_cipher();
	// printf("===Create and Encrypt a file===\n");
	// int fd;
	// int n;
	// const char filename[] = "/password";
	// const char bufw[] = "abcde\0";
	// const int rd_bytes = 4;
	// char bufr[rd_bytes];
	// assert(rd_bytes <= strlen(bufw));
	// fd = open(filename, O_CREAT | O_RDWR);
	// assert(fd != -1);
	// n = write(fd, bufw, strlen(bufw));
	// assert(n == strlen(bufw));
	// close(fd);
	// printl("===TestA: file \"%s\", write_enc: %s, len: %d===\n", filename, bufw, n);
	// /* open */
	// fd = open(filename, O_RDWR);
	// // printl("open: %d\n", fd);
	// assert(fd != -1);
	// n = read(fd, bufr, rd_bytes);
	// assert(n == rd_bytes);
	// // printl("rd_bytes: %d %d\n", n, rd_bytes);
	// bufr[n] = 0;
	// // printl("===TestA: file %s, read: %s, %d, %d, %d===\n", filename, bufr, n, tmp, fd);
	// printl("===TestA: file \"%s\", read: %s, len: %d===\n", filename, bufr, n);
	// close(fd);  // 注意，这里如果没有关文件描述符，会从下一个字符开始读取；与buf无关

	char rdbuf[256];
	char curpath[BYTES_SHELL_WORKING_DIRECTORY];
	printf("Please set your key for ciphering, within 256 bytes: \n");
	int r    = read(0, rdbuf, 70);
	rdbuf[r] = 0;
	set_cipherkey(rdbuf, r);

	char *tty_list[] = {"/dev_tty1", "/dev_tty2"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
			printf("[parent is running, child pid:%d]\n", pid);
		} else { /* child process */
			printf("[child is running, pid:%d]\n", getpid());
			close(fd_stdin);
			close(fd_stdout);

			shell_permission();
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}

/*======================================================================*
							   TestA
 *======================================================================*/
void TestA() {
	for (;;)
		;
}

/*======================================================================*
							   TestB
 *======================================================================*/
void TestB() {
	for (;;)
		;
}

/*======================================================================*
							   TestB
 *======================================================================*/
void TestC() {
	for (;;)
		;
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...) {
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char *)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

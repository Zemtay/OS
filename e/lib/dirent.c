
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


#include "mdirent.h"

// clang-format on
int opendir(char *path, DIR *d) {
	int fd = open(path, O_RDWR);
	if (fd == -1) {
		return -1;
	}

	d->fd = fd;
	d->i  = 0;
	d->j  = 0;
	d->m  = 0;
	return 0;
}

void readdir(DIR *dir, char *ret) {
	if (dir == 0) {
		return;
	}
	char out[MAX_PATH];

	MESSAGE msg;
	msg.type      = LIST;
	msg.u.m3.m3i1 = dir->fd;
	msg.u.m3.m3i2 = dir->i;
	msg.u.m3.m3i3 = dir->j;
	msg.u.m3.m3i4 = dir->dev;
	msg.PATHNAME  = (void *)out;
	msg.u.m3.m3l1 = dir->m;

	send_recv(BOTH, TASK_FS, &msg);

	dir->i = msg.u.m3.m3i2;
	dir->j = msg.u.m3.m3i3;
	dir->m = msg.u.m3.m3l1;

	assert(msg.type == SYSCALL_RET);

	_strcpy(ret, msg.PATHNAME);
}

int closedir(DIR *d) {
	return close(d->fd);
}
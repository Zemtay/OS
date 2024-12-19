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
#include "logfila.h"

#include "logfilament.h"
// clang-format on

void PrintLogTail_User(int tail) {
	MESSAGE msg;
	msg.u.m3.m3i1 = tail;
	msg.type      = 2;

	send_recv(SEND, TASK_LOGS, &msg);
}

void SwitchLogLevel_User(int level) {
	if (level < LEVEL_TRACE || level > LEVEL_PANIC)
		return;
 
 	MESSAGE msg;
	msg.u.m3.m3i1 = level;
	msg.type      = 3;

	send_recv(SEND, TASK_LOGS, &msg);
}
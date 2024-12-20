/*************************************************************************//**
 *****************************************************************************
 * @file   open.c
 * @brief  open()
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
#include "proto.h"
#include "logfila.h"
/*****************************************************************************
 *                                open
 *****************************************************************************/
/**
 * open/create a file.
 * 
 * @param pathname  The full path of the file to be opened/created.
 * @param flags     O_CREAT, O_RDWR, etc.
 * 
 * @return File descriptor if successful, otherwise -1.
 *****************************************************************************/
PUBLIC int open(const char *pathname, int flags)
{
	MESSAGE m_msg;

	m_msg.type	= CHECK;
	m_msg.PATHNAME	= (void*)pathname;
	m_msg.FLAGS	= flags;
	m_msg.NAME_LEN	= strlen(pathname);

	send_recv(BOTH, TASK_M, &m_msg);
	// printl("\n**permission %d, m_msg.src: %d, pathname: %s\n", m_msg.RETVAL, m_msg.source, pathname);
	if(m_msg.RETVAL == 0){
		printl("***open permission denied!\n");
		// printl("the permission is: %d\n", m_msg.RETVAL);
		assert(m_msg.RETVAL == 0);
		return 0;
	}
	// printl("the permission is: %d\n", m_msg.RETVAL);

	MESSAGE msg;

	msg.type	= OPEN;

	msg.PATHNAME	= (void*)pathname;
	msg.FLAGS	= flags;
	msg.NAME_LEN	= strlen(pathname);

	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	if((flags&O_CREAT) == 1){  //O_CREAT
		MESSAGE m_msg;
		m_msg.type	= ADD;
		m_msg.PATHNAME	= (void*)pathname;
		m_msg.FLAGS	= flags;
		m_msg.NAME_LEN	= strlen(pathname);

		send_recv(BOTH, TASK_M, &m_msg);
		// printl("the permission is: %d\n", m_msg.RETVAL);
		assert(m_msg.RETVAL == 0);
	}
	return msg.FD;  // 这句话别忘了
}


/*****************************************************************************
 *                                mkdir
 *****************************************************************************/
/**
 * make a directory.
 * 
 * @param pathname  The full path of the directory to be created.
 * @param flags     O_CREAT, O_RDWR, etc.
 * 
 * @return File descriptor if successful, otherwise -1.
 *****************************************************************************/
PUBLIC int mkdir(const char *pathname, int flags){
	MESSAGE msg;

	msg.type	= MKDIR;

	msg.PATHNAME	= (void*)pathname;
	msg.FLAGS	= flags;
	msg.NAME_LEN	= strlen(pathname);

	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.FD;
}

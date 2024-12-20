/*************************************************************************//**
 *****************************************************************************
 * @file   unlink.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   Tue Jun  3 16:12:05 2008
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

/*****************************************************************************
 *                                unlink
 *****************************************************************************/
/**
 * Delete a file.
 * 
 * @param pathname  The full path of the file to delete.
 * 
 * @return Zero if successful, otherwise -1.
 *****************************************************************************/
PUBLIC int unlink(const char * pathname)
{
	MESSAGE m_msg;

	m_msg.type	= CHECK;
	m_msg.PATHNAME	= (void*)pathname;
	m_msg.NAME_LEN	= strlen(pathname);

	send_recv(BOTH, TASK_M, &m_msg);
	printl("**unlink permission %d, m_msg.src: %d, pathname: %s\n", m_msg.RETVAL, m_msg.source, pathname);
	if(m_msg.RETVAL == 0){
		printl("***unlink permission denied!\n");
		// printl("the permission is: %d\n", m_msg.RETVAL);
		assert(m_msg.RETVAL == 0);
		return 0;
	}

	MESSAGE msg;
	msg.type   = UNLINK;

	msg.PATHNAME	= (void*)pathname;
	msg.NAME_LEN	= strlen(pathname);

	send_recv(BOTH, TASK_FS, &msg);

	return msg.RETVAL;
}

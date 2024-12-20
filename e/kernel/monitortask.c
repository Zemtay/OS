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

PRIVATE void init_permission();
PRIVATE int check_permission();
PRIVATE int add_permission();
PRIVATE int clear_permission();

PUBLIC void task_m1(){
    while(1){

    }
}
PUBLIC void task_m() {
    // 注意不能用printf;且要写成无限循环
    printl("{MONITOR} Task MONITOR begins.\n");

    init_permission();
    // MESSAGE msg;
    // send_recv(RECEIVE, ANY, &msg);
	while(1){
        send_recv(RECEIVE, ANY, &m_msg);
        int type = m_msg.type;
        int src  = m_msg.source;

        switch(type){
        case CHECK:
            m_msg.RETVAL = check_permission();
            // printl("the permission: %d\n", m_msg.RETVAL);
            break;
        case ADD:
            m_msg.RETVAL = add_permission();
            break;
        case CLEAR:
            m_msg.RETVAL = clear_permission();
            break;
        default:
            dump_msg("M::unknown message:", &m_msg);
            assert(0);
            break;
        }

        /* reply */
		send_recv(SEND, src, &m_msg); // 不是抄过来的fs_msg
    }
}

void init_permission(){
    for(int i = 0; i < (NR_TASKS+1); i ++){
		for(int j = 0; j < (NR_INODE); j ++){
			permission_map[i][j] = 1;
		}
	}
    for(int i = NR_TASKS+1; i < (NR_TASKS + NR_PROCS); i ++){
		for(int j = 0; j < (NR_INODE); j ++){
			permission_map[i][j] = 0;
		}
	}
    // INIT需要打开tty0,cmd.tar,kernel.bin 7:0,3,4,需要接管
    permission_map[7][0] = 1;
    permission_map[7][5] = 1;
    // INIT_11需要打开tty1，kernel.bin 11,1,4
    permission_map[11][1] = 1;
    // permission_map[11][3] = 1;
    // permission_map[11][4] = 1;
    // INIT_12需要打开tty2 12,2
    permission_map[12][2] = 1;
    // permission_map[11][2] = 1;
    // permission_map[12][4] = 1;
    // for(int i = 0; i < (NR_TASKS + NR_PROCS); i ++){
	// 	for(int j = 0; j < (NR_INODE); j ++){
	// 		permission_map[i][j] = 1;
	// 	}
	// }
}

PRIVATE int check_permission(){
    // printl("want to check permission.\n");
    if(m_msg.FLAGS&O_CREAT == 1){  //O_CREAT|O_RDWR，由此出现了3
        return 1;
    }

    char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = m_msg.NAME_LEN;	/* length of filename */
    int src = m_msg.source;
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_M, pathname),
		  (void*)va2la(src, m_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;
    // printl("===>filepath %s, pid %d\n", pathname, src);

    if(!strcmp(pathname, "/dev_tty0")){
        return permission_map[src][0];
    }
    if(!strcmp(pathname, "/dev_tty1")){
        return permission_map[src][1];
    }
    if(!strcmp(pathname, "/dev_tty2")){
        return permission_map[src][2];
    }
    int inode = search_file(pathname); //不能使用m_msg.PATHNAME
    pcaller     = &proc_table[src];
    printl("===>option %d, process is %s, filepath %s, pid is %d, inode is %d, permission %d.\n", m_msg.FLAGS, pcaller->name, pathname, src, inode, permission_map[src][inode]);
    return permission_map[src][inode];
    // return 1;
}

PRIVATE int add_permission(){
    char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = m_msg.NAME_LEN;	/* length of filename */
    int src = m_msg.source;
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_M, pathname),
		  (void*)va2la(src, m_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;
    // printl("===>filepath %s, pid %d\n", pathname, src);

    int inode = search_file(m_msg.PATHNAME);
    permission_map[src][inode] = 1;
    return 0;
}

PRIVATE int clear_permission(){
    char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = m_msg.NAME_LEN;	/* length of filename */
    int src = m_msg.source;
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_M, pathname),
		  (void*)va2la(src, m_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;
    // printl("===>filepath %s, pid %d\n", pathname, src);

    int inode = search_file(m_msg.PATHNAME);
    permission_map[src][inode] = 0;
    return 0;
}
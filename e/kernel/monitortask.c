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
PRIVATE int permit_instructions();
PRIVATE int permit_files();
PRIVATE int rpermit_files();
PRIVATE int do_cipher();

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
        case PERMIT_P:
            m_msg.RETVAL = permit_instructions();
            // printl("the permission: %d\n", m_msg.RETVAL);
            break;
        case PERMIT_F:
            m_msg.RETVAL = permit_files();
            // printl("the permission: %d\n", m_msg.RETVAL);
            break;
        case RPERMIT_F:
            m_msg.RETVAL = rpermit_files();
            // printl("the permission: %d\n", m_msg.RETVAL);
            break;
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
        case CIPHER:
            m_msg.RETVAL = do_cipher();
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
    for(int i = 0; i < (NR_TASKS); i ++){
		for(int j = 0; j <= (NR_INODE); j ++){
			permission_map[i][j] = 1;
		}
	}
    for(int i = NR_TASKS; i < (NR_TASKS + NR_PROCS); i ++){
		for(int j = 0; j <= (NR_INODE); j ++){
			permission_map[i][j] = 0;
		}
	}
    // INIT需要打开tty0,cmd.tar,kernel.bin 7:0,3,4,需要接管
    permission_map[7][2] = 1;
    permission_map[7][5] = 1;
    // INIT_11需要打开tty1，可以执行所有指令 11,1,4
    permission_map[11][3] = 1;
    // // permission_map[11][3] = 1;
    // // permission_map[11][4] = 1;
    // // INIT_12需要打开tty2 12,2
    // permission_map[12][] = 1;
    // permission_map[11][2] = 1;
    permission_map[12][4] = 1;
    // for(int i = 0; i < (NR_TASKS + NR_PROCS); i ++){
	// 	for(int j = 0; j < (NR_INODE); j ++){
	// 		permission_map[i][j] = 1;
	// 	}
	// }
}

PRIVATE int check_permission(){
    // printl("want to check permission.\n");
    if((m_msg.FLAGS&O_CREAT) == 1){  //O_CREAT|O_RDWR，由此出现了3
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

    //居然可能找不到这些文件
    if(!strcmp(pathname, "/dev_tty0")){
        return permission_map[src][2];
    }
    if(!strcmp(pathname, "/dev_tty1")){
        return permission_map[src][3];
    }
    if(!strcmp(pathname, "/dev_tty2")){
        return permission_map[src][4];
    }
    int inode = search_file(pathname); //不能使用m_msg.PATHNAME
    pcaller     = &proc_table[src];
    printl("====>option %d, process is %s, filepath %s, pid is %d, inode is %d, permission %d\n", m_msg.FLAGS, pcaller->name, pathname, src, inode, permission_map[src][inode]);
    return permission_map[src][inode];
    // return 1;
}

PRIVATE int add_permission(){
    char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = m_msg.NAME_LEN;	/* length of filename */
    int pid = m_msg.PROC_NR;
    int src = m_msg.source;
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_M, pathname),
		  (void*)va2la(src, m_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;

    int inode = search_file(pathname);  //注意不是m_msg.PATHNAME
    permission_map[pid][inode] = 1;

    printl("===>ADD permission, filepath %s, pid is %d, inode is %d, permission %d.\n", pathname, pid, inode, permission_map[pid][inode]);
    return 0;
}

PRIVATE int clear_permission(){
    char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = m_msg.NAME_LEN;	/* length of filename */
    int src = m_msg.source;
    int pid = m_msg.PROC_NR;
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_M, pathname),
		  (void*)va2la(src, m_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;
    // printl("===>filepath %s, pid %d\n", pathname, src);

    int inode = search_file(pathname);
    permission_map[pid][inode] = 0;

    pcaller     = &proc_table[src];
    printl("===>CLEAR permission, filepath %s, pid is %d, inode is %d, permission %d.\n", pathname, pid, inode, permission_map[pid][inode]);
    return 0;
}

PRIVATE int permit_instructions(){
    int src = m_msg.source;
    for(int i = SYS_FILE + 1; i <= SYS_FILE+PROG_FILE; i ++){
        permission_map[src][i] = 1;
    }
    return 0;
}

PRIVATE int permit_files(){
    int src = m_msg.source;
    for(int i = SYS_FILE+PROG_FILE + 1; i <= NR_INODE; i ++){
        permission_map[src][i] = 1;
    }
    permission_map[src][1] = 1;
    return 0;
}

PRIVATE int rpermit_files(){
    int src = m_msg.source;
    for(int i = SYS_FILE+PROG_FILE + 1; i <= NR_INODE; i ++){
        permission_map[src][i] = 0;
    }
    permission_map[src][1] = 0;
    printl("===>RPERMIT permission, pid is %d\n", src);
    return 0;
}

PRIVATE int do_cipher(){
    // char buf[MBUF_SIZE];
    // buf[0] = 'a'; 这样搞不行 增加了mbuf2
    // printl("|||now in do_cipher, buf");
    int bytes = m_msg.CNT;
    // phys_copy(buf, (void*)va2la(TASK_M, mbuf), bytes);
    // printl("|||in do_cipher, check buf: %s\n", buf);
    cipher((void*)va2la(TASK_M, mbuf2), (void*)va2la(TASK_M, mbuf), bytes);
    // // printl("|||in do_cipher, check buf: %s\n", buf);
    // phys_copy((void*)va2la(TASK_M, mbuf), (void*)va2la(TASK_M, mbuf2), bytes);
    return 0;
}


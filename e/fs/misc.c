// clang-format off

/*************************************************************************//**
 *****************************************************************************
 * @file   misc.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

/* Orange'S FS */

#include "type.h"
#include "mdirent.h"
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
#include "hd.h"
#include "fs.h"

/*****************************************************************************
 *                                do_stat
 *************************************************************************//**
 * Perform the stat() syscall.
 * 
 * @return  On success, zero is returned. On error, -1 is returned.
 *****************************************************************************/
PUBLIC int do_stat()
{
	char pathname[MAX_PATH]; /* parameter from the caller */
	char filename[MAX_PATH]; /* directory has been stipped */

	/* get parameters from the message */
	int name_len = fs_msg.NAME_LEN;	/* length of filename */
	int src = fs_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_FS, pathname),    /* to   */
		  (void*)va2la(src, fs_msg.PATHNAME), /* from */
		  name_len);
	pathname[name_len] = 0;	/* terminate the string */

	int inode_nr = search_file(pathname);
	DEBUG_PRINT("do_stat", "1 searchfile success");
	if (inode_nr == INVALID_INODE) {	/* file not found */
		printl("{FS} FS::do_stat():: search_file() returns "
		       "invalid inode: %s\n", pathname);
		return -1;
	}

	struct inode * pin = 0;

	struct inode * dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0) {
		/* theoretically never fail here
		 * (it would have failed earlier when
		 *  search_file() was called)
		 */
		assert(0);
	}
	pin = get_inode(dir_inode->i_dev, inode_nr);

	struct stat s;		/* the thing requested */
	s.st_dev = pin->i_dev;
	s.st_ino = pin->i_num;
	s.st_mode= pin->i_mode;
	s.st_rdev= is_special(pin->i_mode) ? pin->i_start_sect : NO_DEV;
	s.st_size= pin->i_size;

	struct inode* traverse_node = pin;

	while (traverse_node->i_next_node != INVALID_INODE) {
		traverse_node = get_inode(traverse_node->i_dev,traverse_node->i_next_node); // pin link起初没引用
		s.st_size += traverse_node->i_size;
	}

	put_inodes_link(pin); // 去除对lseek时的所有引用

	phys_copy((void*)va2la(src, fs_msg.BUF), /* to   */
		  (void*)va2la(TASK_FS, &s),	 /* from */
		  sizeof(struct stat));

	return 0;
}

int last_query_dev = -1;

/**
 * low_search_entry
 * 
 * A lowlevel function to search specified inode_nr of the file or directory 
 * through the entry name
 * 
 * @param[in] dirNode current directory inode which searching starts
 * @param[in] entryName target entry;s name
 * @return 	Ptr to the i-node of the file if successful, otherwise zero.
 */
PUBLIC int low_search_entry(struct inode* dir_inode, char *entryName){
	int i, j;
	int m = 0;
	/**
	 * Search the dir for the entry.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries =
	  dir_inode->i_size / DIR_ENTRY_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	struct dir_entry * pde;
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (memcmp(entryName, pde->name, MAX_FILENAME_LEN) == 0) {
				last_query_dev = dir_inode->i_dev;
				return pde->inode_nr;
			}
				
			if (++m > nr_dir_entries)
				break;
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}
	return INVALID_INODE;
}



/*****************************************************************************
 *                                search_file
 *****************************************************************************/
/**
 * An advanced function to search the file and return the first inode_nr of the file.
 * 
 * WILL NOT MODIFY ANY ELEMENT OF THE INODE
 *
 * @param[in] path The full path of the file to search.
 * @return         Ptr to the i-node of the file if successful, otherwise zero.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
PUBLIC int search_file(char * path)
{
	int i, j;

	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode * dir_inode;
	if (strip_path(filename, path, &dir_inode) != 0)
		return INVALID_INODE;

	if (filename[0] == 0)	/* path: "/" */
		return dir_inode->i_num;

	int nr = low_search_entry(dir_inode, filename);
	
	// free dir inode
	if (dir_inode->i_num != ROOT_INODE) {
		put_inode(dir_inode);
	}

	return nr;
}

/** List_Dir
 * 
 * Modified from `low_search_file`. 
 * 
 * Used for `ls` which will enumerate all entry in the dir
 * except paired to specified name.
 * 
 */
PUBLIC void do_List_Dir() {
	int fd = fs_msg.u.m3.m3i1;
	int i = fs_msg.u.m3.m3i2;
	int j = fs_msg.u.m3.m3i3;
	int dev = fs_msg.u.m3.m3i4;
	int m = fs_msg.u.m3.m3l1;
	struct dirent* dirent_out = (struct dirent*)fs_msg.u.m3.m3p1;
	// msg.u.m3.m3p1 这个是想要返回的dirent
	/*
	struct dirent {
	char *filename;
	u32 i_mode;
	u32 i_size;
};
	 */
	struct inode* dir_inode = pcaller->filp[fd]->fd_inode;	
	/**
	 * Search the dir for the entry.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries =
	  dir_inode->i_size / DIR_ENTRY_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	struct dir_entry * pde;

	 // 遍历目录块
    for (; i < nr_dir_blks; i++) {
        // 读取当前块内容
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
        pde = (struct dir_entry *)fsbuf;  // 将当前块的内容指针转化为目录项指针

        // 遍历当前块中的目录项
        for (; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) {
            // 如果遍历到的目录项数大于总目录项数，则结束
            if (++m > nr_dir_entries)
                break;

            // 将目录项信息填充到 dirent_out 中
			_strcpy(dirent_out->filename, pde->name);
			struct inode* tmp_node = get_inode_directly(dev, pde->inode_nr);

            dirent_out->i_mode = tmp_node->i_mode;      // 文件模式
            dirent_out->i_size = getsize(tmp_node);      // 文件大小

            // 更新索引
            fs_msg.u.m3.m3i2 = i;  // 当前块索引
            fs_msg.u.m3.m3i3 = j;  // 当前目录项索引

            // 返回当前目录项
			fs_msg.u.m3.m3l1 = m;
            return;
        }

        // 如果已经遍历完当前块中的所有目录项，继续遍历下一个块
        if (m > nr_dir_entries) /* all entries have been iterated */
            break;
    }

    // 如果遍历完成，返回
	dirent_out->filename[0] = '\0';
	fs_msg.u.m3.m3l1 = m;
    return;
}

/*****************************************************************************
 *                                strip_path
 *****************************************************************************/
/**
 * Get the basename from the fullpath.
 *
 * In Orange'S FS FORK v2.0, support multi-path entry.
 * Sub-folder thing is permitted.
 * 
 *
 * This routine should be called at the very beginning of file operations
 * such as open(), read() and write(). It accepts the full path and returns
 * two things: the basename and a ptr of the root dir's i-node.
 *
 * e.g. After stip_path(filename, "/blah", ppinode) finishes, we get:
 *      - filename: "blah"
 *      - *ppinode: root_inode
 *      - ret val:  0 (successful)
 *
 * Currently an acceptable pathname supports multiplied `/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'.
 *
 * @param[out] filename The string for the result.
 * @param[in]  pathname The full pathname(MUST started with '/').
 * @param[out] ppinode  The ptr of the dir's inode will be stored here.
 * 
 * @return Zero if success, otherwise the pathname is not valid.
 *****************************************************************************/
PUBLIC int strip_path(char * filename, const char * pathname,
		      struct inode** ppinode)
{
	const char * s = pathname;

	// 目前的entry名字(可能是file or dir)
	// 用作暂时的buffer
	char * cur_entry_name = filename; 
	struct inode* curDirInode = root_inode;
	u8 IsRootDir = 1;

	if (s == 0) // fail
		return -1;

	if (*s == '/')
		s++;
	else
		return -1;

	while (*s) {		/* check each character */
		if (*s == '/'){
			// filename[-1] = 0
			*cur_entry_name = 0;
			// 根据当前的entry name找inode
			int next_dir_inode_nr = low_search_entry(curDirInode, filename);
			if (next_dir_inode_nr == INVALID_INODE) return -1;
			int cur_dev = curDirInode->i_dev;
			if (IsRootDir == 0) put_inode(curDirInode); // 先前的dir不需要再引用了
			curDirInode = get_inode(cur_dev, next_dir_inode_nr);
			
			// buffer指针重新回到开头
			cur_entry_name = filename;
		}
		*cur_entry_name++ = *s++;
		/* if filename is too long, just truncate it */
		if (cur_entry_name - filename >= MAX_FILENAME_LEN)
			break;	
	}
	// loop finished, when the final curDirInode is the dir inode of the file
	// cur_entry_name is the file's name

	*cur_entry_name = 0;

	*ppinode = curDirInode;

	return 0;
}


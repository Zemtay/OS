// clang-format off

/*************************************************************************//**
 *****************************************************************************
 * @file   read_write.c
 * @brief  
 * @author scarletborder
 * @date   2024
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


/*****************************************************************************
 *                                do_rdwt
 *****************************************************************************/
/**
 * Read/Write file and return byte count read/written.
 *
 * Sector map is not needed to update, since the sectors for the file have been
 * allocated and the bits are set when the file was created.
 * 
 * @return How many bytes have been read/written.
 *****************************************************************************/
PUBLIC int do_rdwt()
{
	int fd = fs_msg.FD;	/**< file descriptor. */
	void * buf = fs_msg.BUF;/**< r/w buffer */
	int len = fs_msg.CNT;	/**< r/w bytes */

	int src = fs_msg.source;		/* caller proc nr. */

	assert((pcaller->filp[fd] >= &f_desc_table[0]) &&
	       (pcaller->filp[fd] < &f_desc_table[NR_FILE_DESC]));

	if (!(pcaller->filp[fd]->fd_mode & O_RDWR))
		return 0;

	u64 pos = pcaller->filp[fd]->fd_pos;

	u64 real_size = 0; // file size
	u32 real_sects_num = 0; // sum of sections

	struct inode * pin = pcaller->filp[fd]->fd_inode;

	assert(pin >= &inode_table[0] && pin < &inode_table[NR_INODE]);

	int imode = pin->i_mode & I_TYPE_MASK;

	if (imode == I_CHAR_SPECIAL) {
		int t = fs_msg.type == READ ? DEV_READ : DEV_WRITE;
		fs_msg.type = t;

		int dev = pin->i_start_sect;
		assert(MAJOR(dev) == 4);

		fs_msg.DEVICE	= MINOR(dev);
		fs_msg.BUF	= buf;
		fs_msg.CNT	= len;
		fs_msg.PROC_NR	= src;
		assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
		send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &fs_msg);
		assert(fs_msg.CNT == len);

		return fs_msg.CNT;
	}
	else {
		assert(pin->i_mode == I_REGULAR || pin->i_mode == I_DIRECTORY);
		assert((fs_msg.type == READ) || (fs_msg.type == WRITE));

		u64 pos_end;

		// Traverse every inode until the numeric of next node is 0
		struct inode* traverse_node = pin;

		// count the real size of the inode link
		while (1){
			printl("traversenode size is %d\n", traverse_node->i_size);
			real_size += traverse_node->i_size;
			real_sects_num += traverse_node->i_nr_sects;

			if (traverse_node->i_next_node == INVALID_INODE) {
				break;
			}
			traverse_node = get_inode_directly(traverse_node->i_dev, traverse_node->i_next_node);
		}


		// Traverse every sector in this inode
		if (fs_msg.type == READ)
			pos_end = min(pos + len, real_size);
		else		/* WRITE */
			pos_end = min(pos + len, real_sects_num * SECTOR_SIZE);


		// 目标是在link中的第几个sector
		u64 start_sector = pos >> SECTOR_SIZE_SHIFT;
		u64 end_sector = pos_end >> SECTOR_SIZE_SHIFT;

		u64 traveled_sector = 0;
		traverse_node = pin;

		// 先找到开始的那个inode
		while (1) {
			if (traveled_sector + traverse_node->i_nr_sects >= start_sector) {
				break;
			}
			// this inode does not contain the start sector
			traveled_sector += traverse_node->i_nr_sects;
			if (traverse_node->i_next_node == INVALID_INODE) {
				return 0;
			}
			traverse_node = get_inode_directly(traverse_node->i_dev, traverse_node->i_next_node);
		}
		u64 before_sector_num = traveled_sector;
		printl("end sec=%d, befsec=%d\n",end_sector,before_sector_num);
		struct inode* start_inode = traverse_node;
		while (1) {
			if (traveled_sector + traverse_node->i_nr_sects >= end_sector) {
				break;
			}
			// this inode does not contain the end sector
			traveled_sector += traverse_node->i_nr_sects;
			if (traverse_node->i_next_node == INVALID_INODE) {
				return 0;
			}
			traverse_node = get_inode_directly(traverse_node->i_dev, traverse_node->i_next_node);
		}
		printl("%d",traveled_sector);
		struct inode* end_inode = traverse_node;
		traverse_node = start_inode;

		// 找到pos所在的Inode
		int off = pos % SECTOR_SIZE;

		// Start to traverse from current start_inode (the first inode of the pos among the file)
		// until it reach the inode which contains `pos_end`
		int bytes_rw = 0;
		int bytes_left = len;
		pos = pos - before_sector_num * SECTOR_SIZE; // 相对于起始inode的pos
		pos_end = pos_end - traveled_sector * SECTOR_SIZE;// 相对于最后inode的pos end
		while (1){
			int rw_sect_min = traverse_node->i_start_sect+(pos>>SECTOR_SIZE_SHIFT);

			int rw_sect_max;
			if (traverse_node->i_num == end_inode->i_num) {
				rw_sect_max = traverse_node->i_start_sect+(pos_end>>SECTOR_SIZE_SHIFT);
			}else{
				rw_sect_max = traverse_node->i_start_sect + traverse_node->i_nr_sects;
			}

			int chunk = min(rw_sect_max - rw_sect_min + 1,
					FSBUF_SIZE >> SECTOR_SIZE_SHIFT);

			int i;

			for (i = rw_sect_min; i <= rw_sect_max; i += chunk) {
				/* read/write this amount of bytes every time */
				int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);
				rw_sector(DEV_READ,
					pin->i_dev,
					i * SECTOR_SIZE,
					chunk * SECTOR_SIZE,
					TASK_FS,
					fsbuf);

				if (fs_msg.type == READ) {
					phys_copy((void*)va2la(src, buf + bytes_rw),
						(void*)va2la(TASK_FS, fsbuf + off),
						bytes);
				}
				else {	/* WRITE */
					phys_copy((void*)va2la(TASK_FS, fsbuf + off),
						(void*)va2la(src, buf + bytes_rw),
						bytes);
					rw_sector(DEV_WRITE,
						pin->i_dev,
						i * SECTOR_SIZE,
						chunk * SECTOR_SIZE,
						TASK_FS,
						fsbuf);
				}
				off = 0;
				bytes_rw += bytes;
				pcaller->filp[fd]->fd_pos += bytes;
				bytes_left -= bytes;
			}	

			if (traverse_node->i_num == end_inode->i_num) {
				break;
			}
			traverse_node = get_inode_directly(traverse_node->i_dev, traverse_node->i_next_node);
		}
		
		// size is larger than original, which means the final inode need to make change
		printl("%d",traveled_sector);
		if (pcaller->filp[fd]->fd_pos > real_size && fs_msg.type == WRITE) { 		
			/* update inode::size */

			// TODO: read幽默结束后fdpos=0导致enlarge
			printl("need to enlarge file %d to pos %d\n", real_size, pcaller->filp[fd]->fd_pos);
			end_inode->i_size = pcaller->filp[fd]->fd_pos - traveled_sector * SECTOR_SIZE;
			printl("node=%d",end_inode->i_size);
			/* write the updated i-node back to disk */
			sync_inode_link(pin);
			printl("-%d\n", pin->i_size);

		}
		sync_inode_link(pin);
		return bytes_rw;
	}
}

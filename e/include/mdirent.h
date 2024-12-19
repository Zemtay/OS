#ifndef _SCARLETBORDER_MDIRENT_H_
#define _SCARLETBORDER_MDIRENT_H_

#include "type.h"

typedef struct {
	int fd;
	int i;
	int j;
	int dev;
	int m;
} DIR;

// directly use char*, use `stat()` to get attributes of dir instead
// struct dirent {
// 	char *filename;
// 	u32 i_mode;
// 	u32 i_size;
// };

int opendir(char *path, DIR *d);

void readdir(DIR *dir, char *ret);

int closedir(DIR *d);

#endif
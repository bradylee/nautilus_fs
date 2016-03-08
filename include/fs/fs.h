#ifndef __FS_H
#define __FS_H

#include <dev/block.h>

struct file_operations {
	uint32_t (*open)(uint8_t*, char*, int);
	size_t (*read)(int, char*, size_t, size_t);
	size_t (*write)(int, char*, size_t, size_t);
};

#endif

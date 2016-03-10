#ifndef __FS_EXT2_H__
#define __FS_EXT2_H__

#include <dev/block.h>
#include <fs/ext2/ext2_access.h>

static uint8_t EOF = 255;

uint32_t ext2_open(uint8_t *device, char *path, int access);
size_t ext2_read(int inode_number, char *buf, size_t num_bytes, size_t offset);
size_t ext2_write(int inode_number, char *buf, size_t num_bytes, size_t offset);

uint64_t ext2_get_file_size(int inode_number);

//void ext2_set_file_int(struct file_int *fi);

/*
static struct file_int ext2_file_int = {
	.open = ext2_open,
	.read = ext2_read,
	.write = ext2_write
};
*/

#endif


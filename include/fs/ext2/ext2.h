#ifndef __FS_EXT2_H
#define __FS_EXT2_H

#include <fs/fs.h>
#include <fs/ext2/ext2_access.h>

uint32_t ext2_open(uint8_t *device, char *path, int access);
size_t ext2_read(int inode_number, char *buf, size_t num_bytes, size_t offset);
size_t ext2_write(int inode_number, char *buf, size_t num_bytes, size_t offset);
uint64_t ext2_get_file_size(int inode_number);

void ext2_set_file_op(struct file_operations *);
#endif

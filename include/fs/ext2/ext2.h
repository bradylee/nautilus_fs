#ifndef __FS_EXT2_H__
#define __FS_EXT2_H__

#include <dev/block.h>
#include <fs/ext2/ext2_access.h>

static uint8_t EOF = 255;

uint32_t ext2_open(uint8_t *device, char *path, int access);
size_t ext2_read(int inode_number, char *buf, size_t num_bytes, size_t offset);
size_t ext2_write(int inode_number, char *buf, size_t num_bytes, size_t offset);

uint64_t ext2_get_file_size(int inode_number);
uint32_t ext2_file_exist(uint8_t *device, char *path);
uint32_t ext2_file_create(uint8_t *device, char *path);
uint32_t add_to_dir(uint8_t *device, int dir_inode_number, int target_inode_number, char* name, uint8_t file_type);

#endif


#ifndef __FS_EXT2_H__
#define __FS_EXT2_H__

#include <dev/block.h>
#include <fs/ext2/ext2_access.h>

#define EXT2_S_IFREG 0x8000

static uint8_t EOF = 255;

int ext2_open(uint8_t *device, char *path, int access);
ssize_t ext2_read(uint8_t *device, int inode_number, char *buf, size_t num_bytes, size_t offset);
ssize_t ext2_write(uint8_t *device, int inode_number, char *buf, size_t num_bytes, size_t offset);
size_t ext2_get_file_size(uint8_t *device, int inode_number);

int ext2_file_exists(uint8_t *device, char *path);
uint32_t ext2_file_create(uint8_t *device, char *path);
int ext2_file_delete(uint8_t *device, char *path);
uint32_t directory_add_file(uint8_t *device, int dir_inode_number, int target_inode_number, char* name, uint8_t file_type);

#endif


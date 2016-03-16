#ifndef __FS_EXT2_H__
#define __FS_EXT2_H__

#include <dev/block.h>
#include <fs/ext2/ext2_access.h>

#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFDIR 0x4000
#define BLOCK_SIZE 1024
#define DENTRY_ALIGN 4
#define NUM_DATA_BLOCKS 12
#define ENDFILE 0xa0

//static uint8_t EOF = 255;

uint32_t ext2_open(uint8_t *device, char *path, int access);
ssize_t ext2_read(uint8_t *device, int inode_number, char *buf, size_t num_bytes, size_t offset);
ssize_t ext2_write(uint8_t *device, int inode_number, char *buf, size_t num_bytes, size_t offset);
size_t ext2_get_size(uint8_t *device, int inode_number);
size_t ext2_get_file_size(uint8_t *device, int inode_number);
size_t ext2_get_dir_size(uint8_t *device, int inode_number);

//uint32_t ext2_get_directory_inode(uint8_t *device, char *path);
int ext2_file_exists(uint8_t *device, char *path);
uint32_t ext2_file_create(uint8_t *device, char *path);
int ext2_file_delete(uint8_t *device, char *path);
int ext2_dir_add_file(uint8_t *device, int dir_inum, int target_inum, char* name, uint8_t file_type);
int ext2_dir_remove_file(uint8_t *device, int dir_inum, int target_inum);

uint16_t ext2_dentry_find_len(struct ext2_dir_entry_2 *dentry);

int ext2_inode_has_mode(struct ext2_inode *inode, int mode);

#endif

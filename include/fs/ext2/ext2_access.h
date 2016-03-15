#ifndef __EXT2_ACCESS_H
#define __EXT2_ACCESS_H

#include "nautilus/naut_types.h"
#include "fs/ext2/ext2fs.h"

struct ext2_super_block *get_super_block(void *fs);
uint32_t get_block_size(void *fs);
void* get_block(void *fs, uint32_t block_num);
struct ext2_group_desc *get_block_group(void *fs, uint32_t block_group_num);
struct ext2_inode *get_inode(void *fs, uint32_t inode_num);
char** split_path(char *path, int *num_parts); 
struct ext2_inode *get_root_dir(void *fs);
uint32_t get_inode_from_dir(void *fs, struct ext2_inode *dir, char *name);
uint32_t get_inode_by_path(void *fs, char *path);
uint32_t get_free_inode(void *fs);

#endif


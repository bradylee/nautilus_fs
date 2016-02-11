#ifndef __NAUTILUS_FS_H__
#define __NAUTILUS_FS_H__

//#include <nautilus/printk.h>
#include <nautilus/list.h>

extern uint8_t ramdisk_start, ramdisk_end;

struct super_block {
	struct list_head s_list;
	uint32_t s_blocksize;
	uint64_t s_maxbytes; // max file size
	struct file_system_type * s_type;
	uint32_t s_magic;
	struct list_head s_inodes;
	struct list_head s_files;
	char* id; // container device
	// ...
};

/* super block methods */
// alloc_inode()
// destroy_inode()
// read_inode()
// write_inode()
// ...

struct inode {
	struct list_head i_list;
	struct list_head i_dentry;
	uint64_t i_size; // file length
	uint32_t i_blksize;
	uint32_t i_blocks;
	struct super_block *i_sb;
	// ...
};

/* inode methods */
// create()
// lookup()
// mkdir()
// rmdir()
// rename()
// ...

struct dentry {
	//spinlock_t d_lock;
	struct inode *d_inode;
	struct dentry *d_parent;
	struct list_head d_subdirs;
	struct super_block * d_sb;
	// ...
};

/* dentry methods */
// ...

struct file {
	struct list_head f_list;
	struct dentry *f_dentry;
	// ...
};

/* file methods */
// read()
// write()
// open()
// lock()
// release()
// ...



/*
enum file_access_flags {O_RDONLY, O_WRONLY, O_RDWR, O_APPEND};

struct block_device {
	uint64_t size;
	uint8_t *contents;
};

struct file {
	uint64_t size;
	uint64_t position;
	uint8_t *contents;
};

void init_fs(void);
int mount_block_device(struct block_device *device);
int unmount_block_device(struct block_device *device);

int open(char *filepath, int access);
size_t read(int fileid, char *dst, size_t n);
size_t write(int fileid, char *src, size_t n);
int close(int fileid);
int seek(int fileid, size_t position);
size_t tell(int fileid);

int ext2_open(char *filepath, int access);
size_t ext2_read(int fileid, char *dst, size_t n);
size_t ext2_write(int fileid, char *src, size_t n);
int ext2_close(int fileid);
int ext2_seek(int fileid, size_t position);
size_t ext2_tell(int fileid);
*/

#endif

#ifndef __FS_H__
#define __FS_H__

#include <nautilus/naut_types.h>
#include <nautilus/printk.h>
#include <nautilus/list.h>
#include <nautilus/spinlock.h>

#include <dev/block.h>
#include <fs/ext2/ext2.h>

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 3 // OR of RD and WR ONLY
#define O_APPEND 4
#define O_CREAT 8
// there are more ...

enum Filesystem {ext2, fat32};

struct filesystem {
	char *path;
	struct block_dev *device;
	enum Filesystem fs_type;
};

struct file_int {
	uint32_t (*open)(uint8_t*, char*, int);
	ssize_t (*read)(uint8_t*, int, char*, size_t, size_t);
	ssize_t (*write)(uint8_t*, int, char*, size_t, size_t);
	size_t (*get_size)(uint8_t*, int);
};

struct file {
	struct list_head file_node;
	struct file_int interface;
	size_t position;
	uint32_t filenum;
	uint32_t fileid;
	int access;
	spinlock_t lock;
};

void mount(char *source, char *target);
void umount(char *target);

void test_fs(void);
void init_fs(void);
void deinit_fs(void);

int open(char *path, int access);
int close(uint32_t filenum);
int remove(char *path);
int exists(char *path);
ssize_t read(int filenum, char *buf, size_t num_bytes);
ssize_t write(int filenum, char *buf, size_t num_bytes);
ssize_t lseek(int filenum, size_t offset, int whence);
ssize_t tell(int filenum);

void __close(struct file *fd);
ssize_t __lseek(struct file *fd, size_t offset, int whence);


void directory_list(char *path);

#endif

#ifndef __VFS_H
#define __VFS_H

#include <nautilus/list.h>
#include <nautilus/spinlock.h>

#include <fs/fs.h>
#include <fs/ext2/ext2.h>

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 3
#define O_APPEND 4
// there are more ...

//extern uint8_t RAMFS_START, RAMFS_END;
static uint8_t EOF = 255;

static struct {
	struct list_head head;
	spinlock_t lock;
} open_files;

struct file_data {
	struct list_head file_node;
	struct file_operations *op;
	size_t position;
	//uint32_t (*open)(uint8_t*, char*, int);
	//size_t (*read)(int, char*, size_t, size_t);
	//size_t (*write)(int, char*, size_t, size_t);
	uint32_t filenum;
	uint32_t fileid;
	int access;
	spinlock_t lock;
};

void test_fs(void);
void init_fs(void);
void deinit_fs(void);

void __file_close(struct file_data*);
size_t __file_seek(struct file_data*, size_t offset, int pos);
void __file_print(struct file_data*);
void __iterate_opened(void (*callback)(struct file_data*));
int __file_has_access(struct file_data*, int access);

size_t file_open(char *path, int access);
int file_close(uint32_t filenum);
size_t file_seek(int filenum, size_t offset, int pos);
size_t file_read(int filenum, char *buf, size_t num_bytes);
size_t file_write(int filenum, char *buf, size_t num_bytes);
//size_t file_append(int filenum, char* write_data, size_t num_bytes);

struct file_data* __get_open_file(uint32_t filenum);

void dir_ls(char* path);

#endif

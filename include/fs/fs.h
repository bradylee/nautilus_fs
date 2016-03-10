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
#define O_RDWR 3
#define O_APPEND 4
// there are more ...

enum Filesystem {ext2, fat32};

static struct {
	struct list_head head;
	spinlock_t lock;
} open_files;

struct file_int {
  uint32_t (*open)(uint8_t*, char*, int);
  size_t (*read)(int, char*, size_t, size_t);
  size_t (*write)(int, char*, size_t, size_t);
  uint64_t (*get_size)(int);
};

struct file_data {
	struct list_head file_node;
	struct file_int interface;
	size_t position;
	uint32_t filenum;
	uint32_t fileid;
	int access;
	spinlock_t lock;
};


void test_fs(void);
void init_fs(void);
void deinit_fs(void);

void __file_set_interface(struct file_int *, enum Filesystem);
struct file_data* __get_open_file(uint32_t filenum);

void __file_close(struct file_data*);
size_t __file_seek(struct file_data*, size_t offset, int whence);
void __file_print(struct file_data*);
void __iterate_opened(void (*callback)(struct file_data*));
int __file_has_access(struct file_data*, int access);

size_t file_open(char *path, int access);
int file_close(uint32_t filenum);
size_t file_seek(int filenum, size_t offset, int whence);
size_t file_tell(int filenum);
size_t file_read(int filenum, char *buf, size_t num_bytes);
size_t file_write(int filenum, char *buf, size_t num_bytes);
//size_t file_append(int filenum, char* write_data, size_t num_bytes);

void dir_ls(char* path);

#endif

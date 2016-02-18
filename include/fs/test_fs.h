#ifndef __TEST_FS_H
#define __TEST_FS_H

static uint8_t EOF = 255;

void test_fs(void);
uint32_t ext2_open(char * path, int access);
size_t ext2_read(int file_number, char * dst, size_t num_bytes, size_t offset);
size_t file_open(char *path, int access);
size_t file_read(int file_number, char *dst, size_t num_bytes);
void dir_ls(char* path);

struct file_data {
	int status; //closed = -1, opened = thread_id
	size_t position;
	uint32_t inode;
};

static struct file_data open_files[10];

#endif

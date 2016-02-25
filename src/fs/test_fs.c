#include <fs/fs.h>
#include "fs/ext2/ext2_access.h"
#include "fs/test_fs.h"

void test_fs() {
	init_fs();
	printk("Opening files...\n");
	uint32_t fn = file_open("/readme", 0);
	fn = file_open("/null", 0);
	fn = file_open("/nothing", 0);
	fn = file_open("/nothing", 0);
	printk("Done.\n");
	print_opened_files();
	printk("Closing...\n");
	file_close(fn);
	print_opened_files();
	deinit_fs();
}

void init_fs(void) {
	printk("Initing list...\n");
	INIT_LIST_HEAD(&open_files);
}

void deinit_fs(void) {
	// close open files and delete list
}

size_t file_open(char *path, int access) {
	uint32_t inode_num = ext2_open(&RAMFS_START, path, access);
	struct file_data *fdp = malloc(sizeof(struct file_data));
		
	fdp->filenum = inode_num;
	// check already opened
	if (!get_opened_file(fdp->filenum)) { 
		printk("Opened %s %d\n", path, fdp->filenum);
		list_add(&fdp->file_node, &open_files);
	}
	return fdp->filenum;
}

uint32_t ext2_open(uint8_t *device, char *path, int access) {
	uint32_t inode_num = (uint32_t) get_inode_by_path(device, path);
	return inode_num;
}

void print_opened_files(void) {
	struct list_head *cur;
	struct file_data *fd;

	list_for_each(cur, &open_files) {
		fd = (struct file_data*)cur;
		printk("%d\n", fd->filenum);
	}
}

int file_close(uint32_t filenum) {
	struct file_data *fd = get_opened_file(filenum);
	if (!fd)
		return 0;

	list_del((struct list_head*)fd);
	free(fd);
	return 1;
}

struct file_data* get_opened_file(uint32_t filenum) {
	struct list_head *cur;
	struct file_data *fd;

	list_for_each(cur, &open_files) {
		fd = (struct file_data*)cur;
		if (fd->filenum == filenum)
			return fd;
	}
	return NULL;
}

//test content reading
void read_contents(uint32_t inode_num) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_num);
	uint32_t num_blocks = inode_pointer->i_blocks;
	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[0]));
	int i = 0;
	printk("num_block: %d\n",num_blocks);
	unsigned char ch;
	do  {
		ch = *(blocks+i);
		printk("%c",(uint32_t)ch,ch);
		i++;
	} while (ch);
}

size_t ext2_read(int inode_number, char * dst, size_t num_bytes,size_t offset) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);
	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[0]));
	int i = 0;	
	uint8_t * cur = blocks + offset;
	while (i<num_bytes && *cur != 0x0a) {		
		dst[i] = *cur;		
		i++;
		cur++;
	}
	dst[i] = 0x00; //null terminator
	return i;
}

size_t file_read(int file_number, char * dst, size_t num_bytes) {
	return 0;
	/*
	struct file_data * target = &open_files[file_number];
	uint32_t inode_num = target->inode;
	size_t n = ext2_read(inode_num, dst, num_bytes,target->position);
	target->position += n;
	printk("n: %d pos: %d\n", n, target->position);
	return n;
	*/
}

void ext2_write() {
}

void dir_ls(char* path) {
	char* tempname;
	struct ext2_dir_entry_2* directory_entry;
	struct ext2_inode * dir = get_inode(&RAMFS_START, get_inode_by_path(&RAMFS_START,path));
	//get pointer to block where directory entries reside using inode of directory
	directory_entry = (struct ext2_dir_entry_2*)get_block(&RAMFS_START,dir->i_block[0]);

	//scan only valid files in directory
	while(directory_entry->inode != 0){

		//create temporary name
		tempname = (char*)malloc(directory_entry->name_len*sizeof(char));
		int i;
		//copy file name to temp name, then null terminate it
		for(i = 0; i < directory_entry->name_len; i++){
			tempname[i] = directory_entry->name[i];
		}
		tempname[i] = '\0';
		if(strcmp(tempname,".") && strcmp(tempname,"..")) { //strcmp -> 0 = equal, !0 = not equal
			printk("%s\n", tempname);
		}
		directory_entry = (struct ext2_dir_entry_2*)((void*)directory_entry+directory_entry->rec_len);

		free(tempname);
	}
}

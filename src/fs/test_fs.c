#include <nautilus/printk.h>
#include <fs/fs.h>

#include "fs/ext2/ext2_access.h"
#include "fs/test_fs.h"

void test_fs() {
// print inode count
	printk("%d\n", *(uint32_t*)(&RAMFS_START+1024));
	// print block count
	printk("%d\n", *(uint32_t*)(&RAMFS_START+1028));

	printk("inode: %d\n", (uint32_t)get_inode_by_path(&RAMFS_START,"/testdir/test"));
	uint32_t inode_num = (uint32_t)get_inode_by_path(&RAMFS_START,"/testdir/test");
	//read_contents(inode_num);
	char * buffer = malloc(21);
	int result = ext2_read((int)inode_num, buffer, (size_t)20, 0);
	printk("%s\n", buffer);

	uint32_t fn = file_open("/testdir/test", 0);
	uint32_t n = file_read(fn, buffer, 5);
	printk("%s\n", buffer);
	n = file_read(fn, buffer, 5);
	printk("%s\n", buffer);
	n = file_read(fn, buffer, 5);
	printk("%s\n\n", buffer);
	
	dir_ls("/testdir");

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


uint32_t ext2_open(char * path, int access) {
	//get inode number of file
	uint32_t inode_num = (uint32_t)get_inode_by_path(&RAMFS_START,path);
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_num);
	
	return inode_num;
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

size_t file_open(char *path, int access) {
	uint32_t inode_num = ext2_open(path, access);
	struct file_data fd;
	fd.inode = inode_num;
	open_files[0] = fd;
	return 0;	
}

size_t file_read(int file_number, char * dst, size_t num_bytes) {
	struct file_data * target = &open_files[file_number];
	uint32_t inode_num = target->inode;
	size_t n = ext2_read(inode_num, dst, num_bytes,target->position);
	target->position += n;
	printk("n: %d pos: %d\n", n, target->position);
	return n;
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

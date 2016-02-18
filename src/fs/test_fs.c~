#include "fs/ext2/ext2_access.h"
#include <nautilus/printk.h>
#include <fs/fs.h>

void test_fs() {
// print inode count
	printk("%d\n", *(uint32_t*)(&RAMFS_START+1024));
	// print block count
	printk("%d\n", *(uint32_t*)(&RAMFS_START+1028));

	printk("inode: %d\n", (uint32_t)get_inode_by_path(&RAMFS_START,"/testdir/test"));
	uint32_t inode_num = (uint32_t)get_inode_by_path(&RAMFS_START,"/testdir/test");
	read_contents(inode_num);
	
}

//test content reading
read_contents(uint32_t inode_num) {
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


open(char * path, int access) {
	//get inode number of file
	uint32_t inode_num = (uint32_t)get_inode_by_path(&RAMFS_START,path);
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_num);
	
	
}

size_t read(int file_number, char * dst, size_t num_bytes) {

}

write() {
}

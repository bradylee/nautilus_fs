#include "fs/ext2/ext2_access.h"
#include "fs/ext2/ext2fs.h"

#define INFO(fmt, args...) printk("EXT2_ACCESS: " fmt "\n", ##args)
#define DEBUG(fmt, args...) printk("EXT2_ACCESS (DEBUG): " fmt "\n", ##args)
#define ERROR(fmt, args...) printk("EXT2_ACCESS (ERROR): " fmt "\n", ##args)

#ifndef NAUT_CONFIG_DEBUG_FILESYSTEM
#undef DEBUG
#define DEBUG(fmt, args...)
#endif

///////////////////////////////////////////////////////////
//  Accessors for the basic components of ext2.
///////////////////////////////////////////////////////////

// Return a pointer to the primary superblock of a filesystem.
struct ext2_super_block *get_super_block(void * fs) {

	//Super block is always a constant offset away from fs
	return (struct ext2_super_block*)(fs+SUPERBLOCK_OFFSET);
}

// Return the block size for a filesystem.
uint32_t get_block_size(void *fs) {
	uint32_t shift;
	struct ext2_super_block* sb;
	sb = get_super_block(fs);

	//s_log_block_size tells how much to shift 1K by to determine block size
	shift = sb->s_log_block_size;
	return (1024 << shift);
}

// Return a pointer to a block given its number.
// get_block(fs, 0) == fs;
void * get_block(void * fs, uint32_t block_num) {
	uint32_t block_size;
	block_size = get_block_size(fs);

	//gets block size, adds block offset to beginning of fs
	return (void*)(block_size*block_num + fs);
}

// Return a pointer to the first block group descriptor in a filesystem. Real
// ext2 filesystems will have several of these, but, for simplicity, we will
// assume there is only one.
struct ext2_group_desc *get_block_group(void * fs, uint32_t block_group_num) {
	struct ext2_super_block* sb;
	sb = get_super_block(fs);

	//returns location directly after superblock, which is where first block group descriptor resides 
	return (struct ext2_group_desc *)((void*)sb+SUPERBLOCK_SIZE);
}


// Return a pointer to an inode given its number. In a real filesystem, this
// would require finding the correct block group, but you may assume it's in the
// first one.
struct ext2_inode* get_inode(void *fs, uint32_t inode_num) {
	struct ext2_group_desc* bg;
	uint32_t inode_table_num;
	void* inode_table;

	//get first (only) block group pointer
	bg = get_block_group(fs,1);

	//get index into inode table
	inode_table_num = bg->bg_inode_table;

	//gets pointer to block where inodes are located 
	inode_table = get_block(fs, inode_table_num);

	//Adds offset of inode within table to the beginning of the inode table pointer 
	return (struct ext2_inode*)((void*)inode_table + (inode_num -1)*sizeof(struct ext2_inode));
}

// Chunk a filename into pieces.
// split_path("/a/b/c") will return {"a", "b", "c"}.
char** split_path(char *path, int *num_parts) {
	int num_slashes = 0;
	for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
		num_slashes++;
	}
	*num_parts = num_slashes;
	// Copy out each piece by advancing two pointers (piece_start and slash).
	char **parts = (char **)malloc(num_slashes*sizeof(char *));
	char *piece_start = path + 1;
	int i = 0;
	for (char *slash = strchr(path + 1, '/'); slash != NULL; slash = strchr(slash + 1, '/')) {
		int part_len = slash - piece_start;
		parts[i] = (char *) malloc((part_len + 1)*sizeof(char));
		strncpy(parts[i], piece_start, part_len);
		piece_start = slash + 1;
		i++;
	}
	// Get the last piece.
	parts[i] = (char *)malloc((strlen(piece_start) + 1)*sizeof(char));
	strcpy(parts[i], " ");
	//strncpy(parts[i], piece_start, strlen(piece_start));
	strcpy(parts[i], piece_start);
	return parts;
}

// Convenience function to get the inode of the root directory.
struct ext2_inode* get_root_dir(void * fs) {
	return get_inode(fs, EXT2_ROOT_INO);
}

// Given the inode for a directory and a filename, return the inode number of
// that file inside that directory, or 0 if it doesn't exist there.
// 
// name should be a single component: "foo.txt", not "/files/foo.txt".
uint32_t get_inode_from_dir(void *fs, struct ext2_inode *dir, char *name) {
	char *tempname;

	//get pointer to block where directory entries reside using inode of directory
	struct ext2_dir_entry_2* dentry = (struct ext2_dir_entry_2*)get_block(fs,dir->i_block[0]);

	//scan only valid files in dentry
	//while(dentry->inode){
	ssize_t blocksize = get_block_size(fs) - dentry->rec_len;
	int count = 0;
	//while(blocksize > 0) {
	while(++count < 20) {
		int i;
		tempname = (char*)malloc(dentry->name_len*sizeof(char));
		//copy file name to temp name, then null terminate it
		for(i = 0; i<dentry->name_len;i++){
			tempname[i] = dentry->name[i];
		}
		tempname[i] = '\0';

		//if there is a match, return the inode number
		DEBUG("INODE DIR: name %s, tempname %s", name, tempname);
		DEBUG("INODE DIR: blocksize %d, reclen %d", blocksize, dentry->rec_len);
		if(strcmp(name,tempname) ==0){
			free((void*)tempname);
			return dentry->inode;
		}
		//else, go to next dentry entry 
		dentry = (struct ext2_dir_entry_2*)((void*)dentry+dentry->rec_len);
		blocksize -= dentry->rec_len;

		free((void*)tempname);
	}
	return 0;
}


// Find the inode number for a file by its full path.
// This is the functionality that ext2cat ultimately needs.
uint32_t get_inode_by_path(void *fs, char *path) {
	int num_parts = 0;
	if (!strcmp(path, "/")) {
		return EXT2_ROOT_INO;
	}
	char **parts = split_path(path, &num_parts);
	char *cur_part = *parts;
	int i;
	struct ext2_inode* cur_inode = get_root_dir(fs);
	uint32_t new_inode_num = 0;

	DEBUG("GET INODE: %s has %d parts", path, num_parts);

	for(i = 1; i <= num_parts; i++){
		//treat current inode as directory, and search for inode of next part
		new_inode_num = get_inode_from_dir(fs, cur_inode, cur_part);
		if (!new_inode_num) {
			ERROR("GET INODE: inode == 0");
			return 0;
		}
		DEBUG("GET INODE found: %d, %s", new_inode_num, cur_part);
		cur_inode = get_inode(fs, new_inode_num);
		cur_part = *(parts+i);
	}

	//final inode is the requested file. return its number
	DEBUG("GET INODE returning: %d", new_inode_num);
	return new_inode_num;
}

//Returns the inode number of the first free inode
uint32_t alloc_inode(void *fs) {
	struct ext2_group_desc *bg = get_block_group(fs, 1);	//get block group descriptor
	uint8_t *bitmap_byte = get_block(fs, bg->bg_inode_bitmap);	//get inode bitmap first byte
	int bytes_checked = 0;
	int bit;
	uint8_t cur_byte;

	while(bytes_checked < 1024) {		//cycle through bitmap
		cur_byte = *bitmap_byte;
		for(bit=0; bit < 8; bit++) {
			if(!(cur_byte & 0x01)) {
				*bitmap_byte = *bitmap_byte | (0x01 << bit); //set the inode to taken
				return (uint32_t)(bytes_checked*8 + bit + 1); //first inode number is 1, not 0
			}
			cur_byte = cur_byte >> 1;
		}
		bitmap_byte++;
		bytes_checked++;
	}
	return 0;
}

int free_inode(void *fs, uint32_t inum) {
	struct ext2_group_desc *bg = get_block_group(fs, 1);	//get block group descriptor
	uint8_t *bitmap_byte = get_block(fs, bg->bg_inode_bitmap);	//get inode bitmap first byte
	bitmap_byte += (inum / 8);
	int bit = (inum % 8);
	if (!bit) {
		bit = 8;
	}
	uint8_t mask = !(1 << (bit - 1));
	*bitmap_byte &= mask;
	return 1;
}




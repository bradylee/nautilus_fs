// ext2 definitions from the real driver in the Linux kernel.
#include "ext2/ext2fs.h"

// This header allows your project to link against the reference library. If you
// complete the entire project, you should be able to remove this directive and
// still compile your code.
//#include "reference_implementation.h"

// Definitions for ext2cat to compile against.
#include "ext2/ext2_access.h"



///////////////////////////////////////////////////////////
//  Accessors for the basic components of ext2.
///////////////////////////////////////////////////////////

// Return a pointer to the primary superblock of a filesystem.
struct ext2_super_block * get_super_block(void * fs) {
    
    //Super block is always a constant offset away from fs
    return (struct ext2_super_block*)(fs+SUPERBLOCK_OFFSET);
    
}


// Return the block size for a filesystem.
__u32 get_block_size(void * fs) {

    __u32 shift;
    struct ext2_super_block* sb;

    //get super block
    sb = get_super_block(fs);

    //s_log_block_size tells how much to shift 1K by to determine block size
    shift = sb->s_log_block_size;

    return (1024 << shift);
}


// Return a pointer to a block given its number.
// get_block(fs, 0) == fs;
void * get_block(void * fs, __u32 block_num) {

    __u32 block_size;
    block_size = get_block_size(fs);

    //gets block size, adds block offset to beginning of fs
    return (void*)(block_size*block_num + fs);

}


// Return a pointer to the first block group descriptor in a filesystem. Real
// ext2 filesystems will have several of these, but, for simplicity, we will
// assume there is only one.
struct ext2_group_desc * get_block_group(void * fs, __u32 block_group_num) {
 
    struct ext2_super_block* sb;

    //get superblock
    sb = get_super_block(fs);

    //returns location directly after superblock, which is where first block group descriptor resides 
    return (struct ext2_group_desc *)((void*)sb+SUPERBLOCK_SIZE);

}


// Return a pointer to an inode given its number. In a real filesystem, this
// would require finding the correct block group, but you may assume it's in the
// first one.
struct ext2_inode * get_inode(void * fs, __u32 inode_num) {
    

    struct ext2_group_desc* bg;
    __u32 inode_table_num;
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



///////////////////////////////////////////////////////////
//  High-level code for accessing filesystem components by path.
///////////////////////////////////////////////////////////

// Chunk a filename into pieces.
// split_path("/a/b/c") will return {"a", "b", "c"}.
//
// This one's a freebie.
char ** split_path(char * path) {
    int num_slashes = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
        num_slashes++;
    }

    // Copy out each piece by advancing two pointers (piece_start and slash).
    char ** parts = (char **) malloc(num_slashes*sizeof(char *));
    char * piece_start = path + 1;
    int i = 0;
    for (char * slash = strchr(path + 1, '/');
         slash != NULL;
         slash = strchr(slash + 1, '/')) {
        int part_len = slash - piece_start;
        parts[i] = (char *) malloc((part_len + 1)*sizeof(char));
        strncpy(parts[i], piece_start, part_len);
        piece_start = slash + 1;
        i++;
    }
    // Get the last piece.
    parts[i] = (char *) malloc((strlen(piece_start) + 1)*sizeof(char));
    strncpy(parts[i], piece_start, strlen(piece_start));
    return parts;
}


// Convenience function to get the inode of the root directory.
struct ext2_inode * get_root_dir(void * fs) {
    return get_inode(fs, EXT2_ROOT_INO);
}


// Given the inode for a directory and a filename, return the inode number of
// that file inside that directory, or 0 if it doesn't exist there.
// 
// name should be a single component: "foo.txt", not "/files/foo.txt".
__u32 get_inode_from_dir(void * fs, struct ext2_inode * dir, 
        char * name) {

    char* tempname;
    struct ext2_dir_entry_2* directory;

    //get pointer to block where directory entries reside using inode of directory
    directory = (struct ext2_dir_entry_2*)get_block(fs,dir->i_block[0]);

   //scan only valid files in directory
    while(directory->inode != 0){

        //create temporary name
        tempname = (char*)malloc(directory->name_len*sizeof(char));
	int i;
        //copy file name to temp name, then null terminate it
	for(i = 0; i<directory->name_len;i++){
		tempname[i] = directory->name[i];
	}
	tempname[i] = '\0';

	//if there is a match, return the inode number
	if(strcmp(name,tempname) ==0){
		free((void*)tempname);
		return directory->inode;
		}
        //else, go to next directory entry 
        directory = (struct ext2_dir_entry_2*)((void*)directory+directory->rec_len);

        free((void*)tempname);
    }
    return 0;
}


// Find the inode number for a file by its full path.
// This is the functionality that ext2cat ultimately needs.
__u32 get_inode_by_path(void * fs, char * path) {
    char** parts = split_path(path);
    char* cur_part = *parts;
    int i;
    struct ext2_inode* cur_inode = get_root_dir(fs);
    __u32 new_inode_num;

    //use number of slashes to get number of parts in path as in split_path()
    int num_parts = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
        num_parts++;
    }
    
    //for each part
    for(i = 1; i <= num_parts; i++){
	//treat current inode as directory, and search for inode of next part
	new_inode_num = get_inode_from_dir(fs,cur_inode,cur_part);
	cur_inode = get_inode(fs, new_inode_num);
	cur_part = *(parts+i);
    }
    
    //final inode is the requested file. return its number
    return new_inode_num;
}


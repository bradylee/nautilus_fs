#include <fs/fs.h>

void test_fs() {
	init_fs();
	printk("Opening files...\n");
	uint32_t fn = file_open("/readme", 0);
	fn = file_open("/null", 0);
	fn = file_open("/nothing", 0);
	fn = file_open("/nothing", 0);
	printk("Closing files...\n");
	deinit_fs();
	printk("Done\n");
}

void init_fs(void) {
	printk("Initing list...\n");
	INIT_LIST_HEAD(&open_files);
}

void deinit_fs(void) {
	iterate_opened(__file_close);
}

void iterate_opened(void (*callback)(struct file_data*)) {
	struct list_head *cur;
	struct list_head *temp;
	struct file_data *fd;

	list_for_each_safe(cur, temp, &open_files) {
		fd = (struct file_data*)cur;
		callback(fd);
	}
}

void __file_print(struct file_data* fd) {
	printk("%d\n", fd->filenum);
}

size_t file_open(char *path, int access) {
	uint32_t inode_num = ext2_open(&RAMFS_START, path, access);
	struct file_data *fdp = malloc(sizeof(struct file_data));

	fdp->filenum = inode_num;
	// check already opened
	if (!get_open_file(fdp->filenum)) { 
		printk("Opened %s %d\n", path, fdp->filenum);
		list_add(&fdp->file_node, &open_files);
	}
	return fdp->filenum;
}

void __file_close(struct file_data* fd) {
	list_del((struct list_head*)fd);
	//free(fd);
}

int file_close(uint32_t filenum) {
	struct file_data *fd = get_open_file(filenum);
	if (!fd)
		return 0;
	__file_close(fd);
	return 1;
}

struct file_data* get_open_file(uint32_t filenum) {
	struct list_head *cur;
	struct file_data *fd;

	list_for_each(cur, &open_files) {
		fd = (struct file_data*)cur;
		if (fd->filenum == filenum)
			return fd;
	}
	return NULL;
}

size_t file_read(int filenum, char * dst, size_t num_bytes) {
	struct file_data *target = get_open_file(filenum);
	uint32_t inode_num = target->filenum;
	size_t n = ext2_read(inode_num, dst, num_bytes,target->position);
	target->position += n;
	printk("n: %d pos: %d\n", n, target->position);
	return n;
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

size_t file_write(int filenum,char * write_data, size_t num_bytes) {
	struct file_data *target = get_open_file(filenum);
	uint32_t inode_num = target->filenum;
	size_t n = ext2_write(inode_num, write_data, num_bytes,target->position);
	target->position += n;
	//printk("n: %d pos: %d\n", n, target->position);
	return n;
}

//pos = 0 -> beginning of file, 1 -> current position, 2 -> end of file
size_t file_seek(int filenum, size_t offset, int pos) {
	struct file_data *target = get_open_file(filenum);
	if(pos == 0) {
		target -> position = offset;
		return target->position;
	}
	else if(pos == 1) {
		target -> position += offset;
		return target->position;
	}
	else if(pos == 2) {
		uint64_t size = get_ext2_file_size((uint32_t)target->filenum);
		printk("file size = %d\n",size);
		target -> position = size + offset-1;
		return target->position;		
	}
	else {
		return -1;
	}
}

size_t file_append(int filenum,char * write_data, size_t num_bytes) {
	struct file_data *target = get_open_file(filenum);
	uint32_t inode_num = target->filenum;
	size_t n = file_seek(filenum,0,2);
	n = ext2_write(inode_num, write_data, num_bytes,target->position);
	target->position += n;
	printk("n: %d pos: %d\n", n, target->position);
	return n;
}

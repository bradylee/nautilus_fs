#include <fs/fs.h>

void test_fs() {

	init_fs();
	printk("Opening files...\n");
	uint32_t fn;
	fn = file_open("/readme", 1);
	fn = file_open("/null", 1);
	fn = file_open("/nothing", 1);
	fn = file_open("/nothing", 1);
	__iterate_opened(__file_print);
	printk("Closing files...\n");
	deinit_fs();
	printk("Done\n");

  printk("-----------------------------------\n");
  file_create("/a");
  fn = file_open("/a",O_RDWR);
  file_write(fn, "Testing file /a", 15);
  char * buf = malloc(15);
  file_seek(fn,0,0);
  file_read(fn,buf,15);
  printk("buffer: %s size: %d\n",buf, ext2_get_file_size(get_inode_by_path(&RAMFS_START,"/a")));
  
  
  
}

void init_fs(void) {
	printk("Initing list...\n");
	INIT_LIST_HEAD(&open_files);
}

void deinit_fs(void) {
	__iterate_opened(__file_close);
}

void __iterate_opened(void (*callback)(struct file_data*)) {
	struct list_head *cur;
	struct list_head *temp;
	struct file_data *fd;

	list_for_each_safe(cur, temp, &open_files) {
		fd = (struct file_data*)cur;
		callback(fd);
	}
}

void __file_print(struct file_data* fd) {
	printk("%d %d\n", fd->filenum, fd->fileid);
}

size_t file_open(char *path, int access) {
	static uint32_t n = 1;
	struct file_data *fdp = malloc(sizeof(struct file_data));
	//uint32_t inode_num = ext2_open(&RAMFS_START, path, access);
	
	// if filetype ext2 then...
	fdp->open = ext2_open;
	fdp->read = ext2_read;
	fdp->write = ext2_write;
  fdp->position = 0;

	fdp->filenum = n++; // allows file to be opened multiple times
	fdp->fileid = fdp->open(&RAMFS_START, path, access);
	fdp->access = access;

	// check already opened
	if (!__get_open_file(fdp->filenum)) { 
		list_add(&fdp->file_node, &open_files);
		printk("Opened %s %d %d\n", path, fdp->filenum, fdp->fileid);
	}

	// check if opening a file that doesn't exist
	if (!file_exist(path) && __file_has_access(fdp, O_WRONLY) && __file_has_access(fdp, O_CREAT)) {
		
	}

	if (file_exist(path)) {

  }

	return fdp->filenum;
}

void __file_close(struct file_data* fd) {
	list_del((struct list_head*)fd);
	free(fd);
}

int file_close(uint32_t filenum) {
	struct file_data *fd = __get_open_file(filenum);
	if (!fd)
		return 0;
	__file_close(fd);
	return 1;
}

struct file_data* __get_open_file(uint32_t filenum) {
	struct list_head *cur;
	struct file_data *fd;

	list_for_each(cur, &open_files) {
		fd = (struct file_data*)cur;
		if (fd->filenum == filenum)
			return fd;
	}
	return NULL;
}

size_t file_read(int filenum, char *buf, size_t num_bytes) {
	struct file_data *target = __get_open_file(filenum);

	//  RDONLY or RDWR will return the same
	if (target == NULL || !__file_has_access(target, O_RDONLY)) {
		return -1;
	}

	size_t n = target->read(target->fileid, buf, num_bytes, target->position);
	target->position += n;
	//printk("n: %d pos: %d\n", n, target->position);
	return n;
}

size_t file_write(int filenum, char *buf, size_t num_bytes) {
	struct file_data *target = __get_open_file(filenum);

	//  WRONLY or RDWR will return the same
	if (target == NULL || !__file_has_access(target, O_WRONLY)) {
		return -1;
	}
	if (__file_has_access(target, O_APPEND)) {
		__file_seek(target, 0, 2);
	}

	size_t n = target->write(target->fileid, buf, num_bytes, target->position);
	target->position += n;
	//printk("n: %d pos: %d\n", n, target->position);
	return n;
}

int __file_has_access(struct file_data *fd, int access) {
	return ((fd->access & access) == access);
}

size_t __file_seek(struct file_data *target, size_t offset, int pos) {
	if(pos == 0) {
		target->position = offset;
	}
	else if(pos == 1) {
		target->position += offset;
	}
	else if(pos == 2) {
		uint64_t size = ext2_get_file_size((uint32_t)target->filenum);
		//printk("file size = %d\n",size);
		target->position = size + offset-1;
	}
	else {
		return -1;
	}
	return target->position;
}

//pos = 0 -> beginning of file, 1 -> current position, 2 -> end of file
size_t file_seek(int filenum, size_t offset, int pos) {
	struct file_data *target = __get_open_file(filenum);
	if (target == NULL) {
		return -1;
	}
	return __file_seek(target, offset, pos);
}

// returns 1 if file exists, 0 other
uint32_t file_exist(char *path) {
  return ext2_file_exist(&RAMFS_START,path);
}

// creates file with the given path
uint32_t file_create(char* path) {
  return ext2_file_create(&RAMFS_START, path);
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


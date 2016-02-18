#ifndef __TEST_FS_H
#define __TEST_FS_H

void test_fs(void);

struct file_data {
	int status= 0; //closed = -1, opened = thread_id
	int index = 0;
}

#endif

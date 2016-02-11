.align 4096
.global RAMFS_START 
RAMFS_START:
.incbin "ramfs.img"
.global RAMFS_END 
RAMFS_END:

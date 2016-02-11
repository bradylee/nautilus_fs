.align 4096
.global ramfs_start
ramfs_start:
.incbin "ramfs.img"
.global ramfs_end 
ramfs_end:

#include "util.h"

int mount(char *filesys, char *mountpt)
{
    int fd, ino;
    char buf[BLKSIZE];
    MINODE *mip;
    MOUNT *mnptr;
    SUPER *sup;

    // if no parameters
    if (strcmp(filesys, "") == 0)
    {
        // display current mounted filesystems
        printf("Current mounted filesystems: \n");
        for (int i = 0; i < 8; i++)
        {
            if (mountTable[i].dev)
                printf("%s\t%s\n", mountTable[i].name, mountTable[i].mount_name);
        }
        return 0;
    }

    // check whether filesys is already mounted
    for (int i = 0; i < 8; i++)
    {
        // if already mounted, reject
        if (mountTable[i].dev != 0 && strcmp(mountTable[i].name, filesys) == 0)
        {
            printf("Filesys is already mounted\n");
            return 0;
        }
    }

    // allocate a free MOUNT table entry (dev = 0 means FREE)
    for (int i = 0; i < 8; i++)
    {
        // free
        if (mountTable[i].dev == 0)
        {
            mnptr = &(mountTable[i]);
        }
    }

    // LINUX open FS for RW; use its fd number as the new DEV
    printf("checking EXT2 FS %s", filesys);
    fd = open(filesys, O_RDWR);
    if (fd < 0 || fd > 9)
    {
        printf("ERROR: INVALID FILE DESCRIPTOR\n");
        return -1;
    }
    dev = fd;
    /********** read super block  ****************/
    get_block(fd, 1, buf);
    sup = (SUPER *)buf;
    // check whether it's an EXT2 file system: if not, reject
    if (sup->s_magic != 0xEF53)
    {
        printf("ERROR: FILESYSTEM IS NOT OF TYPE EXT2\n");
        return -1;
    }

    ino = getino(mountpt); // get ino
    mip = iget(dev, ino);   // get minode in memory
    printf("Mount point type: %x\n", mip->INODE.i_mode);
    // check mount_point is a DIR
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("ERROR: MOUNT POINT %s IS NOT A DIR\n", mountpt);
        return 0;
    }
    // check mount_point is NOT busy (can't be someone's CWD)
    if (running->cwd->dev == mip->dev)
    {
        printf("ERROR: DIRECTORY IS BUSY\n");
        return 0;
    }

    // allocate a FREE (dev = 0) mountTable[] for newdev
    for (int i = 0; i < 8; i++)
    {
        // free
        if (mountTable[i].dev == 0)
        {
            mnptr = &(mountTable[i]);
        }
    }
    // record new DEV, ninodes, nblocks, bmap, imap, iblk in mountTable[]
    mnptr->dev = dev;
    strcpy(mnptr->name, filesys);
    strcpy(mnptr->mount_name, mountpt);
    mnptr->ninodes = sup->s_inodes_count;
    mnptr->mounted_inode = iget(dev, 2);
    mnptr->nblocks = sup->s_blocks_count;
    return 0;
}

int umount(char* filesys)
{
    MOUNT *umptr;
    // search the mount table to check filesys is indeed mounted
    if (strcmp(filesys, "") == 0)
    {
        printf("ERROR: NO FILESYSTEM TO UNMOUNT\n");
        return -1;
    }

    int count = 0;
    for (int i = 0; i < 8; i++)
    {
        if (strcmp(mountTable[i].name, filesys) == 0)
        {
            count++;
            break;
        }
    }

    if (count == 0)
    {
        printf("ERROR: FILESYSTEM IS NOT MOUNTED\n");
        return -1;
    }

    // check whether any file is still active in the mounted filesys
    // checking all minode[].dev
    for (int i = 0; i < NMINODE; i++)
    {
        // find mount_point's inode
        if (minode[i].refCount && minode[i].mounted && (minode[i].dev == umptr->dev))
        {
            close(umptr->dev);
            // reset it to "not mounted"
            minode[i].mptr = 0;
            // then iput() the minode because it was iget() during mounting
            iput(&minode[i]);
            umptr->mounted_inode = 0;
            umptr->dev = 0;
            break;
        }
    }
    return 0;
}
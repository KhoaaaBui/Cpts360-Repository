// #include "util.h"
// #include "mkdir_creat.c"
#include "rmdir.c"

int unlink(char *filename)
{
    int ino, pino;
    MINODE *mip, *pmip;
    char *parent, *child;
    
    //must be owner of the unlinked file
    if(!isowner(filename))
    {
    	printf("not owner!\n");
    	return -1;
    }
    
    ino = getino(filename);
    mip = iget(dev, ino);

    parent = dirname(filename);
    child = basename(filename);
    pino = getino(parent);
    pmip = iget(dev, pino);
    rm_child(pmip, child);
    pmip->dirty = 1;
    iput(pmip);
    mip->INODE.i_links_count--;
    if (mip->INODE.i_links_count > 0)
        mip->dirty = 1;
    else
    {
        bdalloc(dev, mip->INODE.i_block[0]);
        idalloc(dev, ino);
    }
    iput(mip);
}

int link(char *oldpath, char *new_path)
{

    int oino, nino, pino;
    MINODE *omip, *nmip, *pmip;
    char *parent, *child;

    if (oldpath == NULL ||
        new_path == NULL)
    {
        return -1;
    }

    if (strlen(oldpath) == 0 || strlen(new_path) == 0)
    {
        return -1;
    }
    printf("path:%s\n", oldpath);

    // verify old_file exists and is not a DIR;
    oino = getino(oldpath);
    omip = iget(dev, oino);
    printf("*******\n");
    printf("old file mode:%x\n", omip->INODE.i_mode);
    printf("*******\n");
    if (!S_ISREG(omip->INODE.i_mode))
    {
        // must not be a DIR
        return -1;
    }

    // new_file must not exist yet
    if (getino(new_path) != 0)
    {
        return -1;
    }

    // creat new_file with the same inode number of old_file
    parent = dirname(new_path);
    child = basename(new_path);
    pino = getino(parent);
    pmip = iget(dev, pino);

    // creat entry in new parent DIR with same inode number of old_file
    //omip->INODE.i_mode &= (~0xF000);
    //omip->INODE.i_mode |= 0xA000;
    enter_name(pmip, oino, child);

    omip->INODE.i_links_count++; // inc INODE’s links_count by 1
    omip->dirty = 1;             // for write back by iput(omip)
    iput(omip);
    iput(pmip);
}

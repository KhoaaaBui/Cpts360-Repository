#include "mkdir_creat.c"

int symlink(char *oldpath, char *new_path)
{
    int ino;
    MINODE *mip;
    if (0 >= (ino = getino(oldpath)))
    {
        printf("ERROR: FILE DOES NOT EXIST\n");
        return -1;
    }
    mycreat(new_path);
    if (0 >= (ino = getino(new_path)))
    {
        printf("ERROR: COULD NOT CREATE FILE\n");
        return -1;
    }
    mip = iget(dev, ino);
    mip->INODE.i_mode &= ~0770000;
    mip->INODE.i_mode |= 0120000;
    mip->dirty = 1;
    strcpy((char *)mip->INODE.i_block, oldpath);
    iput(mip);
}

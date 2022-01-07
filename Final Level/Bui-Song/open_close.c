#include "util.h"

int file_open(char *filename, int flags)
{
    int ino;
    MINODE *mip;
    OFT *oftp;

    ino = getino(filename);
    if (ino==0){ // if file does not exist
        mycreat(filename); // creat it first, then
        ino = getino(filename); // get its ino
    }
    mip = iget(dev, ino);

    oftp = (OFT*)malloc(sizeof(OFT));
    oftp->mode = flags;
    oftp->refCount = 1;
    oftp->mptr = mip;
    switch(flags)
    {
        case 0: oftp->offset = 0;
            break;
        case 1: truncate(mip);
            oftp->offset = 0;
            break;
        case 2: oftp->offset = 0;
            break;
        case 3: oftp->offset = mip->INODE.i_size;
            break;
        default: printf("ERROR: INVALID MODE\n");
            iput(mip);
            free(oftp);
            return -1;
            break;
    }

    int i = 0;
    while(running->fd[i] != NULL && i < 10) { i++; }
    if(i == 10)
    {
        printf("ERROR: NO ROOM TO OPEN FILE\n");
        iput(mip);
        free(oftp);
        return -1;
    }
    running->fd[i] = oftp;
    if(flags != 0) { mip->dirty = 1; }
    printf("file opened,fd=%d\n",i);
    return i;
}

int file_close(int fd)
{
    OFT *oftp;
    MINODE *mip;
    if (fd > 9 || fd < 0)
    {
        printf("ERROR: FILE DESCRIPTOR OUT OF RANGE\n");
        return -1;
    }
    if(running->fd[fd] == NULL)
    {
        printf("ERROR: FILE DESCRIPTOR NOT FOUND\n");
        return -1;
    }
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if(oftp->refCount > 0) {return 1;}
    mip = oftp->mptr;
    iput(mip);
    free(oftp);
    return 0;
}

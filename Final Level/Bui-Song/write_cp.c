#include "util.h"

int file_write(int fd, char *buf, int nbytes)
{
    if(fd < 0 || fd > 9)
    {
        printf("ERROR: INVALIDE FILE DESCRIPTOR\n");
        return -1;
    }
    if(running->fd[fd] == NULL)
    {
        printf("ERROR: FILE NOT OPEN\n");
        return -1;
    }
    if(running->fd[fd]->mode == 0)
    {
        printf("ERROR: FILE OPEN FOR READ ONLY\n");
        return -1;
    }

    int blk, newblk, lblk, lblk2, lblk3, startByte, remain, nBytesTot;
    int tempBuf1[256], tempBuf2[256];
    char writeBuf[BLKSIZE];
    char *cp, *cq;
    cq = buf;
    nBytesTot = nbytes;
    while (nbytes > 0)
    {
        lblk = running->fd[fd]->offset / BLKSIZE;
        startByte = running->fd[fd]->offset % BLKSIZE;
        if(lblk <12)
        {
            blk = running->fd[fd]->mptr->INODE.i_block[lblk];
            if(blk == 0)
            {
                blk = balloc(running->fd[fd]->mptr->dev);
                running->fd[fd]->mptr->INODE.i_block[lblk] = blk;
            }
        }
        else if (lblk >= 12 && lblk < 256 + 12)
        {
            if(running->fd[fd]->mptr->INODE.i_block[12] == 0)
            {
                newblk = balloc(running->fd[fd]->mptr->dev);
                running->fd[fd]->mptr->INODE.i_block[12] = newblk;
            }
            memset((char*)tempBuf1, 0, BLKSIZE);
            get_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[12], (char*)tempBuf1);
            blk = tempBuf1[lblk-12];
            if (blk == 0)
            {
                blk = balloc(running->fd[fd]->mptr->dev);
                tempBuf1[lblk-12] = blk;
                put_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[12], (char*)tempBuf1);
            }
        }
        else
        {
            if(running->fd[fd]->mptr->INODE.i_block[13] == 0)
            {
                newblk = balloc(running->fd[fd]->mptr->dev);
                running->fd[fd]->mptr->INODE.i_block[13] = newblk;
            }
            memset((char*)tempBuf1, 0, BLKSIZE);
            get_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[13], (char*)tempBuf1);
            lblk2 = (lblk - (256+12)) / 256;
            lblk3 = (lblk - (256+12)) % 256;
            if(tempBuf1[lblk2] == 0)
            {
                newblk = balloc(running->fd[fd]->mptr->dev);
                tempBuf1[lblk2] = newblk;
            }
            memset((char*)tempBuf2, 0, BLKSIZE);
            get_block(running->fd[fd]->mptr->dev, tempBuf1[lblk2], (char*)tempBuf2);
            blk = tempBuf2[lblk3];
            if(blk == 0)
            {
                blk = balloc(running->fd[fd]->mptr->dev);
                tempBuf2[lblk3] = blk;
                put_block(running->fd[fd]->mptr->dev, tempBuf1[lblk2], (char*)tempBuf2);
            }
            put_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[13], (char*)tempBuf1);
        }
        memset(writeBuf, 0, BLKSIZE);
        get_block(running->fd[fd]->mptr->dev, blk, writeBuf);
        cp = writeBuf + startByte;
        remain = BLKSIZE - startByte;
        if (remain < nbytes) //Copy remain
        {
            strncpy(cp, cq, remain);
            nbytes -= remain; running->fd[fd]->offset += remain;
            if(running->fd[fd]->offset > running->fd[fd]->mptr->INODE.i_size)
            {
                running->fd[fd]->mptr->INODE.i_size += remain;
            }
            remain -= remain;
        }
        else //Copy nbytes
        {
            strncpy(cp, cq, nbytes);
            remain -= nbytes; running->fd[fd]->offset += nbytes;
            if(running->fd[fd]->offset > running->fd[fd]->mptr->INODE.i_size)
            {
                running->fd[fd]->mptr->INODE.i_size += nbytes;
            }
            nbytes -= nbytes;
        }
        put_block(running->fd[fd]->mptr->dev, blk, writeBuf);
    }
    running->fd[fd]->mptr->dirty = 1;
    printf("******wrote %d chars into file fd = %d******\n", nBytesTot, fd);
    return nBytesTot;
}

int file_cp(char *oldfile,char *newfile)
{
    int fd, gd, n;
    char buf[BLKSIZE], src[256], dest[256], origDest[256];

    if(newfile == NULL || oldfile==NULL) {return -1;}
    fd = file_open(oldfile,0);
    if(fd < 0 || fd > 9){ return -1;}
    gd = file_open(newfile,1);
    if(gd < 0 || gd > 9)
    {
        mycreat(newfile);
        gd = file_open(newfile,1);
        if(gd < 0 || gd > 9) {file_close(fd); return -1;}
    }
    while(n = file_read(fd, buf, 1024))
    {
        file_write(gd, buf, n);
    }
    file_close(fd);
    file_close(gd);
    return 0;
}

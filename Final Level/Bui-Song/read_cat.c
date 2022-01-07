#include "util.h"

int cat(char *filename)
{
    char mybuf[1024], dummy = 0; // a null char at end of mybuf[]
    int n;
    if(filename == NULL){ return -1;}
    // open filename for READ
    int fd = file_open(filename, 0);
    if(fd < 0 || fd > 9){ return -1;}
    n = file_read(fd, mybuf, BLKSIZE);
    while(n != 0)
    {
        mybuf[n] = 0; // as a null terminated string
        printf("%s", mybuf);
        n = file_read(fd, mybuf, BLKSIZE);
    }
    file_close(fd);
}

int file_read(int fd, char *buf, int nbytes)
{
    if (fd < 0 || fd > 9)
    {
        printf("ERROR: INVALID FILE DESCRIPTOR\n");
        return -1;
    }
    if (running->fd[fd] == NULL)
    {
        printf("ERROR: FILE NOT OPEN\n");
        return -1;
    }
    if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)
    {
        printf("ERROR: NO READ ACCESS\n");
        return -1;
    }
    int avail, lblk, lblk2, lblk3, blk, startByte, remain, count = 0;
    char *cq, *cp, readBuf[BLKSIZE];
    int tempBuf1[256], tempBuf2[256];
    int size = running->fd[fd]->mptr->INODE.i_size;
    avail = size - running->fd[fd]->offset;
    cq = buf;
    while (nbytes && avail)
    {
        lblk = running->fd[fd]->offset / BLKSIZE;
        startByte = running->fd[fd]->offset % BLKSIZE;
        if(lblk <12)
        {
            blk = running->fd[fd]->mptr->INODE.i_block[lblk];
        }
        else if (lblk >= 12 && lblk < 256 + 12)
        {
            get_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[12], (char*)tempBuf1);
            blk = tempBuf1[lblk-12];
        }
        else
        {
            get_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[13], (char*)tempBuf1);
            lblk2 = (lblk - (256+12)) / 256;
            lblk3 = (lblk - (256+12)) % 256;
            get_block(running->fd[fd]->mptr->dev, tempBuf1[lblk2], (char*)tempBuf2);
            blk = tempBuf2[lblk3];
        }
        get_block(running->fd[fd]->mptr->dev, blk, readBuf);
        cp = readBuf + startByte;
        remain = BLKSIZE - startByte;
        if(avail >= BLKSIZE && remain == BLKSIZE && nbytes >= BLKSIZE) //Copy the entire block
        {
            strncpy(cq, cp, BLKSIZE);
            running->fd[fd]->offset += BLKSIZE;
            count += BLKSIZE; avail -= BLKSIZE; nbytes -= BLKSIZE; remain -= BLKSIZE;
        }
        else if (nbytes <= avail && nbytes <= remain) //Copy nbytes
        {
            strncpy(cq, cp, nbytes);
            running->fd[fd]->offset += nbytes;
            count += nbytes; avail -= nbytes; nbytes -= nbytes; remain -= nbytes;
        }
        else if (remain <= avail && remain <= nbytes) //Copy remain
        {
            strncpy(cq, cp, remain);
            running->fd[fd]->offset += remain;
            count += remain; avail -= remain; nbytes -= remain; remain -= remain;
        }
        else //Copy avail
        {
            strncpy(cq, cp, avail);
            running->fd[fd]->offset += avail;
            count += avail; avail -= avail; nbytes -= avail; remain -= avail;
        }
    }
    printf("******myread: read %d chars from file %d******\n", count, fd);
    return count;
}

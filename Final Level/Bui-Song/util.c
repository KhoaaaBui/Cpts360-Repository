/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "util.h"
// #include "type.h"

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk * BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk * BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}

int tokenize(char *pathname)
{
   int i;
   char *s;
   printf("tokenize %s\n", pathname);

   strcpy(gpath, pathname); // tokens are in global gpath[ ]
   n = 0;

   s = strtok(gpath, "/");
   while (s)
   {
      name[n] = s;
      n++;
      s = strtok(0, "/");
   }
   name[n] = 0;

   for (i = 0; i < n; i++)
      printf("%s  ", name[i]);
   printf("\n");
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
   int i;
   MINODE *mip;
   char buf[BLKSIZE];
   int blk, offset;
   INODE *ip;

   for (i = 0; i < NMINODE; i++)
   {
      mip = &minode[i];
      if (mip->refCount && mip->dev == dev && mip->ino == ino)
      {
         mip->refCount++;
         //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
         return mip;
      }
   }

   for (i = 0; i < NMINODE; i++)
   {
      mip = &minode[i];
      if (mip->refCount == 0)
      {
         //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
         mip->refCount = 1;
         mip->dev = dev;
         mip->ino = ino;

         // get INODE of ino to buf
         blk = (ino - 1) / 8 + iblk;
         offset = (ino - 1) % 8;

         //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

         get_block(dev, blk, buf);
         ip = (INODE *)buf + offset;
         // copy INODE to mp->INODE
         mip->INODE = *ip;
         return mip;
      }
   }
   printf("PANIC: no more free minodes\n");
   return 0;
}

void iput(MINODE *mip)
{
   int i, block, offset;
   char buf[BLKSIZE];
   INODE *ip;

   if (mip == 0)
      return;

   // decrement refCount by 1
   mip->refCount--;

   // still has user
   if (mip->refCount > 0)
      return;
   // no need to write back
   if (!mip->dirty)
      return;

   /* write INODE back to disk */
   /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WORK : MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 *****************************************************/
   block = (mip->ino - 1) / 8 + iblk;
   offset = (mip->ino - 1) % 8;

   // get block containing this inode
   get_block(mip->dev, block, buf);
   // ip points at INODE
   ip = (INODE *)buf + offset;
   // copy INODE to inode in block
   *ip = mip->INODE;
   // write back to disk
   put_block(mip->dev, block, buf);
   // release a used minode
   mip->refCount = 0;
}

int search(MINODE *mip, char *name)
{
   int i;
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE)
   {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%4d  %4d  %4d    %s\n",
             dp->inode, dp->rec_len, dp->name_len, dp->name);
      if (strcmp(temp, name) == 0)
      {
         printf("found %s : ino = %d\n", temp, dp->inode);
         return dp->inode;
      }
      cp += dp->rec_len;
      dp = (DIR *)cp;
   }
   return 0;
}

int getino(char *pathname)
{
   int i, ino, blk, offset;
   char buf[BLKSIZE];
   INODE *ip;
   MINODE *mip;

   printf("getino: pathname=%s\n", pathname);
   if (strcmp(pathname, "/") == 0)
      return 2;

   // starting mip = root OR CWD
   if (pathname[0] == '/')
      mip = root;
   else
      mip = running->cwd;

   mip->refCount++; // because we iput(mip) later

   tokenize(pathname);

   for (i = 0; i < n; i++)
   {
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

      ino = search(mip, name[i]);

      if (ino == 0)
      {
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);
      mip = iget(dev, ino);
   }

   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[])
{
   // WRITE YOUR code here
   // search parent's data block for myino; SAME as search() but by myino
   // copy its name STRING to myname[ ]
   int i;
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", myname, parent->dev, parent->ino);
   ip = &(parent->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE)
   {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%4d  %4d  %4d    %s\n",
             dp->inode, dp->rec_len, dp->name_len, dp->name);
      if (strcmp(temp, name) == 0)
      {
         printf("found %s : ino = %d\n", temp, dp->inode);
         return dp->inode;
      }
      cp += dp->rec_len;
      dp = (DIR *)cp;
   }
   return 0;
}

int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
   // mip points at a DIR minode
   // WRITE your code here: myino = ino of .  return ino of ..
   // all in i_block[0] of this DIR INODE.
   char buf[BLKSIZE];
   DIR *dp;
   char *cp;

   get_block(mip->dev, mip->INODE.i_block[0], buf);
   dp = (DIR *)buf;
   cp = buf;

   *myino = dp->inode;
   cp += dp->rec_len;
   dp = (DIR *)cp;
   return dp->inode;
}

// check if bit is 1 or 0
int tst_bit(char *buf, int bit)
{
   // position of the bit in buf
   int i = bit / 8;
   // position of the bit in the byte at buf[i]
   int j = bit % 8;
   if (buf[i] & (1 << j))
      return 1;
   return 0;
}

// set bit to 1
int set_bit(char *buf, int bit)
{
   int i = bit / 8;
   int j = bit % 8;
   return (buf[i] |= (1 << j));
}

// clear bit in char buf[BLKSIZE]
int clr_bit(char *buf, int bit)
{
   int i = bit / 8;
   int j = bit % 8;
   buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{
   char buf[BLKSIZE];

   // decrement free inodes count in SUPER block
   get_block(dev, 1, buf);
   sp = (SUPER *)buf;
   sp->s_free_inodes_count--;
   put_block(dev, 1, buf);

   // decrement free inodes count in Group Descriptor
   get_block(dev, 2, buf);
   gp = (GD *)buf;
   gp->bg_free_inodes_count--;
   put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
   char buf[BLKSIZE];

   // decrement free blocks count in SUPER block
   get_block(dev, 1, buf);
   sp = (SUPER *)buf;
   sp->s_free_blocks_count--;
   put_block(dev, 1, buf);

   // decrement free blocks count in Group Descriptor
   get_block(dev, 2, buf);
   sp = (GD *)buf;
   sp->s_free_blocks_count--;
   put_block(dev, 2, buf);
}

// allocate an inode number from inode_bitmap
int ialloc(int dev)
{
   int i;
   char buf[BLKSIZE];

   // read inode_bitmap block
   get_block(dev, imap, buf);

   for (i = 0; i < ninodes; i++)
   {
      // if the inode in imap is free
      if (tst_bit(buf, i) == 0)
      {
         // allocate free inode
         set_bit(buf, i);
         // decrement # of free inodes from SUPER and GD
         decFreeInodes(dev);
         put_block(dev, imap, buf);
         printf("allocated ino = %d\n", i + 1); // bits count from 0; ino from 1
         return i + 1;
      }
   }
   printf("no free inodes (ialloc)\n");
   return 0;
}

// allocate free disk block from disk
int balloc(int dev)
{
   int i;
   char buf[BLKSIZE];
   // read block_bitmap block
   get_block(dev, bmap, buf);

   for (i = 0; i < nblocks; i++)
   {
      // if block in bmap is free
      if (tst_bit(buf, i) == 0)
      {
         // allocate the free block
         set_bit(buf, i);
         // dec # of free blocks in SUPER and GD
         decFreeBlocks(dev);
         put_block(dev, bmap, buf);
         return i;
      }
   }
}

int incFreeInodes(int dev)
{
   char buf[BLKSIZE];
   // inc free inodes count in SUPER and GD
   get_block(dev, 1, buf);
   sp = (SUPER *)buf;
   sp->s_free_inodes_count++;
   put_block(dev, 1, buf);
   get_block(dev, 2, buf);
   gp = (GD *)buf;
   gp->bg_free_inodes_count++;
   put_block(dev, 2, buf);
}

int incFreeBnodes(int dev)
{
   char buf[BLKSIZE];
   // inc free inodes count in SUPER and GD
   get_block(dev, 1, buf);
   sp = (SUPER *)buf;
   sp->s_free_blocks_count++;
   put_block(dev, 1, buf);
   get_block(dev, 2, buf);
   gp = (GD *)buf;
   gp->bg_free_blocks_count++;
   put_block(dev, 2, buf);
}

int idalloc(int dev, int ino)
{
   int i;
   char buf[BLKSIZE];
   // get inode bitmap block
   get_block(dev, imap, buf);
   clr_bit(buf, ino - 1);
   // write buf back
   put_block(dev, imap, buf);
   incFreeInodes(dev);
}

int bdalloc(int dev, int bno)
{
   int i;
   char buf[BLKSIZE];

   // get block bitmap block
   get_block(dev, bmap, buf);
   clr_bit(buf, bno - 1);
   // write buf back
   put_block(dev, bmap, buf);
   incFreeBnodes(dev);
}

int haspermission(char *filename, char mode)
{
    int r=0;
    if(running->uid == 0)
    	return 1;
    int ino = getino(filename);
    MINODE *mip = iget(dev, ino);
    if(mip->INODE.i_uid == running->uid)
    {
        if(mode == 'r')
            r = tst_bit(mip->INODE.i_mode, 7);
        if(mode == 'w')
            r = tst_bit(mip->INODE.i_mode, 8);
        if(mode == 'x')
            r = tst_bit(mip->INODE.i_mode, 9);
    }
    iput(mip);
    return r;
}

int isowner(char *filename)
{
    int r=0;
    if(running->uid == 0)
    {
    	printf("super user\n");
    	return 1;
    }
    int ino = getino(filename);
    MINODE *mip = iget(dev, ino);
    printf("owner uid=%d, running uid=%d\n", mip->INODE.i_uid, running->uid);
    if(mip->INODE.i_uid == running->uid)
        r = 1;
    iput(mip);
    return r;
}

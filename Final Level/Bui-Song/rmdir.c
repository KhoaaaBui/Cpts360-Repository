#include "util.h"

int myrmdir(char *pathname)
{
    MINODE *mip;
    MINODE *pmip;
    char parent[64], child[32], temp[64], link[128];
    int ino, pino;
    char *cp;
    DIR *dp;
    char buf[BLKSIZE];

    // 1. Divide pathname into dirname and basename
    strcpy(temp, pathname);
    // get dirname
    strcpy(parent, dirname(temp));
    strcpy(temp, pathname);
    // get basename
    strcpy(child, basename(temp));
    
    //must be owner of removed dir
    if(!isowner(pathname))
    {
    	printf("not owner!\n");
    	return -1;
    }

    // 1. get in-memory INODE of pathname
    ino = getino(pathname);
    if (ino == 0)
    {
        printf("Invalid pathname\n");
        return -1;
    }
    mip = iget(dev, ino);

    // 2. verify INODE is a DIR
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("Invalid pathname, not a dir\n");
        return -1;
    }
    // verify DIR is empty (might or might not empty)
    if (mip->INODE.i_links_count == 2)
    {
        // verify if block is not empty
        if (mip->INODE.i_block[0] != 0)
        {
            get_block(dev, mip->INODE.i_block[0], buf);
            cp = buf;
            dp = (DIR *)buf;
            // traverse data blocks
            while (cp < buf + BLKSIZE)
            {
                strncpy(link, dp->name, dp->name_len);
                link[dp->name_len] = 0;

                // check if link is not . or ..
                if (strcmp(link, ".") != 0 && strcmp(link, "..") != 0)
                {
                    // if not, there is a file in dir
                    printf("Dir is not empty. Dir contains %s\n", link);
                    return -1;
                }
                cp += dp->rec_len;
                dp = (DIR *)cp;
            }
        }
    }

    // if dir is definitely not empty
    if (mip->INODE.i_links_count > 2)
    {
        printf("DIR is not empty\n");
        return -1;
    }

    // get parent ino and inode
    pino = findino(mip, &ino); // get pino from .. entry in INODE.i_block[0]
    pmip = iget(mip->dev, pino);

    // get name from parent DIR's data block
    // findmyname(pmip, ino, child);

    // remove name from parent directory
    rm_child(pmip, child);

    // dec parent links_count by 1; mark parent pimp dirty
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);

    // deallocate its data blocks and inode
    for (int i = 0; i < 12; i++)
    {
        if(mip->INODE.i_block[i] == 0)
            continue;
        bdalloc(dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip);
}

int rm_child(MINODE *pip, char *child)
{
   int i, size, found = 0;
   char *cp, *cp2;
   DIR *dp, *dp2, *dpPrev;
   char buf[BLKSIZE], buf2[BLKSIZE], temp[256];
   memset(buf2, 0, BLKSIZE);
   for (i = 0; i < 12; i++)
   {
      if (pip->INODE.i_block[i] == 0)
      {
         return 0;
      }
      get_block(pip->dev, pip->INODE.i_block[i], buf);
      dp = (DIR *)buf;
      dp2 = (DIR *)buf;
      dpPrev = (DIR *)buf;
      cp = buf;
      cp2 = buf;
      while (cp < buf + BLKSIZE && !found)
      {
         memset(temp, 0, 256);
         strncpy(temp, dp->name, dp->name_len);
         if (strcmp(child, temp) == 0)
         {
            //if child is only entry in block
            if (cp == buf && dp->rec_len == BLKSIZE)
            {
               bdalloc(pip->dev, pip->INODE.i_block[i]);
               pip->INODE.i_block[i] = 0;
               pip->INODE.i_blocks--;
               found = 1;
            }
            //else delete child and move entries over left
            else
            {
               while ((dp2->rec_len + cp2) < buf + BLKSIZE)
               {
                  dpPrev = dp2;
                  cp2 += dp2->rec_len;
                  dp2 = (DIR *)cp2;
               }
               if (dp2 == dp) //Child is last entry
               {
                  //printf("Child is last entry\n"); //FOR TESTING
                  dpPrev->rec_len += dp->rec_len;
                  found = 1;
               }
               else
               {
                  //printf("Child is not the last entry\n"); //FOR TESTING
                  size = ((buf + BLKSIZE) - (cp + dp->rec_len));
                  printf("Size to end = %d\n", size);
                  dp2->rec_len += dp->rec_len;
                  printf("dp2 len = %d\n", dp2->rec_len);
                  memmove(cp, (cp + dp->rec_len), size);
                  dpPrev = (DIR *)cp;
                  memset(temp, 0, 256);
                  strncpy(temp, dpPrev->name, dpPrev->name_len);
                  printf("new dp name = %s\n", temp);
                  found = 1;
               }
            }
         }
         cp += dp->rec_len;
         dp = (DIR *)cp;
      }
      if (found)
      {
         put_block(pip->dev, pip->INODE.i_block[i], buf);
         return 1;
      }
   }
   printf("ERROR: CHILD NOT FOUND\n");
   return -1;
}

#include "util.h"
/************* mkdir_creat_rmdir.c file **************/

int mymkdir(char *pathname)
{
    MINODE *mip;
    char parent[64], child[32], temp[64];

    // 1. Divide pathname into dirname and basename
    strcpy(temp, pathname);
    // get dirname
    strcpy(parent, dirname(temp));
    strcpy(temp, pathname);
    // get basename
    strcpy(child, basename(temp));
    
    // start with root
    if (parent[0] == '/')
    {
        mip = root;
        dev = root->dev;
    }
    // start with cwd
    else
    {
        mip = running->cwd;
        dev = running->cwd->dev;
    }

    // 2. dirname must exist and is a DIR
    int pino = getino(parent);
    // parent minode
    MINODE *pmip = iget(dev, pino);
    // check pmip is a DIR
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("pathname is not a dir\n");
        iput(pmip);
        return;
    }

    // 3. basename must not already exist in parent DIR
    // must return 0
    if (search(pmip, child) != 0)
    {
        printf("dir already exists in pathname\n");
        iput(pmip);
        return;
    }

    // 4. call kmkdir(pmip, basename) to create a DIR
    kmkdir(pmip, child);

    // 5. increment parent INODE's links_count by 1 and mark pmip dirty
    pmip->INODE.i_links_count++;
    pmip->dirty = 1;

    iput(pmip);
}

// mip points to parent inode
// name is the name of the child whose parent pointed by mip
int kmkdir(MINODE *mip, char *name)
{
    MINODE *pip;
    // 1. allocate an inode and a disk block
    int ino = ialloc(dev);
    int blk = balloc(dev);

    // 2. Create INODE in a minode and writes the INODE to disk
    pip = iget(dev, ino);
    ip = &pip->INODE;
    ip->i_mode = 0x41ED;      // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group id
    ip->i_size = BLKSIZE;     // size in bytes
    ip->i_links_count = 2;    // links count = 2 because of . and ..
    ip->i_atime = time(0L);
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 2;     // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk; // new DIR has one data block
    for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;
    }
    pip->dirty = 1; // mark minode dirty
    iput(pip);      // write INODE to disk

    // 3. Create data block for new DIR containing . and .. entries
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE); // clear buf[] to 0
    DIR *dp = (DIR *)buf;

    // make '.' entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    // make ".." entry
    dp = (char *)dp + 12;       // move to parent entry ..
    dp->inode = mip->ino;       // mip is parent pointer
    dp->rec_len = BLKSIZE - 12; // minus .'s size
    dp->name_len = 2;
    dp->name[0] = '.';
    dp->name[1] = '.';
    put_block(dev, blk, buf); // write to blk on disk

    // 4. enters (ino, name) as a dir_entry to the parent INODE
    enter_name(mip, ino, name);
}

void mycreat(char *pathname)
{
    MINODE *mip;
    char parent[64], child[32], temp[64];

    // 1. Divide pathname into dirname and basename
    strcpy(temp, pathname);
    // get dirname
    strcpy(parent, dirname(temp));
    strcpy(temp, pathname);
    // get basename
    strcpy(child, basename(temp));

    // start with root
    if (parent[0] == '/')
    {
        mip = root;
        dev = root->dev;
    }
    // start with cwd
    else
    {
        mip = running->cwd;
        dev = running->cwd->dev;
    }

    // 2. dirname must exist and is a DIR
    int pino = getino(parent);
    // parent minode
    MINODE *pmip = iget(dev, pino);
    // check pmip is a DIR
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("pathname is not a dir\n");
        iput(pmip);
        return;
    }

    // 3. basename must not already exist in parent DIR
    // must return 0
    if (search(pmip, child) != 0)
    {
        printf("dir already exists in pathname\n");
        iput(pmip);
        return;
    }

    // 4. call kmkdir(pmip, basename) to create a DIR
    kcreat(pmip, child);

    // do not increment parent INODE"s links_count
    // pmip->INODE.i_links_count++;
    pmip->dirty = 1;

    iput(pmip);
}

int kcreat(MINODE *mip, char *name)
{
    MINODE *pip;
    // 1. allocate an inode and a disk block
    int ino = ialloc(dev);
    // no data block is allocated
    // int blk = balloc(dev);

    // 2. Create INODE in a minode and writes the INODE to disk
    pip = iget(dev, ino);
    ip = &pip->INODE;
    ip->i_mode = 0x81A4;      // REG file type
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group id
    ip->i_size = BLKSIZE;     // size in bytes
    ip->i_links_count = 2;    // links count = 2 because of . and ..
    ip->i_atime = time(0L);   // set to current time
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    // no data block is allocated
    // ip->i_block[0] = blk; // new DIR has one data block
    for (int i = 0; i < 15; i++)
    {
        ip->i_block[i] = 0;
    }
    pip->dirty = 1; // mark minode dirty
    iput(pip);      // write INODE to disk

    // 4. enters (ino, name) as a dir_entry to the parent INODE
    enter_name(mip, ino, name);
}

int enter_name(MINODE *pip, int myino, char *myname)
{
    char buf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;
    int i = 0;

    //  For each data block of parent DIR do { // assume: only 12 direct blocks
    for (i = 0; i < 12; i++)
    {
        int ideal_len, need_len, remain;
        if (pip->INODE.i_block[i] == 0)
        {
            break;
        }

        //  get parent's data block into a buf[ ]
        get_block(pip->dev, pip->INODE.i_block[i], buf);

        dp = (DIR *)buf;
        cp = buf;

        // step to LAST entry in block:
        int blk = pip->INODE.i_block[i];
        printf("step to LAST entry in data block %d\n", blk);
        while (cp + dp->rec_len < buf + BLKSIZE)
        {

            /*************************************************
       print DIR record names while stepping through
     **************************************************/

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        // dp NOW points at last entry in block

        ideal_len = 4 * ((8 * dp->name_len + 3) / 4);
        need_len = 4 * ((8 * strlen(myname) + 3) / 4);
        remain = dp->rec_len - ideal_len;
        if (remain >= need_len)
        {
            dp->rec_len = ideal_len;
            cp += dp->rec_len;
            dp = (DIR *)cp;
            dp->inode = myino;
            dp->rec_len = remain;
            dp->name_len = strlen(myname);
            dp->file_type = 0;
            strcpy(dp->name, myname);
            goto enter_name_done;
        }
    }

    printf(" NO space in existing data block, allocate a new data block!\n");
    int bno = balloc(dev);
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    pip->INODE.i_block[i] = bno;
    pip->INODE.i_size += BLKSIZE;
    dp = (DIR *)buf;
    dp->inode = myino;
    dp->rec_len = BLKSIZE;
    dp->name_len = strlen(myname);
    dp->file_type = 0;
    strcpy(dp->name, myname);

enter_name_done:
    put_block(dev, pip->INODE.i_block[i], buf);

    return 0;
}

int truncate(MINODE *mip)
{
    int buf[256];
    int buf2[256];
    int bnumber, i, j;
    if(mip == NULL)
    {
        printf("ERROR: NO FILE\n");
        return -1;
    }
    // Deallocate all used blocks
    for(i = 0; i < 12; i++) //Check direct blocks
    {
        if(mip->INODE.i_block[i] != 0)
        {
            bdalloc(mip->dev, mip->INODE.i_block[i]);
        }
    }
    //Indirect blocks
    if(mip->INODE.i_block[12] == 0) {return 1;}
    get_block(dev, mip->INODE.i_block[12], (char*)buf);
    for(i = 0; i < 256; i++)
    {
        if(buf[i] != 0) {bdalloc(mip->dev, buf[i]);}
    }
    bdalloc(mip->dev, mip->INODE.i_block[12]);
    if(mip->INODE.i_block[13] == 0) {return 1;}
    //deallocate all double indirect blocks
    memset(buf, 0, 256);
    get_block(mip->dev, mip->INODE.i_block[13], (char*)buf);
    for(i = 0; i < 256; i++)
    {
        if(buf[i])
        {
            get_block(mip->dev, buf[i], (char*)buf2);
            for(j = 0; j < 256; j++)
            {
                if(buf2[j] != 0) {bdalloc(mip->dev, buf2[j]);}
            }
            bdalloc(mip->dev, buf[i]);
        }
    }
    bdalloc(mip->dev, mip->INODE.i_block[13]);
    mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    return 0;
}

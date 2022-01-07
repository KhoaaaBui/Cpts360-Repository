/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include "util.h"
//#include "cd_ls_pwd.c"
// #include "mkdir_creat.c"
// #include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close.c"
#include "read_cat.c"
#include "write_cp.c"
// #include "type.h"

extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int n;           // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128], new_path[128];
MOUNT mountTable[8];
#include "cd_ls_pwd.c"
#include "mount_umount.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC *p;

  printf("init()\n");

  // set all dev = 0 in init()
  for (i = 0; i < 8; i++)
  {
    mountTable[i].dev = 0;
  }

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i = 0; i < NPROC; i++)
  {
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = i;
    p->cwd = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{
  printf("mount_root()\n");
  root = iget(dev, 2);
}

int quit()
{
  int i;
  MINODE *mip;
  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
}

int pswitch()
{
    int pid = (running->pid + 1) % 2;
    quit();
    init();
    mount_root();
    printf("root refCount = %d\n", root->refCount);
    printf("Switch to P%d\n", pid);    
    running = &proc[pid];
    running->status = READY;
    running->cwd = iget(dev, 2);
    printf("root refCount = %d\n", root->refCount);
}
 

char *disk = "disk2";
int main(int argc, char *argv[])
{
  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0)
  {
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd; // global dev same as this fd

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53)
  {
    printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
    exit(1);
  }
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process

  while (1)
  {
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|cp|cat|mount|umount|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;

    if (line[0] == 0)
      continue;
    pathname[0] = 0;
    new_path[0] = 0;
    sscanf(line, "%s %s %s", cmd, pathname, new_path);
    printf("cmd=%s pathname=%s\n", cmd, pathname, new_path);

    if (strcmp(cmd, "ls") == 0)
      ls(pathname);
    else if (strcmp(cmd, "cd") == 0)
      cd(pathname);
    else if (strcmp(cmd, "pwd") == 0)
      pwd(running->cwd);
    else if (strcmp(cmd, "mkdir") == 0)
      mymkdir(pathname);
    else if (strcmp(cmd, "creat") == 0)
      mycreat(pathname);
    else if (strcmp(cmd, "rmdir") == 0)
      myrmdir(pathname);
    else if (strcmp(cmd, "link") == 0)
      link(pathname, new_path);
    else if (strcmp(cmd, "unlink") == 0)
      unlink(pathname);
    else if (strcmp(cmd, "symlink") == 0)
      symlink(pathname, new_path);
    else if (strcmp(cmd,"cp")==0)
        file_cp(pathname,new_path);
    else if (strcmp(cmd,"pswitch")==0)
        pswitch();    
    else if (strcmp(cmd, "cat") == 0)
        cat(pathname);
    else if (strcmp(cmd, "mount") == 0)
        mount(pathname, new_path);
    else if (strcmp(cmd, "unmount") == 0)
        umount(pathname);
    else if (strcmp(cmd, "quit") == 0)
    {
      quit();
      // exit(0);
    }
  }
}

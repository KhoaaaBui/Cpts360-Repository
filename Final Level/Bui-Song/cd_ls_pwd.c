#include "util.h"
/************* cd_ls_pwd.c file **************/
int cd(char *pathname)
{
  // printf("cd: under construction READ textbook!!!!\n");
  int ino = getino(pathname);
  if (ino == 0)
  {
    printf("Error: failed to traverse the path");
    return 0;
  }
  if(!haspermission(pathname, 'x'))
  {
      printf("No permission to access!\n");
      return 0;
  }

  MINODE *mip = iget(dev, ino);

  // check if mip->INODE is a DIR
  if (!S_ISDIR(mip->INODE.i_mode))
  {
    printf("%s is not a directory\n", pathname);
    return 0;
  }

  iput(running->cwd); // release old cwd
  running->cwd = mip; // change cwd to mip
  return 0;
  // READ Chapter 11.7.3 HOW TO chdir
}

int ls_file(MINODE *mip, char *name)
{
  // READ Chapter 11.7.3 HOW TO ls
  // print file mode
  if (S_ISDIR(mip->INODE.i_mode))
    printf("d");
  else if (S_ISREG(mip->INODE.i_mode))
  {
    printf("\n*******\n");
    printf("file mode:%x\n", mip->INODE.i_mode);
    printf("*******\n");
    printf("-");
  }
  else if (S_ISLNK(mip->INODE.i_mode))
    printf("l");
  else
    printf("-");

  // print permission
  for (int i = 8; i >= 0; i--)
  {
    char c;
    if (i % 3 == 2)
    {
      if (mip->INODE.i_mode & (1 << i))
        c = 'r';
      else
        c = '-';
    }
    else if (i % 3 == 1)
    {
      if (mip->INODE.i_mode & (1 << i))
        c = 'w';
      else
        c = '-';
    }
    else
    {
      if (mip->INODE.i_mode & (1 << i))
        c = 'x';
      else
        c = '-';
    }
    putchar(c);
  }

  // name/info
  printf(" %d %d %d %.4d ", mip->INODE.i_links_count, mip->INODE.i_uid, mip->INODE.i_gid, mip->INODE.i_size);

  char *time = ctime(&(mip->INODE.i_mtime));
  // add null to the end of time string
  time[strlen(time) - 1] = '\0';
  printf("%s ", time);

  // print pathname if symlinked file
  if (S_ISLNK(mip->INODE.i_mode))
    printf("%.20s -> %.20s\n", name, (char *)mip->INODE.i_block);
  else
    printf("%.20s\n", name);
}

int ls_dir(MINODE *mip)
{
  // printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");

  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  // go through blocks
  while (cp < buf + BLKSIZE)
  {
    // get directory name
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    // printf("%s  ", temp);
    mip = iget(dev, dp->inode);
    ls_file(mip, temp);
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  printf("\n");
  iput(mip);
  return 0;
}

int ls(char *pathname)
{
  MINODE *mip;
  int ino;
  // ls for cwd
  if (pathname[0] == 0)
  {
    ino = running->cwd->ino;
    mip = iget(dev, ino);
  }
  // ls with pathname
  else
  {
    // if path starts from root
    if (pathname[0] == '/')
      dev = root->dev;
    ino = getino(pathname);
    if (ino == 0)
    {
      printf("Error: Can not find pathname\n");
      return 0;
    }
    mip = iget(dev, ino);
    if (!S_ISDIR(mip->INODE.i_mode))
    {
      printf("Cannot traverse file that is not a directory\n");
      return 0;
    }
  }
  // printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");
  ls_dir(mip);
}

char* pwd(MINODE *wd)
{
  printf("do my PWD here!\n");
  if (wd == root)
  {
    printf("/\n");
    return;
  }
  else
  {
    rpwd(wd);
    printf("/\n");
  }
}

int rpwd(MINODE *wd)
{
  char buf[BLKSIZE], dirname[BLKSIZE];
  int my_ino, parent_ino;

  DIR *dp;
  char *cp;
  MINODE *pip;

  //(1). if (wd==root) return;
  if (wd == root)
  {
    return 0;
  }

  // (2). from wd->INODE.i_block[0], get my_ino and parent_ino
  get_block(wd->dev, wd->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  // Searches through cwd for cwd and parent ino
  while (cp < buf + BLKSIZE)
  {
    strcpy(dirname, dp->name);
    dirname[dp->name_len] = '\0';

    if (!strcmp(dirname, "."))
    {
      my_ino = dp->inode;
    }
    if (!strcmp(dirname, ".."))
    {
      parent_ino = dp->inode;
    }

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  //(3). pip = iget(dev, parent_ino);
  pip = iget(wd->dev, parent_ino);
  //(4). from pip->INODE.i_block[ ]: get my_name string by my_ino as LOCAL
  get_block(wd->dev, pip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE)
  {
    strncpy(dirname, dp->name, dp->name_len);
    dirname[dp->name_len] = '\0';

    if (dp->inode == my_ino)
    {
      break;
    }

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  //(5). rpwd(pip); // recursive call rpwd(pip) with parent minode
  rpwd(pip);
  iput(pip);

  // (6). print "/%s", my_name;
  printf("/%s", dirname);
  return 0;
}
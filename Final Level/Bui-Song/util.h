#ifndef UTIL_H
#define UTIL_H
#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf);
int put_block(int dev, int blk, char *buf);
int tokenize(char *pathname);
MINODE *iget(int dev, int ino);
void iput(MINODE *mip);
int getino(char *pathname);
int search(MINODE *mip, char *name);
int findmyname(MINODE *parent, u32 myino, char myname[]);
int findino(MINODE *mip, u32 *myino);
int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int decFreeInodes(int dev);
int decFreeBlocks(int dev);
int incFreeInodes(int dev);
int incFreeBnodes(int dev);
int ialloc(int dev);
int balloc(int dev);
int idalloc(int dev, int ino);
int bdalloc(int dev, int bno);
void mycreat(char *pathname);
int haspermission(char *filename, char mode);
int isowner(char *filename);
#endif

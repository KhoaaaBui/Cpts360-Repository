#include <stdio.h>
#include <fcntl.h>

#include <sys/types.h>
#include <unistd.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

struct partition {
   // SAME AS GIVEN ABOVE  
    u8 drive;             // drive number FD=0, HD=0x80, etc.

	u8  head;             // starting head 
	u8  sector;           // starting sector
	u8  cylinder;         // starting cylinder

	u8  sys_type;         // partition type: NTFS, LINUX, etc.

	u8  end_head;         // end head 
	u8  end_sector;       // end sector
	u8  end_cylinder;     // end cylinder

	u32 start_sector;     // starting sector counting from 0 
	u32 nr_sectors;       // number of of sectors in partition 
};

char *dev = "vdisk";
int fd;
    
// read a disk sector into char buf[512]
int read_sector(int fd, int sector, char *buf)
{
    lseek(fd, sector*512, SEEK_SET);  // lssek to byte sector*512
    read(fd, buf, 512);               // read 512 bytes into buf[ ]
}

int main()
{
    struct partition *p;
    char buf[512];

    fd = open(dev, O_RDONLY);   // open dev for READ
    read_sector(fd, 0, buf);    // read in MBR at sector 0    

    p = (struct partition *)(&buf[0x1be]); // p->P1
    printf("P1: start_sector = %d, nr_sectors = %d, sys_type = %d\n", p->start_sector, p->nr_sectors, p->sys_type);
    for(int i = 1; i <= 4; i++){
        // print P1's start_sector, nr_sectors, sys_type;
        printf("\tstart_sector\tend_sector\tnr_sectors\tsys_type\n");
        printf("P%d:\t ",i);
        printf("%d\t\t", p->start_sector);
        printf("%d\t\t", p->end_sector);
        printf("%d\t\t", p->nr_sectors);
        printf("%d\n", p->sys_type);
        p++;
    }
    p--;
    // Write code to print all 4 partitions;

    // ASSUME P4 is EXTEND type:
    int extStart = p->start_sector;
    //P4's start_sector
    //print extStart to see it
    printf("Ext Partition 4: start_sector=%d\n", extStart);

    int localMBR = extStart;
    int iEXT = 4;
    
    //loop:
    while(p->start_sector != 0){
        read_sector(fd, localMBR, buf);
        p = (struct partition *)(&buf[0x1be]);
        printf("Entry1 : start_sector=%d, nr_sectors=%d\n", p->start_sector, p->nr_sectors);
        // move to the next entry to get the location of the next partition
        p++;
        // print entry 2
        printf("Entry2 : start_sector=%d, nr_sectors=%d\n", p->start_sector, p->nr_sectors);
        
        p--; // point back to entry 1
        printf("\tstart_sector\tend_sector\tnr_sectors\n");
        printf("P%d:\t ",++iEXT);
        printf("%d\t\t", localMBR + p->start_sector);
        printf("%d\t\t", localMBR + p->start_sector + p->nr_sectors - 1);
        printf("%d\t\t\n", p->nr_sectors);
        
        p++; // point to entry 2
        if(p->start_sector != 0){
            printf("next localMBR sector = %d + %d = %d\n", extStart, p->start_sector, extStart + p->start_sector);
            localMBR = extStart + p->start_sector;
        }
    }
    printf("End of Extend partitions\n");
    
    
/*
    // partition table of localMBR in buf[ ] has 2 entries: 
    print entry 1's start_sector, nr_sector;
    compute and print P5's begin, end, nr_sectors

    if (entry 2's start_sector != 0){
        compute and print next localMBR sector;
        continue loop;
    }
    */
} 
#include <stdio.h>
#include <stdlib.h>

// 1-1
void prints(char* s){
    while(*s){
        putchar(*s);
        s++;
    }
}

// 2-2
typedef unsigned int u32;

char *tab = "0123456789ABCDEF";
int  BASE = 10; 

int rpu(u32 x)
{  
    char c;
    if (x){
       c = tab[x % BASE];
       rpu(x / BASE);
       putchar(c);
    }
}

int printu(u32 x)
{
   (x==0)? putchar('0') : rpu(x);
   putchar(' ');
}
// DEC
int printd(int x){
    if(x==0)
        putchar('0');
    else if(x<0){
        putchar('-');
        rpu(x*-1);
    }
    else
        rpu(x);
    putchar(' ');
}
// HEX
int printx(u32 x){
    BASE = 16;
    rpu(x);
}
// OCTO
int printo(u32 x){
    BASE = 8;
    rpu(x);
}

// 1-3

void myprintf(char* fmt, ...){
    char *cp = fmt;
    // ip = stack pointer, fmt is pointed by stack frame pointer
    // initialize ip to first item to be printed on stack ~ fmt
    int* ip = (int*)&fmt;
    ip++;
    while(*cp){
        if(*cp == '%'){
            cp++;
            switch(*cp){
                case 'c':
                    putchar(*ip);
                    break;
                case 's':
                    prints(*ip);
                    break;
                case 'u':
                    printu(*ip);
                    break;
                case 'd':
                    printd(*ip);
                    break;
                case 'o':
                    printo(*ip);
                    break;
                case 'x':
                    printx(*ip);
                    break;
            }
            ip++;
        }
        else if(*cp == '\n'){
            putchar('\n');
            putchar('\r');
        }
        else
            putchar(*cp);
        cp++;
    }

}

int main(int argc, char *argv[], char *env[]){
    myprintf("cha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n", 'A', "this is a test", 100, 100, 100, -100);
    myprintf("print argc and argv[]\n");
    myprintf("argc = %d\n", argc);
    for(int i = 0; i < argc; i++){
        myprintf("%s", argv[i]);
    }
}
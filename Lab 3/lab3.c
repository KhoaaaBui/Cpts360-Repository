/***** LAB3 base code *****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

char gpath[128]; // hold token strings
char *arg[64];   // token string pointers
int n;           // number of token strings

char dpath[128]; // hold dir strings in PATH
char *dir[64];   // dir string pointers
int ndir;        // number of dirs

int tokenize(char *pathname) // YOU have done this in LAB2
{                            // YOU better know how to apply it from now on
    char *s;
    strcpy(gpath, pathname); // copy into global gpath[]
    s = strtok(gpath, " ");
    n = 0;
    while (s)
    {
        arg[n++] = s; // token string pointers
        s = strtok(0, " ");
    }
    arg[n] = 0; // arg[n] = NULL pointer
}

int tokenDir(char *pathname)
{
    printf("3. Decompose PATH into dir strings:\n");
    char *s;
    strcpy(dpath, pathname); // copy into globall dir[]
    s = strtok(dpath, ":");
    ndir = 0;
    while (s)
    {
        dir[ndir++] = s; // dir string pointers
        s = strtok(0, ":");
    }
    dir[ndir] = 0; // dir[n] = NULL pointer
}

int main(int argc, char *argv[], char *env[])
{
    int i;
    int pid, status;
    char *cmd;
    char line[28];

    // The base code assume only ONE dir[0] -> "/bin"
    // YOU do the general case of many dirs from PATH !!!!
    // get PATH variable form env
    printf("************* Welcome to kbsh **************\n");
    char *home = getenv("HOME");
    printf("1. Show HOME directory: HOME = %s\n", home);

    char *path = getenv("PATH");
    printf("2. Show PATH:\nPATH=%s\n", path);

    //dir[0] = "/bin";
    //ndir = 1;
    tokenDir(path);

    // show dirs
    for (i = 0; i < ndir; i++)
        printf("dir[%d] = %s\n", i, dir[i]);

    while (1)
    {
        printf("sh %d running\n", getpid());
        printf("enter a command line : ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;
        if (line[0] == 0)
            continue;

        tokenize(line);

        for (i = 0; i < n; i++)
        { // show token strings
            printf("arg[%d] = %s\n", i, arg[i]);
        }
        // getchar();
        redirect();
        
        cmd = arg[0]; // line = arg0 arg1 arg2 ...

        if (strcmp(cmd, "cd") == 0)
        {
            chdir(arg[1]);
            continue;
        }
        if (strcmp(cmd, "exit") == 0)
            exit(0);

        pid = fork();

        if (pid)
        {
            printf("sh %d forked a child sh %d\n", getpid(), pid);
            printf("sh %d wait for child sh %d to terminate\n", getpid(), pid);
            pid = wait(&status);
            printf("ZOMBIE child=%d exitStatus=%x\n", pid, status);
            printf("main sh %d repeat loop\n", getpid());
        }
        else
        {
            printf("child sh %d running\n", getpid());

            // make a cmd line = dir[0]/cmd for execve()
            printf("child sh %d tries ls in each PATH dir\n");
            for (int i = 0; i < ndir; i++)
            {
                strcpy(line, dir[i]);
                strcat(line, "/");
                strcat(line, cmd);
                printf("line = %s\n", line);
                int r = execve(line, arg, env);

                printf("execve failed r = %d\n", r);
            }

            exit(1);
        }
    }
}

/********************* YOU DO ***********************
1. I/O redirections:

Example: line = arg0 arg1 ... > argn-1

  check each arg[i]:
  if arg[i] = ">" {
     arg[i] = 0; // null terminated arg[ ] array 
     // do output redirection to arg[i+1] as in Page 131 of BOOK
  }
  Then execve() to change image
*/
int redirect()
{
    for (int i = 0; i < n; i++)
    {   
        // look for output character
        if (strcmp(arg[i], ">") == 0)
        {
            printf("Redirecting output\n");
            ++i;
            int out = creat(arg[i], 0644);
            dup2(out, STDOUT_FILENO);
            close(out);
            arg[i-1] == 0;
            break;
            //[i] = 0; // null terminated arg[] array
        }
        // look for input character
        if (strcmp(arg[i], "<") == 0)
        {
            int fd = open("outfile.txt", O_RDONLY);
            close(0);                                
            dup(fd);
        }
    }
}

/*
2. Pipes:

Single pipe   : cmd1 | cmd2 :  Chapter 3.10.3, 3.11.2

Multiple pipes: Chapter 3.11.2
****************************************************/

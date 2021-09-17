#include <stdio.h>  // for I/O
#include <stdlib.h> // for I/O
#include <libgen.h> // for dirname()/basename()
#include <string.h>

typedef struct node
{
    char name[64]; // node's name string
    char type;     // 'D' for DIR; 'F' for file
    struct node *child, *sibling, *parent;
} NODE;

NODE *root, *cwd, *start;
char line[128];
char command[16], pathname[64];
char dname[64], bname[64];

//               0       1      2         3      4     5        6       7       8       9       10
char *cmd[] = {"mkdir", "ls", "quit", "rmdir", "cd", "pwd", "create", "rm", "reload", "save", "menu", 0};

int findCmd(char *command)
{
    int i = 0;
    while (cmd[i])
    {
        if (strcmp(command, cmd[i]) == 0)
            return i;
        i++;
    }
    return -1;
}

int dbname(char *pathname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}

NODE *search_child(NODE *parent, char *name)
{
    NODE *p;
    printf("search for %s in parent DIR %s\n", name, parent->name);
    p = parent->child;
    if (p == 0)
        return 0;
    while (p)
    {
        if (strcmp(p->name, name) == 0)
            return p;
        p = p->sibling;
    }
    return 0;
}

int insert_child(NODE *parent, NODE *q)
{
    NODE *p;
    printf("insert NODE %s into END of parent child list\n", q->name);
    p = parent->child;
    if (p == 0)
        parent->child = q;
    else
    {
        while (p->sibling)
            p = p->sibling;
        p->sibling = q;
    }
    q->parent = parent;
    q->child = 0;
    q->sibling = 0;
}

int tokenize(char *pathname)
{
    char *s;
    s = strtok(pathname, "/"); // first call to strtok()
    while (s)
    {
        printf("%s", s);
        s = strtok(0, "/"); // call strtok() until it returns NULL
    }
}

/***************************************************************
 This mkdir(char *name) makes a DIR in the current directory
 You MUST improve it to mkdir(char *pathname) for ANY pathname
****************************************************************/
int rmdir(char *pathname)
{
    set_start(pathname);
    // check if DIR has any child
    char *rmname = start->name;

    if (start->child)
    {
        printf("Can't rmdir if DIR is not empty\n");
        return -1;
    }
    // parent of dir
    start = start->parent;
    // child list of parent
    start = start->child;
    NODE *temp = start;
    // if first child is dir
    if (strcmp(rmname, temp->name) == 0)
    {
        // point parent first child to its next sibling
        start = start->parent;
        start->child = temp->sibling;
        free(temp);
        return 0;
    }
    // search for the node
    while(strcmp(temp->name, rmname) != 0)
    {
        start = temp;
        temp = temp->sibling;
    }
    start->sibling = start->sibling->sibling;
    free(temp);
}

int rm(char *pathname)
{
    dbname(pathname);
    char* rmname = bname;
    if (strcmp(bname, ".") != 0 && strcmp(dname, ".") != 0){
        // move start to the creat position
        set_start(dname);
    }
    start = cwd;
    // child list of parent
    start = start->child;
    NODE *temp = start;
    // if first child is dir
    if (strcmp(rmname, temp->name) == 0)
    {
        // point parent first child to its next sibling
        start = start->parent;
        start->child = temp->sibling;
        free(temp);
        return 0;
    }
    // search for the node
    while(strcmp(temp->name, rmname) != 0)
    {
        start = temp;
        temp = temp->sibling;
    }
    start->sibling = start->sibling->sibling;
    free(temp);
}

int pwd()
{
    // save the name of the current node
    char *curname[32];
    NODE *p = cwd;
    // go to the root node
    int i = 0;
    while (p != root)
    {
        curname[i] = p->name;
        p = p->parent;
        i++;
    }
    i--;
    // print out the pwd
    printf("pwd = ");
    // when cd to the root node
    if (i < 0)
    {
        printf("/");
    }
    else
    {
        for (i; i >= 0; i--)
        {
            printf("/%s", curname[i]);
        }
    }
    printf("\n");
}

int cd(char *pathname)
{
    // CWD point to parent directory
    if (strcmp(pathname, "..") == 0)
    {
        cwd = cwd->parent;
    }
    // CWD
    else if (strcmp(pathname, "/") == 0)
    {
        cwd = root;
    }
    else
    {
        set_start(pathname);
        cwd = start;
    }
}

int set_start(char *pathname)
{
    if (pathname[0] == '/')
        start = root;
    else
        start = cwd;

    char *s;
    s = strtok(pathname, "/"); // first call of strtok
    while (s)
    {
        if (strcmp(s, "..") == 0)
        {
            start = start->parent;
            s = strtok(0, "/"); // call strtok() until it returns NULL
        }
        else
        {
            // search for child s from parent start
            NODE *p = search_child(start, s);
            if (!p)
            {
                printf("%s does not exist in %s\n", s, start->name);
                return -1;
            }
            if (p->type == 'F')
            {
                printf("%s is not a DIR\n", p->name);
                return -1;
            }
            s = strtok(0, "/"); // call strtok() until it returns NULL
            // make start the parent node
            start = p;
        }
    }
}

int mkdir(char *pathname)
{
    NODE *p, *q;
    printf("mkdir: pathname=%s\n", pathname);

    // 1. search for the dirname node
    if (!strcmp(pathname, "/") || !strcmp(pathname, ".") || !strcmp(pathname, ".."))
    {
        printf("can't mkdir with %s\n", pathname);
        return -1;
    }

    // 2. Break up pathname into dirname and basename

    dbname(pathname);
    // only basename hold the directory
    if (strcmp(dname, ".") == 0 || strcmp(bname, ".") == 0)
    {
        // move start to the mkdir position
        set_start(bname);
    }
    else
        set_start(dname);
    // 3. Check if dirname exists and is a DIR
    printf("check whether %s already exists\n", bname);
    p = search_child(start, bname);
    if (p)
    {
        printf("name %s already exists, mkdir FAILED\n", bname);
        return -1;
    }
    printf("--------------------------------------\n");
    printf("ready to mkdir %s\n", bname);
    q = (NODE *)malloc(sizeof(NODE));
    q->type = 'D';
    strcpy(q->name, bname);
    insert_child(start, q);
    printf("mkdir %s OK\n", bname);
    printf("--------------------------------------\n");

    return 0;
}

int create(char *pathname)
{
    NODE *p, *q;
    printf("creat: pathname=%s\n", pathname);

    // 1. search for the dirname node
    if (!strcmp(pathname, "/") || !strcmp(pathname, ".") || !strcmp(pathname, ".."))
    {
        printf("can't creat with %s\n", pathname);
        return -1;
    }

    // 2. Break up pathname into dirname and basename
    dbname(pathname);
    if (strcmp(bname, "") != 0 && strcmp(dname, "") != 0)
        // move start to the creat position
        set_start(dname);
    else
        start = cwd;
    // 3. Check if dirname exists and is a DIR
    printf("check whether %s already exists\n", bname);
    p = search_child(start, bname);
    if (p)
    {
        printf("name %s already exists, creat FAILED\n", bname);
        return -1;
    }
    printf("--------------------------------------\n");
    printf("ready to creat %s\n", bname);
    q = (NODE *)malloc(sizeof(NODE));
    q->type = 'F';
    strcpy(q->name, bname);
    insert_child(start, q);
    printf("creat %s OK\n", bname);
    printf("--------------------------------------\n");

    return 0;
}

// This ls() list CWD. You MUST improve it to ls(char *pathname)
int ls(char *pathname)
{
    if (!strcmp(pathname, "/") || !strcmp(pathname, ".") || !strcmp(pathname, ".."))
    {
        printf("can't ls with %s\n", pathname);
        return -1;
    }

    if (pathname == NULL || strcmp(pathname, "") == 0)
    {
        printf("pathname is empty\n");
        start = cwd;
        printf("cwd %s\n", cwd->name);
    }
    else
    {
        // move start to the ls position
        dbname(pathname);
        if (strcmp(dname, ".") == 0)
        {
            set_start(bname);
        }
        else
            set_start(dname);
    }
    NODE *p = start->child;
    printf("ls contents = ");
    while (p)
    {
        printf("[%c %s] ", p->type, p->name);
        p = p->sibling;
    }
    printf("\n");
}

int quit()
{
    printf("Program exit\n");
    exit(0);
    // improve quit() to SAVE the current tree as a Linux file
    // for reload the file to reconstruct the original tree
}

int initialize()
{
    root = (NODE *)malloc(sizeof(NODE));
    strcpy(root->name, "/");
    root->parent = root;
    root->sibling = 0;
    root->child = 0;
    root->type = 'D';
    cwd = root;
    printf("Root initialized OK\n");
}

int main()
{
    int index;

    initialize();

    printf("NOTE: commands = [mkdir|ls|quit]\n");

    while (1)
    {
        printf("Enter command line : ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;

        command[0] = pathname[0] = 0;
        sscanf(line, "%s %s", command, pathname);
        printf("command=%s pathname=%s\n", command, pathname);

        if (command[0] == 0)
            continue;

        index = findCmd(command);

        switch (index)
        {
        case 0:
            mkdir(pathname);
            break;
        case 1:
            ls(pathname);
            break;
        case 2:
            quit();
            break;
        case 3:
            rmdir(pathname);
            break;
        case 4:
            cd(pathname);
            break;
        case 5:
            pwd();
            break;
        case 6:
            create(pathname);
            break;
        case 7:
            rm(pathname);
            break;
        }
    }
}

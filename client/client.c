//------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h> 
#include <arpa/inet.h>

#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>      
#include <time.h> 

//--------------------Globals/Macros-------------------------
struct sockaddr_in saddr;     // server addr struct
char *serverIP = "127.0.0.1"; // server IP address
int  sfd;                     // socket  
//char *serverIP = "172.19.44.43"; // server IP address
#define PORT 1234             // server port number
#define MAX   256             // set size to send/recieve data
#define BLKSIZE 1024          // used for cp/lcp to copy in larger chunks

char line[MAX];                //Used for collectin user cmd
char *tokens[10];              // maximum number of tokens to be stored
char *commandList[] = {"ls", "cd", "pwd", "cp", "lls", "lcd", "lpwd", "lcp"};             //List of avaliable commands
int length = (sizeof(commandList) / sizeof(commandList[0]));//Used for the for loop when matching the user inputted cmd to the list of avaliable commands
char linkname[MAX];
char *t1 = "xwrxwrxwr-------";      //Used for ls printing
char *t2 = "----------------";      //Used for ls printing 


//-----------------Function Declarations-------------------
void printMenu();           //General client gui

int ls(int sfd, char *cmd);            //Server CMD
int cd(int sfd, char *cmd, char *file1);            //Server CMD
int pwd(int sfd, char *cmd);            //Server CMD
int cp(int sfd, char *cmd, char *file1, char *file2);            //Server CMD

int lls(char *dname);            //Client local CMD
int lcd(char *pathname);            //Client local CMD
void lpwd();            //Client local CMD
int lcp(char *src, char *dest);            //Client local CMD
int lls_file(char *fname);            //Client local CMD


//-------------------------Main-----------------------------
int main(int argc, char *argv[], char *env[]) 
{ 
    printf("length: %d\n", length);
    //-----------------Sets up Connection with Server-----------------------
    int n; 
    printf("1. create a TCP socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET; 
    saddr.sin_addr.s_addr = inet_addr(serverIP); 
    saddr.sin_port = htons(PORT); 
  
    printf("3. connect to server\n");
    if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 

    printf("********  processing loop  *********\n");
    while (1){
        //-------------Local Cmds and Tokenize to send to server------------------
        printMenu();
        

        int n;
        printf("input a line : ");
        bzero(line, MAX);                // zero out line[ ]
        fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

        line[strlen(line)-1] = 0;        // kill \n at end
        if (line[0]==0)                  // exit if NULL line
            exit(0); 

        int i = 0;              //Used as incrementer
        char *token = strtok(line, " ");        //tokenizes beased on space character
        while (token != NULL) {
            tokens[i++] = token;        //each token is assigned to the tokens var
            token = strtok(NULL, " ");      //goes until all token have been assigned and null is returned
        }

        // Print out all the tokens
        /*for (int j = 0; j < i; j++) {
            printf("%s\n", tokens[j]);
        }*/
        printf("\ncmd=%s file1=%s file2=%s\n", tokens[0], tokens[1], tokens[2]);

        int locationCMD = -1; //Set to some random num
        for (i = 0; i < length; i++) {//checks to see if user cmd is in cmd list
            if (strcmp(commandList[i], tokens[0]) == 0) {
                locationCMD = i;
            }
        }

        if (locationCMD != -1){//If it equals -1 that means that the cmd is outside of the list boundarys
            //printf("Command is in the list\n");
            switch(locationCMD){
                case 0:
                    //printf("ls\n");
                    ls(sfd, tokens[0]);
                    break;
                case 1:
                    //printf("cd case\n");
                    cd(sfd, tokens[0], tokens[1]);
                    break;
                case 2:
                    //printf("pwd\n");
                    pwd(sfd, tokens[0]);
                    break;
                case 3:
                    printf("cp case\n");
                    cp(sfd, tokens[0], tokens[1], tokens[2]);
                    break;
                case 4:
                    //printf("lls\n");
                    lls(".");
                    break;
                case 5:
                    //printf("lcd\n");
                    lcd(tokens[1]);
                    break;
                case 6:
                    //printf("lpwd\n");
                    lpwd();
                    break;
                case 7:
                    //printf("lcp\n");
                    lcp(tokens[1], tokens[2]);
                    break;
                default:
                    printf("Default Switch Case Client\n");
                    
            }
        } 
        else{ 
            printf("Invalid Command\n");
        }

        tokens[1] = NULL;
        tokens[2] = NULL;
    }
}

void printMenu()
{
    printf("\n********** menu *******\n*  ls   cd   pwd   cp *\n* lls  lcd  lpwd  lcp *\n***********************\n");
}

int ls(int sfd, char *cmd)
{
    //While loop until all the folder is ls'd
    write(sfd, cmd, MAX);
    //printf("client: wrote n=%d bytes; send=[%s]\n", n, sender);

    //The server sends the keyword "END" which means that ls is completed and that read should end. So every loop needs to be checked to see if "END" has been checked
    char endOfTest[] = "END";
    while(strcmp(line, endOfTest) != 0){
        // Read a line from sock and show it
        read(sfd, line, MAX);
        if((strcmp(line, endOfTest) != 0))
        {
            printf("%s\n", line);
        }
    }   
}

int cd(int sfd, char *cmd, char *file1)
{
    printf("%s %s",cmd, file1);
    //I only send one line set to the server to reduce traffic. So for cd cmd file1 it all has to strcat together for the server to tokenize
    char sender[MAX] = "\0";
    char space[] = " ";
    strcat(sender, cmd);
    strcat(sender, space);
    strcat(sender, file1);
    //----------------Communicates with the Server--------------------------
    // Send ENTIRE line to server
    write(sfd, sender, MAX);
    //printf("client: wrote n=%d bytes; send=[%s]\n", n, sender);

    // Read a line from sock and show it
    read(sfd, line, MAX);
    //printf("client: read  n=%d bytes; recv=[%s]\n",n, line);
    printf(" %s\n", line);
    //---------------------------------------------------------

}

int pwd(int sfd, char *cmd)
{
    //----------------Communicates with the Server---------------
    // Send ENTIRE line to server
    write(sfd, cmd, MAX);
    //printf("client: wrote n=%d bytes; send=[%s]\n", n, sender);

    // Read a line from sock and show it
    read(sfd, line, MAX);
    printf("%s\n", line);
    //-----------------------------------------------------------
}

int cp(int sfd, char *cmd, char *file1, char *file2)
{
    //strcat tokens[0] and [1] and [2]
    char sender[MAX] = "\0";
    char space[] = " ";
    strcat(sender, cmd);
    strcat(sender, space);
    strcat(sender, file1);
    strcat(sender, space);
    strcat(sender, file2);
    printf("this7 %s\n", sender);
    //----------------Communicates with the Server--------------------------
    // Send ENTIRE line to server
    int n;
    n = write(sfd, sender, MAX);
    printf("client: wrote n=%d bytes; send=[%s]\n", n, sender);

    // Read a line from sock and show it
    n = read(sfd, line, MAX);
    printf("client: read  n=%d bytes; recv=[%s]\n",n, line);
    //---------------------------------------------------------
}

int lls(char *dname)
{
    DIR *dir;
    struct dirent *entry;
    char path[1024];
    dir = opendir(dname);
    if (dir == NULL)
    {
        printf("cannot open directory %s\n", dname);
        return -1;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dname, entry->d_name);
        lls_file(path);
    }
    closedir(dir);
    return 0;
}

int lls_file(char *fname)//Just to be clear, this is taken right from KC Wangs textbook 8.6.7 like a lot of the functions in this lab.
{
    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(fname, &fstat)) < 0)
    {
        printf("can’t stat %s\n", fname);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c", '-');
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c", 'd');
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c", 'l');
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i))
            printf("%c", t1[i]); // print r|w|x printf("%c", t1[i]);
        else
            printf("%c", t2[i]); // or print -
    }
    printf("%4ld ", sp->st_nlink); // link count
    printf("%4d ", sp->st_gid);   // gid
    printf("%4d ", sp->st_uid);   // uid
    printf("%8ld ", sp->st_size);  // file size

    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form ftime[strlen(ftime)-1] = 0; // kill \n at end
    ftime[strlen(ftime) - 1] = 0;        // removes the \n
    printf("%s ", ftime);                // prints the time

    printf("%s", basename(fname)); // print file basename // print -> linkname if symbolic file
    if ((sp->st_mode & 0xF000) == 0xA000)
    {
        readlink(fname, linkname, MAX);
        printf(" -> %s", linkname); // print linked name
    }

    printf("\n");
}

int lcd(char *pathname)//Code taken directly from texbook for cd
{
    int r = chdir(pathname);
    if (r == 0)// If the operation is successful, chdir() returns 0. If the operation fails, chdir() returns -1
    {
        printf("cd %s OK", pathname);
    }
    else
    {
        printf("cd %s FAILED", pathname);
    }
}

void lpwd()//Code taken directly from texbook for pwd
{
    char buf[MAX];
    getcwd(buf, MAX);
    printf("pwd=%s\n", buf);
}

int lcp(char *src, char *dest)
{
    /*
    //This is from the textbook at section 8.10.2 -- I couldn't get this to work so i went and made another version from scratch
    int fd, gd, n, total = 0;
    char buf[BLKSIZE];
    if ((fd = (open(src, O_RDONLY)) < 0))
        exit(2);
    if ((gd = open(dest, O_WRONLY | O_CREAT)) < 0)
        exit(3);
    while (n = read(fd, buf, BLKSIZE)){
        write(gd, buf, n);
        total += n;
    }
    printf("total bytes copied=%d\n", total);
    close(fd); 
    close(gd);
    */
    int src_fd, dst_fd, num_read, num_written;
    char buf[BLKSIZE];
    struct stat src_stat;

    // Open src file for reading
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("Failed to open src file");
        return -1;
    }
    
    if (fstat(src_fd, &src_stat) == -1) {// Get src file information
        perror("Failed to get src file information");
        close(src_fd);
        return -1;
    }

    dst_fd = open(dest, O_CREAT | O_WRONLY | O_TRUNC, src_stat.st_mode);// Open dest file for writing
    if (dst_fd == -1) {
        perror("Failed to open dest file");
        close(src_fd);
        return -1;
    }

    while ((num_read = read(src_fd, buf, BLKSIZE)) > 0) {// Copy data from source file to destination file
        num_written = write(dst_fd, buf, num_read);
        if (num_written != num_read) {
            perror("Failed to write to dest file");
            close(src_fd);
            close(dst_fd);
            return -1;
        }
    }

    // Close files
    close(src_fd);
    close(dst_fd);

    return 0;
}



/*
If you want to change the contents of what the pointer is pointing to, pass it into the function by reference, passing the variable direclty is good for most cases if you just using it to check the value.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <netdb.h> 
#include <sys/types.h> 
#include <arpa/inet.h>

#include <netinet/in.h>  
#include <sys/socket.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <time.h>

/************ socket address structure ************
struct sockaddr_in {
   sa_family_t    sin_family;   // AF_INET for TCP/IP
   in_port_t      sin_port;     // port number
   struct in_addr sin_addr;     // IP address
};
If your program crashes on server side, it never closed the port so use this cmd to clear it
sudo fuser -k Port_Number/tcp


sudo fuser -k 1234/tcp
***************************************************/


char *serverIP = "127.0.0.1";  // local host IP address
//char *serverIP = "172.19.44.43";  // by ifconfig
#define PORT 1234              // hardcoded port number

#define MAX 256
char line[MAX];
char *tokens[10]; // maximum number of tokens to be stored
char *commandList[] = {"ls", "cd", "pwd", "cp"}; //List of avaliable commands
int listlength = (sizeof(commandList) / sizeof(commandList[0]));//Used for the for loop when matching the user inputted cmd to the list of avaliable commands
char linkname[MAX];
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
#define BLKSIZE 1024
char full_line[MAX];
int stopper = 0;

//Function prototypes
int ls(char *dname, int cfd);
int cd(char *pathname, int cfd);
void pwd(int cfd);
int cp(char *src, char *dest, int cfd);
int ls_file(char *fname, int cfd);


int main() 
{
    
    struct sockaddr_in saddr;  // server addr struct
    struct sockaddr_in caddr;  // client addr struct 

    int sfd, cfd;              // sockets 
    int n, length;
    
    int me = getpid();
    
    printf("1. create a socket\n");
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
    
    printf("3. bind socket to server\n");
    if ((bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) != 0) { 
        printf("socket bind failed\n"); 
        exit(0); 
    }
      
    // Now server is ready to listen and verification 
    if ((listen(sfd, 5)) != 0) { 
        printf("Listen failed\n"); 
        exit(0); 
    }
    while(1){

       printf("server %d ready for client to connect\n", me);

       //Allows for more than 1 client to connect, it's not dynamic but it does allow for more than 1
       fork();

       // Try to accept a client connection as socket descriptor cfd
       length = sizeof(caddr);
       cfd = accept(sfd, (struct sockaddr *)&caddr, &length);
       if (cfd < 0){
          printf("server: accept error\n");
          exit(1);
       }

       printf("server %d: accepted a client connection from\n", me);
       printf("-----------------------------------------------\n");
       printf("IP=%s  port=%d\n", inet_ntoa(caddr.sin_addr),
                                      ntohs(caddr.sin_port));
       printf("-----------------------------------------------\n");



       char cwd[1024];
       char *root = (getcwd(cwd, sizeof(cwd)));
       if (chroot(root) != 0) {
           perror("chroot failed");
           exit(1);
       }
       if (chdir("/") != 0) {
           perror("chdir failed");
           exit(1);
       }
       // Now the root directory is set to .../cwd
       // and all path names are interpreted relative to this new root directory.
       // I can perform operations in this virtual root directory.
       //printf("Virtual root set to %s\n", root);
        

       // Processing loop
       while(1){

            printf("server %d: ready for next request ....\n", me);

            //N variable contains the sent text string form client
            n = read(cfd, line, MAX);
            if (n==0){
                printf("server: client gone, server %d loops\n", me);
                close(cfd);
            break;
            }

            //----------My additions to the server--------------
            tokens[1] = NULL;
            tokens[2] = NULL;
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
            for (i = 0; i < listlength; i++) {//checks to see if user cmd is in cmd list
                if (strcmp(commandList[i], tokens[0]) == 0) {
                    locationCMD = i;
                }
            }

            if (locationCMD != -1){//If it equals -1 that means that the cmd is outside of the list boundarys
                //printf("Command is in the list\n");
                switch(locationCMD){
                    case 0:
                        printf("ls\n");
                        ls(".", cfd);
                        break;
                    case 1:
                        printf("cd\n");
                        cd(tokens[1], cfd);
                        break;
                    case 2:
                        printf("pwd\n");
                        pwd(cfd);
                        break;
                    case 3:
                        printf("cp\n");
                        cp(tokens[1], tokens[2], cfd);
                        break;
                    default:
                        printf("client cmd\n");
                        //lock = 1;
                }
            } 
            else{ 
                printf("Invalid Command\n");
            }

            /*
            //--------------------------------
            // show the line string
            printf("server %d: read  n=%d bytes; recv=[%s]\n", me, n, line);

            strcat(line, " ECHO");
            // send the echo line to client 
            n = write(cfd, line, MAX);

            printf("server %d: wrote n=%d bytes; send=[%s]\n", me, n, line);
            */

        }
    }
}


int ls(char *dname, int cfd)
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
        if(stopper == 0)
        {
            if (entry->d_name[0] == '.')
                continue;
            snprintf(path, sizeof(path), "%s/%s", dname, entry->d_name);
            ls_file(path, cfd);
        }
    }
    char endOfTest[] = "END";
    write(cfd, endOfTest, MAX);
    //printf("sent: %s\n", endOfTest);

    closedir(dir);
    return 0;
}

int ls_file(char *fname, int cfd)
{
    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;

    memset(full_line, 0, sizeof(full_line));
    stopper = 0;//This was used to test one line of the ls at a line when making it.
    char line[256];
    memset(line, 0, sizeof(line));
    char *hypen = "-";
    char *ll = "l";
    char *ld = "d";
    char space[] = " ";
    char arrow[] = " ->";
    
    if ((r = lstat(fname, &fstat)) < 0)
    {
        printf("cant stat %s\n", fname);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
    {
        //printf("%c", '-');
        strcat(full_line, hypen);
    }
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    {
        //printf("%c", 'd');
        strcat(full_line, ld);
    }
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
    {
        //printf("%c", 'l');
        strcat(full_line, ll);
    }
    
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i))
            //printf("%c", t1[i]); // print r|w|x printf("%c", t1[i]);
            strncat(full_line, &t1[i], 1);
        else
            //printf("%c", t2[i]); // or print -
            strncat(full_line, &t2[i], 1);
    }

    /*
    printf("%4ld ", sp->st_nlink); // link count
    printf("%4d ", sp->st_gid);   // gid
    printf("%4d ", sp->st_uid);   // uid
    printf("%8ld ", sp->st_size);  // file size
    */
    sprintf(line, "%4ld %4d %4d %8ld", (long) sp->st_nlink, sp->st_gid, sp->st_uid, (long) sp->st_size);
    strcat(full_line, line);


    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form ftime[strlen(ftime)-1] = 0; // kill \n at end
    ftime[strlen(ftime) - 1] = 0;        // removes the \n
    //printf("%s ", ftime);                // prints the time
    strcat(full_line, space);
    strcat(full_line, ftime);

    
    //printf("%s", basename(fname)); // print file basename // print -> linkname if symbolic file
    strcat(full_line, space);
    strcat(full_line, basename(fname));

    if ((sp->st_mode & 0xF000) == 0xA000)
    {
        readlink(fname, linkname, MAX);
        //printf(" -> %s", linkname2); // print linked name
        strcat(full_line, arrow);
        strcat(full_line, linkname);
    }

    // send the echo line to client 

    write(cfd, full_line, MAX);
    printf("%s\n", full_line);

}

int cd(char *pathname, int cfd)//Code taken directly from texbook for cd
{
    int r = chdir(pathname);
    if (r == 0)// If the operation is successful, chdir() returns 0. If the operation fails, chdir() returns -1
    {
        char ok[] = "OK";
        write(cfd, ok, MAX);
        printf("cd %s OK\n", pathname);
    }
    else
    {
        char failed[] = "FAILED";
        write(cfd, failed, MAX);
        printf("cd %s FAILED\n", pathname);
    }
}

void pwd(int cfd)//Code taken directly from texbook for pwd
{
    char buf[MAX];
    getcwd(buf, MAX);
    printf("pwd=%s\n", buf);
    
    // send the echo line to client 
    int n;
    n = write(cfd, buf, MAX);
    printf("wrote n=%d bytes; send=[%s]\n", n, buf);

}

int cp(char *src, char *dest, int cfd)
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

    /*
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

    */
    printf("OK %s %s\n", src, dest);
    char buf[] = "ItWorked";
    // send the echo line to client 
    int n;
    n = write(cfd, buf, MAX);
    printf("wrote n=%d bytes; send=[%s]\n", n, buf);

}

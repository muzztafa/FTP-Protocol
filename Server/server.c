#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>
#include <wait.h>
#include <sys/stat.h>
#include <dirent.h>

int cmd_list(int sockfd);
void child(int);
void cmd_retr(char *, char *, int, int);
void cmd_stor(char *, char *, int, int);
void cmd_appe(char *, char *, int, int);
int cmd_cwd(char *directory, int sockid);
int cmd_cdup(int sockid);
int cmd_user(int sockid);
int get_current(int sockid);
int cmd_rein(int sockid);
int cmd_mkd(char *, int sockid);
int cmd_pwd(int sockid);
int cmd_quit(int sockid);
int cmd_noop(int sockid);
int cmd_rest(int sockid);
int cmd_abor(int sockid);
int cmd_dele(char *arg, int sockid);
int cmd_rmd(char *arg, int sockid);
int cmd_rnfr(char *arg, int sockid);
int cmd_rnto(char *arg, int sockid);

int loggedIn = 0;
int processRunning = 0;
int client;
char renameFrom[50] = "";
int renameCheck = 0;

void trapSignals()
{
  close(client);
  exit(0);
}

int main(int argc, char *argv[])
{
  int sd, portNumber, status;
  struct sockaddr_in servAdd; // client socket address

  signal(SIGINT, trapSignals);
  signal(SIGTSTP, trapSignals);

  if (argc < 2)
  {
    printf("Call model: %s <Port Number> -d <Path>\n", argv[0]);
    exit(0);
  }

  if (argc == 4)
  {
    // changing path
    if (cmd_cwd(argv[3], -1))
    {
      exit(0);
    }
  }

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "Cannot create socket\n");
    exit(1);
  }
  sd = socket(AF_INET, SOCK_STREAM, 0);
  servAdd.sin_family = AF_INET;
  servAdd.sin_addr.s_addr = htonl(INADDR_ANY);
  sscanf(argv[1], "%d", &portNumber);
  servAdd.sin_port = htons((uint16_t)portNumber);

  bind(sd, (struct sockaddr *)&servAdd, sizeof(servAdd));
  listen(sd, 5);

  while (1)
  {
    printf("Waiting for a client to chat with\n");
    client = accept(sd, NULL, NULL);
    printf("Got a client %d, start chatting\n", client);

    if (!fork())
      child(client);

    close(client);
    waitpid(0, &status, WNOHANG);
  }
}

void child(int sd)
{
  char message[255];
  int n, pid;

  char command[5]; // Decoded Command
  char arg[1024];  // Decoded args

  write(sd, "Welcome to FTP Server! Start typing a command", 35);
  /* reading client messages */
  while (1)
  {
    if (n = read(sd, message, 255))
    {

      message[n] = '\0';
      sscanf(message, "%s %s", command, arg);
      // fprintf(stderr, "Client entered the command: %s with args: %s\n", command, arg);

      // Commands
      if (loggedIn == 0)
      {
        // printf("user not logged in \n");

        if (!strcasecmp(command, "USER"))
        {
          cmd_user(sd);
        }
        else
        {
          write(sd, "Error: User not logged in", 35);
        }
      }
      else
      {
        // cmd_user(command, sd, n);
        if (!strcasecmp(command, "CWD"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          if (!strcasecmp(arg, ""))
          {
            write(sd, "Error: No directory given", 35);
            // printf("no directory given\n");
          }
          else
          {
            cmd_cwd((char *)arg, sd);
          }
        }
        else if (!strcasecmp(command, "CDUP"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_cdup(sd);
        }
        else if (!strcasecmp(command, "REIN"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_rein(sd);
        }
        else if (!strcasecmp(command, "QUIT"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_quit(sd);
        }
        //  else if(!strcasecmp(command, "PORT")){}
        else if (!strcasecmp(command, "RETR"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_retr(command, arg, sd, n);
        }
        else if (!strcasecmp(command, "STOR"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_stor(command, arg, sd, n);
        }
        else if (!strcasecmp(command, "APPE"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_appe(command, arg, sd, n);
        }
        else if (!strcasecmp(command, "REST"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_rest(sd);
        }
        else if (!strcasecmp(command, "RNFR"))
        {
          cmd_rnfr(arg, sd);
        }
        else if (!strcasecmp(command, "RNTO"))
        {
          cmd_rnto(arg, sd);
        }
        else if (!strcasecmp(command, "ABOR"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_abor(sd);
        }
        else if (!strcasecmp(command, "DELE"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_dele(arg, sd);
        }
        else if (!strcasecmp(command, "RMD"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_rmd(arg, sd);
        }
        else if (!strcasecmp(command, "MKD"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_mkd((char *)arg, sd);
        }
        else if (!strcasecmp(command, "PWD"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_pwd(sd);
        }
        else if (!strcasecmp(command, "LIST"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_list(sd);
        }
        //  else if(!strcasecmp(command, "STAT")){}
        else if (!strcasecmp(command, "NOOP"))
        {
          renameCheck = 0; // if rnto isnt run directly after rnfr
          cmd_noop(sd);
        }
        else
        {
          write(sd, "No such command available", 255);
        }
      }
    }
  }
}

int cmd_rnfr(char *rnfr_file, int sockfd)
{
  if (access(rnfr_file, F_OK) == 0)
  {
    // file exists
    strncpy(renameFrom, rnfr_file, 50);
    renameCheck = 1;
    write(sockfd, "200, rename from recorded", 255);
  }
  else
  {
    // file doesn't exist
    write(sockfd, "400, file doesnt exist", 255);
  }
}

int cmd_rnto(char *newName, int sockfd)
{
  if (renameCheck == 0)
  {
    write(sockfd, "Error: rnto must run right after rnfr", 255);
    return 0;
  }

  if (!rename(renameFrom, newName))
  {
    write(sockfd, "200, Rename successful ", 255);
  }
  else
  {
    perror("Error: \n");
    write(sockfd, "Error: 403, renaming file failed", 255);
  }
}

int cmd_rmd(char *path, int sockfd)
{
  if (!rmdir(path))
  {
    write(sockfd, "200, directory removed", 255);
  }
  else
  {
    write(sockfd, "Error: in deleting dir", 255);
  }
}

int cmd_dele(char *fileName, int sockfd)
{
  if (!remove(fileName))
  {
    write(sockfd, "200, file removed successfully", 255);
  }
  else
  {
    write(sockfd, "Error: in deleting file", 255);
  }
}
int cmd_abor(int sockfd)
{
  processRunning = 0;
  write(sockfd, "226, abort was successful", 255);
}
int cmd_rest(int sockfd)
{
  write(sockfd, "File transfer can be resumed", 255);
}

int cmd_noop(int sockfd)
{
  write(sockfd, "Ok", 255);
}
int cmd_list(int sockfd)
{
  DIR *d;
  struct dirent *dir;
  d = opendir(".");
  if (d)
  {
    write(sockfd, "150 File status okay; about to open data connection.", 255);
    write(sockfd, "125 Data connection already open; transfer starting.", 255);
    while ((dir = readdir(d)) != NULL)
    {
      write(sockfd, dir->d_name, 255);
    }
    closedir(d);
  }
}

void cmd_stor(char *command, char *arg, int sockfd, int n)
{
  FILE *fp;
  char *filename = arg;
  char buffer[255];

  fp = fopen(filename, "w");
  if (fp == NULL)
  {
    write(sockfd, "Error in opening/writing file.", 35);
    // perror("[-]Error in opening/writing file.");
    return;
  }

  while (1)
  {
    if (n = read(sockfd, buffer, 255))
    {
      // buffer[n] = '\0';
      //  printf("Buffer: %s\n", buffer);
      //  printf("N: %d\n",n);
      if (!strcasecmp(buffer, "END"))
      {
        break;
      }
      fprintf(fp, "%s", buffer);
      bzero(buffer, 255);
    }
    else
    {
      break;
    }
  }
  write(sockfd, "Server received file successfully", 35);
  // printf("Server received file successfully\n");
  fclose(fp);
}

void cmd_appe(char *command, char *arg, int sockfd, int n)
{
  FILE *fp;
  char *filename = arg;
  char buffer[255];

  fp = fopen(filename, "a");
  if (fp == NULL)
  {
    write(sockfd, "Error in opening/writing file.", 35);
    // perror("[-]Error in opening/writing file.");
    return;
  }

  while (1)
  {
    if (n = read(sockfd, buffer, 255))
    {
      // buffer[n] = '\0';
      //  printf("Buffer: %s\n", buffer);
      //  printf("N: %d\n",n);
      if (!strcasecmp(buffer, "END"))
      {
        break;
      }
      fprintf(fp, "%s", buffer);
      bzero(buffer, 255);
    }
    else
    {
      break;
    }
  }
  write(sockfd, "Server received file successfully", 35);
  // printf("Server received file successfully\n");
  fclose(fp);
}

void cmd_retr(char *command, char *arg, int sockfd, int n)
{
  FILE *fp;
  char *filename = arg;
  char message[255] = {0};
  processRunning = 1;

  fp = fopen(filename, "r");
  if (fp == NULL)
  {
    write(sockfd, "Error in reading file on server", 255);
    // perror("[-]Error in reading file.");
    processRunning = 0;
    return;
  }

  // sending filename and command back to client
  char temp[15] = "retr ";
  strcat(temp, filename);
  write(sockfd, temp, strlen(temp) + 1);

  while (fgets(message, 255, fp) != NULL)
  {
    write(sockfd, message, 255);
    bzero(message, 255);
  }
  write(sockfd, "END", 3); // sending END so that client doesnt wait
  printf("Server sent file successfully\n");
  fclose(fp);
  processRunning = 0;

  return;
}

int cmd_cwd(char *path, int sockfd)
{
  if (chdir(path) == -1)
  {
    if (sockfd != -1)
    {
      write(sockfd, "Error: cannot change directory", 255);
    }
    // printf("cannot change directory\n");
    return 1;
  }
  else
  {
    if (sockfd != -1)
    {
      write(sockfd, "200, directory changed", 255);
    }
    // printf("200 directory changed to: %s\n", path);
    return 0;
  }
}
int cmd_cdup(int sockfd)
{

  // strtok(directory, "\n");
  // printf("in directory \n");
  if (chdir("..") == -1)
  {
    write(sockfd, "Error: Cannot change directory to parent", 255);
    // printf("cannot change directory to parent\n");
    return 1;
  }
  else
  {
    write(sockfd, "200 directory changed to parent directory\n", 255);
    return 0;
  }
}

int cmd_user(int sockfd)
{
  write(sockfd, "230 User logged in, proceed", 255);
  loggedIn = 1;
}

int cmd_pwd(int sockfd)
{
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
  {
    char temp[50] = "Current working dir:  ";
    strcat(temp, cwd);
    write(sockfd, temp, 255);
  }
  else
  {
    write(sockfd, "Error: Getcwd()", 255);
    // perror("getcwd() error\n");
    return 1;
  }
  return 0;
}

int cmd_quit(int sockfd)
{

  while (1)
  {
    if (!processRunning)
    {
      break;
    }
  }
  loggedIn = 0;
  printf("Successfuly logged out \n");
  exit(0);
}

int cmd_rein(int sockfd)
{
  while (1)
  {
    if (!processRunning)
    {
      break;
    }
  }
  loggedIn = 0;
  write(sockfd, "Reinitializing..", 255);
  // printf("reinitializing\n");
}

int cmd_mkd(char *directory, int sockfd)
{
  if (mkdir(directory, 0700) == -1)
  {
    write(sockfd, "Error: in creating a directory", 255);
    // printf("error in making directory \n");
  }

  else
  {
    write(sockfd, "Directory created", 255);
    // printf("directory created \n");
  }
}
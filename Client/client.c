#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

void cmd_retr_result(int sockfd, char *arg);
void cmd_stor(int sockfd, char *arg);
void cmd_appe(int sockfd, char *arg);

int server;

void trapSignals()
{
    close(server);
    exit(0);
}

int main(int argc, char *argv[])
{
    char message[255];
    int portNumber, pid, n;
    struct sockaddr_in servAdd; // server socket address
    char command[5];            // Decoded Command
    char arg[1024];             // Decoded args

    signal(SIGINT, trapSignals);
    signal(SIGTSTP, trapSignals);

    if (argc != 3)
    {
        printf("Call model: %s <IP Address> <Port Number>\n", argv[0]);
        exit(0);
    }

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }

    servAdd.sin_family = AF_INET;
    sscanf(argv[2], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);

    if (inet_pton(AF_INET, argv[1], &servAdd.sin_addr) < 0)
    {
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }

    if (connect(server, (struct sockaddr *)&servAdd, sizeof(servAdd)) < 0)
    {
        fprintf(stderr, "connect() has failed, exiting\n");
        exit(3);
    }

    read(server, message, 255);
    fprintf(stderr, "message received: %s\n", message);

    pid = fork();

    if (pid)
        while (1) /* reading server's messages */
            if (n = read(server, message, 255))
            {
                message[n] = '\0';
                printf("Server: %s\n", message);
                sscanf(message, "%s %s", command, arg);

                // printf("command: %s %s \n", command, arg);
                if (!strcasecmp(command, "RETR"))
                {
                    cmd_retr_result(server, arg);
                }
            }

    if (!pid) /* sending messages to server */
        while (1)
            if (n = read(0, message, 255))
            {
                message[n] = '\0';
                sscanf(message, "%s %s", command, arg);
                write(server, message, strlen(message) + 1);

                if (!strcasecmp(command, "RETR\n"))
                {
                    cmd_retr_result(server, arg);
                }
                else if (!strcasecmp(message, "QUIT\n"))
                {
                    kill(getppid(), SIGTERM);
                    exit(0);
                }
                else if (!strcasecmp(command, "STOR"))
                {
                    cmd_stor(server, arg);
                }
                else if (!strcasecmp(command, "APPE"))
                {
                    cmd_appe(server, arg);
                }
            }
}

void cmd_retr_result(int sockfd, char *arg)
{
    int n;
    FILE *fp;
    char *filename = arg;
    char buffer[255];

    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("[-]Error in opening/writing file.");
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
    printf("Client received file successfully\n");
    fclose(fp);
}

void cmd_stor(int sockfd, char *arg)
{
    FILE *fp;
    char *filename = arg;
    char message[255] = {0};

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        write(sockfd, "Error in reading file on server", 255);
        perror("[-]Error in reading file.");
        return;
    }

    // sending filename and command back to client
    // char temp[15] = "retr ";
    // strcat(temp, filename);
    // write(sockfd, temp, strlen(temp) + 1);

    while (fgets(message, 255, fp) != NULL)
    {
        write(sockfd, message, 255);
        bzero(message, 255);
    }
    write(sockfd, "END", 3); // sending END so that server doesnt wait
    printf("Client sent file successfully\n");
    fclose(fp);
    return;
}

void cmd_appe(int sockfd, char *arg)
{
    FILE *fp;
    char *filename = arg;
    char message[255] = {0};

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        write(sockfd, "Error in reading file on server", 255);
        perror("[-]Error in reading file.");
        return;
    }

    // sending filename and command back to client
    // char temp[15] = "retr ";
    // strcat(temp, filename);
    // write(sockfd, temp, strlen(temp) + 1);

    while (fgets(message, 255, fp) != NULL)
    {
        write(sockfd, message, 255);
        bzero(message, 255);
    }
    write(sockfd, "END", 3); // sending END so that server doesnt wait
    printf("Client sent file successfully\n");
    fclose(fp);
    return;
}
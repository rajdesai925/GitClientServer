#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>

void create (int sockfd, char* projectName) {
  int len = strlen(projectName);
  char titleLength[1024];
  sprintf(titleLength, "%d", len);
  char request[1 + strlen(titleLength) + strlen(projectName)];
  bzero(request, (1 + strlen(titleLength) + 1 + strlen(projectName)));
  request[0] = 'c';
  strcat(request, titleLength);
  strcat(request, ":");
  strcat(request, projectName);
  write(sockfd, request, strlen(request));
  //printf("%s\n", request);
  char result;
  read(sockfd, &result, 1);
  if (result == 'e') {
    printf("Error: %s already exists\n", projectName);
  } else if (result == 's') {
    DIR* directory = opendir("./");
    mkdir(projectName, S_IRWXU);
    char manifestName[strlen(projectName) + strlen(".manifest")];
    bzero(manifestName, strlen(projectName) + strlen(".manifest"));
    strcat(manifestName, projectName);
    strcat(manifestName, ".manifest");
    chdir(projectName);
    int manifest = open(manifestName, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
    write(manifest, "0", 1);
    write(manifest, "\n", 1);
    printf("%s created successfully\n", projectName);
    close(manifest);
    closedir(directory);
  }

  return;
}

void destroy (int sockfd, char* projectName) {
  int len = strlen(projectName);
  char titleLength[1024];
  sprintf(titleLength, "%d", len);
  char request[1 + strlen(titleLength) + strlen(projectName)];
  bzero(request, (1 + strlen(titleLength) + 1 + strlen(projectName)));
  request[0] = 'd';
  strcat(request, titleLength);
  strcat(request, ":");
  strcat(request, projectName);
  write(sockfd, request, strlen(request));
  //printf("%s\n", request);
  char result;
  read(sockfd, &result, 1);
  if (result == 'e') {
    printf("Error: %s could not be found\n", projectName);
  } else if (result == 's') {
    printf("%s destroyed successfully\n", projectName);
  }
  return;
}

void configure (char* ip, char* port) {
  int config;
  config = open(".configure", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  write(config, ip, strlen(ip));
  write(config, " ", 1);
  write(config, port, strlen(port));
  write(config, " ", 1);
  close(config);
  printf("\nSERVER CONFIGURED :: ip = %s  port = %s\n\n", ip, port);
  return;
}

int main(int argc, char* argv[]) {

  //create .configure
  if (strcmp((argv[1]), "configure") == 0) {
    configure(argv[2], argv[3]);
    return 0;
  }

  //get ip and port from .configure
  int config = open(".configure", O_RDWR);
  if (config == -1) {
    printf("server ip not configured\n");
    return 0;
  }
  char ip[15];
  char port[5];
  char c;
  int i = 0;
  read(config, &c, 1);
  while (c != ' ') {
    ip[i] = c;
    i++;
    read(config, &c, 1);
  }
  ip[i] = '\0';
  read(config, &c, 1);
  i = 0;
  while (c != ' ') {
    port[i] = c;
    i++;
    read(config, &c, 1);
  }
  port[i] = '\0';
  int portConv = strtol(port, NULL, 10);


  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("socket failed\n");
    return 0;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_port = htons(portConv);

  if (connect(sockfd, (struct sockaddr *) &address, sizeof(address)) != 0) {
    printf("connection with the server failed\n");
    return 0;
  }
  printf("\nSERVER CONNECTED :: ip = %s  port = %d\n", ip, portConv);
  //---------------------------------------------------------------------------
  // Server Connected
  //---------------------------------------------------------------------------


  if (strcmp(argv[1], "create") == 0) {
    create(sockfd, argv[2]);
  }
  if (strcmp(argv[1], "destroy") == 0) {
    destroy(sockfd, argv[2]);
  }




  int cl = close(sockfd);
  if (cl == 0) printf("SERVER DISCONNECTED :: ip = %s  port = %d\n\n", ip, portConv);
  return 0;
}

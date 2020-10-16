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
#include <pthread.h>

pthread_mutex_t lock;

void create (int clientfd) {
  //this section will place the project name into string title
  char buffer[100];
  char delim;
  int digitsOfTitleLength = 0;
  int i;
  recv(clientfd, buffer, sizeof(buffer), MSG_PEEK);
  for (i = 0; i < strlen(buffer); i++) {
    if (buffer[i] == ':') break;
    digitsOfTitleLength++;
  }
  char titleLength[digitsOfTitleLength];
  bzero(titleLength, digitsOfTitleLength);
  read(clientfd, titleLength, digitsOfTitleLength);
  int titleLengthConv = atoi(titleLength);
  char title[titleLengthConv];
  title[titleLengthConv] = '\0';
  bzero(title, titleLengthConv);
  read(clientfd, &delim, 1);
  read(clientfd, title, titleLengthConv);
  DIR* directory = opendir("./");
  struct dirent* file;
  while ((file = readdir(directory))) {
    if (file->d_type == DT_DIR) {
      if (strcmp(file->d_name, title) == 0) {
        printf("Error: %s already exists\n", title);
        write(clientfd, "e", 1);
        return;
      }
    }
  }
  mkdir(title, S_IRWXU);
  char manifestName[strlen(title) + strlen(".manifest")];
  bzero(manifestName, strlen(title) + strlen(".manifest"));
  strcat(manifestName, title);
  strcat(manifestName, ".manifest");
  chdir(title);
  int manifest = open(manifestName, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  write(manifest, "0", 1);
  write(manifest, "\n", 1);
  chdir("..");
  printf("%s created successfully\n", title);
  write(clientfd, "s", 1);
  close(manifest);
  closedir(directory);
  return;
}

void destroy (int clientfd) {
  char buffer[100];
  char delim;
  int digitsOfTitleLength = 0;
  int i;
  recv(clientfd, buffer, sizeof(buffer), MSG_PEEK);
  for (i = 0; i < strlen(buffer); i++) {
    if (buffer[i] == ':') break;
    digitsOfTitleLength++;
  }
  char titleLength[digitsOfTitleLength];
  bzero(titleLength, digitsOfTitleLength);
  read(clientfd, titleLength, digitsOfTitleLength);
  int titleLengthConv = atoi(titleLength);
  char title[titleLengthConv];
  title[titleLengthConv] = '\0';
  bzero(title, titleLengthConv);
  read(clientfd, &delim, 1);
  read(clientfd, title, titleLengthConv);
  DIR* directory = opendir("./");
  struct dirent* file;
  int found = 0;
  while ((file = readdir(directory))) {
    if (file->d_type == DT_DIR) {
      if (strcmp(file->d_name, title) == 0) {
        found = 1;
        break;
      }
    }
  }
  if (found == 0) {
    printf("Error: %s could not be found\n", title);
    write(clientfd, "e", 1);
    return;
  }
  char toRemove[strlen("rm -r ") + strlen(title)];
  bzero(toRemove, strlen("rm -r ") + strlen(title));
  strcat(toRemove, "rm -r ");
  strcat(toRemove, title);
  system(toRemove);
  printf("%s destroyed successfully\n", title);
  write(clientfd, "s", 1);
  return;
}


void* request (void* client) {
  pthread_mutex_lock(&lock);
  int clientfd = *((int*) client);
  char request;
  int incoming = read(clientfd, &request, 1);
  //printf("request = %c\n", request);
  if (request == 'c') {
    create(clientfd);
  }
  if (request == 'd') {
    destroy(clientfd);
  }
  pthread_mutex_unlock(&lock);
  return client;
}

int main(int argc, char* argv[]) {

  int port = strtol(argv[1], NULL, 10);
  printf("port = %d\n", port);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("socket failed\n");
    return 0;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(sockfd, (struct sockaddr *) &address, sizeof(address)) != 0) {
    printf("bind failed, try a different port\n");
    return 0;
  }

  if (listen(sockfd, 5) < 0) {
    printf("listening failed\n");
    return 0;
  }


  while (1) {
    struct sockaddr_in client;
    int len = sizeof(client);
    int clientfd = accept(sockfd, (struct sockaddr*) &client, (socklen_t*) &len);
    if (clientfd < 0) {
      printf("accepting failed\n");
      return 0;
    }
    struct sockaddr_in* clientPointer = (struct sockaddr_in*) &client;
    struct in_addr ip = clientPointer->sin_addr;
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ip, clientIP, INET_ADDRSTRLEN);
    printf("\nCLIENT CONNECTED :: %s\n", clientIP);

    pthread_mutex_init(&lock, NULL);

    pthread_t thread;
    void* arg = &clientfd;
    pthread_create(&thread, NULL, &request, arg);
    pthread_join(thread, NULL);

    pthread_mutex_destroy(&lock);

    int cl = close(clientfd);
    if (cl == 0) printf("CLIENT DISCONNECTED :: %s\n\n", clientIP);
  }
  close(sockfd);
  return 0;
}

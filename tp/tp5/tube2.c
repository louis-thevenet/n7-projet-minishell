#include "stdio.h"
#include "unistd.h"
#include <string.h>

/*
Ici le fils et le père partagent le même tube, le fils pourra donc lire ce que
le père écrit.
*/
int main() {
  int tube[2];

  if (pipe(tube) == -1) {
    printf("Error creating pipe");
    return -1;
  }

  int n = 5;
  if (write(tube[1], &n, sizeof(n)) == -1) {
    printf("Error writing to pipe");
    return -1;
  }
  close(tube[1]);

  int pid_fork = fork();
  switch (pid_fork) {
  case -1:
    printf("Error forking");
    return -1;

  case 0:
    close(tube[1]);
    char buffer[64];
    int received;
    ssize_t bytes_read = read(tube[0], buffer, sizeof(buffer));
    printf("Bytes read: %ld\n", bytes_read);

    if (bytes_read > 0) {
      memcpy(&received, &buffer, sizeof(received));
      printf("Received: %d\n", received);
    }
    close(tube[0]);
    break;
  }
}
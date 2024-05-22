#include "stdio.h"
#include "unistd.h"
#include <string.h>

/*
Ici on crée le tube après le fork, donc le fils n'a pas le même tube, il ne peut
peut pas lire ce que le père envoie.
*/

int main() {
  int tube[2];

  int pid_fork = fork();
  if (pid_fork == -1) {
    printf("Error forking");
    return -1;
  }

  if (pipe(tube) == -1) {
    printf("Error creating pipe");
    return -1;
  }
  if (pid_fork == 0) {
    close(tube[1]);
    char buffer[64];
    int received;
    ssize_t bytes_read = read(tube[0], buffer, sizeof(buffer));
    printf("Bytes read: %ld", bytes_read);

    if (bytes_read > 0) {
      memcpy(&received, &buffer, sizeof(received));
      printf("Received: %d", received);
    }
    close(tube[0]);

  } else {
    close(tube[0]);
    int n = 5;
    if (write(tube[1], &n, sizeof(n)) == -1) {
      printf("Error writing to pipe");
      return -1;
    }
    close(tube[1]);
  }
}
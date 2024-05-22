#include "stdio.h"
#include "unistd.h"
#include <string.h>

/*
Dans cet exemple, l'appel à pause() est bloquant et le père ne se réveille pas,
même si le fils termine
Il faudrait plutôt utiliser wait avec le PID du fils.
*/
int child(int tube[2]) {
  char buffer[sizeof(int)];
  int received;
  ssize_t bytes_read;

  close(tube[1]);

  do {
    bytes_read = read(tube[0], buffer, sizeof(buffer));

    if (bytes_read == -1) {
      printf("Error while reading from pipe\n");
      return -1;
    }
    if (bytes_read > 0) {
      memcpy(&received, &buffer, sizeof(received));
      printf("Bytes read: %ld\n", bytes_read);
      printf("Received: %d\n\n", received);
    }
  } while (bytes_read > 0);
  printf("sortie de boucle");
  close(tube[0]);
  return 0;
}

int parent(int tube[2]) {
  close(tube[0]);

  for (int i = 0; i < 10; i++) {

    if (write(tube[1], &i, sizeof(i)) == -1) {
      printf("Error writing %d to pipe\n", i);
      return -1;
    }
  }
  close(tube[1]);
  printf("Finished writing to pipe\n");
  pause();
  return 0;
}

int main() {
  int tube[2];

  if (pipe(tube) == -1) {
    printf("Error creating pipe");
    return -1;
  }

  int pid_fork = fork();

  switch (pid_fork) {
  case -1:
    printf("Error forking");
    return -1;

  case 0:
    return child(tube);

  default:
    return parent(tube);
  }
}
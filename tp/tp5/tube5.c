#include "stdio.h"
#include "unistd.h"
#include <signal.h>
#include <string.h>
#define N 10
/*
Ici le père attend 10s avant de se terminer après avoir écrit dans le tube.
*/

void handler_sigusr1() {}

int child(int tube[2]) {
  struct sigaction sa_usr1;
  sa_usr1.sa_handler = handler_sigusr1;
  sigemptyset(&sa_usr1.sa_mask);
  sa_usr1.sa_flags = SA_RESTART;
  sigaction(SIGUSR1, &sa_usr1, NULL);

  char buffer[10 * N];
  ssize_t bytes_read;

  pause();

  close(tube[1]);
  bytes_read = read(tube[0], buffer, sizeof(buffer));

  if (bytes_read == -1) {
    printf("Error while reading from pipe\n");
    return -1;
  }
  if (bytes_read > 0) {
    printf("Bytes read: %ld\n", bytes_read);
  };
  close(tube[0]);
  return 0;
}

int parent(int tube[2]) {
  char tab[N];

  ssize_t bytes_written;
  close(tube[0]);
  while (1) {
    bytes_written = write(tube[1], &tab, sizeof(tab));
    if (bytes_written == -1) {
      printf("Error while writing to pipe\n");
      return -1;
    }
    sleep(1);
    printf("Bytes written: %ld\n",
           bytes_written); // pourquoi le faire après le sleep ?
  }
  close(tube[1]);
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
    printf("CHILD PID: %d\n", pid_fork);
    return parent(tube);
  }
}
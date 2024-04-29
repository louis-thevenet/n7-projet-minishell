#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSIZE 64

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <source> <destination>\n", argv[0]);
  }

  char buf[BUFSIZE];
  int src = open(argv[1], O_RDONLY);
  int dest = open(argv[2], O_WRONLY);

  if (src == -1) {
    printf("Error while oppening source file");
    exit(EXIT_FAILURE);
  }

  if (dest == -1) {
    dest = creat(argv[2], S_IRWXU);
    if (dest == -1) {
      printf("Error creating destination file");
    }
  }

  ssize_t bytesRead;
  ssize_t bytesWritten;
  do {
    bytesRead = read(src, buf, BUFSIZE);
    bytesWritten = write(dest, buf, bytesRead);
  } while (bytesRead != 0);

  close(src);
  close(dest);
}
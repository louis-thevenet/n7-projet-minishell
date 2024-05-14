#include "fcntl.h"
#include "readcmd.h"
#include "signal.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUFSIZE 64
#define DEBUG
#ifdef DEBUG
#define LIGHT_GRAY "\033[1;30m"
#define NC "\033[0m"
#define DEBUG_PRINT(x)                                                         \
  printf("%s", LIGHT_GRAY);                                                    \
  printf x;                                                                    \
  printf("%s", NC);
#else
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
  } while (0)
#endif

int fg_pid;
int fd_input_for_stdout;

/**
 * Handler for SIGCHLD signal.
 */
void handler_sig_child() {
  int status;
  int pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
  if (pid != -1) {
    if (WIFSTOPPED(status)) {
      DEBUG_PRINT(("[Child %d stopped]\n", pid));
    }
    if (WIFCONTINUED(status)) {
      DEBUG_PRINT(("[Child %d continued]\n", pid));
    }
    if (WIFSIGNALED(status)) {
      DEBUG_PRINT(("[Child %d signaled]\n", pid));
    }
    if (WIFEXITED(status)) {
      DEBUG_PRINT(("[Child %d exited]\n", pid));
    }

    if (fd_input_for_stdout > 0 && (WIFEXITED(status) || WIFSIGNALED(status))) {
      close(fd_input_for_stdout);
    }
    if (pid == fg_pid && !WIFCONTINUED(status)) {
      fg_pid = 0;
    }
  }
}

/**
 * Handler for SIGSTP signal. Stops the foreground process.
 */
void handler_sig_tstp() {
  DEBUG_PRINT(("[SIGTSTP received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Stopping %d]\n", fg_pid));
    kill(fg_pid, SIGSTOP);
    fg_pid = 0;
  }
}

/**
 * Handler for SIGINT signal. Kills the foreground process.
 */
void handler_sig_int() {
  DEBUG_PRINT(("[SIGINT received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Killing %d]\n", fg_pid));
    kill(fg_pid, SIGTERM);
    fg_pid = 0;
  }
}

/**
 * Copies the contents from one file descriptor to another then closes both.
 */
void redirect_pipe(int src, int dest) {
  ssize_t bytesRead;
  ssize_t bytesWritten;
  char buf[BUFSIZE];

  do {
    bytesRead = read(src, buf, BUFSIZE);
    bytesWritten = write(dest, buf, bytesRead);
    DEBUG_PRINT(("read %ld, wrote %ld\n", bytesRead, bytesWritten));
  } while (bytesRead > 0);
  DEBUG_PRINT(("Closing pipes\n"));
  close(src);
  close(dest);
}

/**
 * Executes a command in a child process. Handling pipes if needed. Waiting for
 * the child process to finish if it's a foreground process.
 */
void create_fork(char **cmd, struct cmdline *commande) {

  /********PIPES********/
  int fd_in[2];
  int fd_out[2];
  fd_input_for_stdout = -1;

  // is there an input file ?
  if (commande->in != NULL) {
    if (pipe(fd_in) == -1) {
      printf("Error: Cannot create pipe\n");
      return;
    }
  }

  // is there an output file ?
  if (commande->out != NULL) {
    if (pipe(fd_out) == -1) {
      printf("Error: Cannot create pipe\n");
      return;
    }
    fd_input_for_stdout = fd_out[1]; // global to be closed on SIGCHLD signal
  }

  /********FORK********/
  int pid_fork = fork();

  if (pid_fork == -1) {
    printf("La commande n'a pas fonctionné.");
  }

  if (pid_fork == 0) {
    /********CHILD********/

    if (commande->backgrounded != NULL) {
      setpgrp();
    }

    // read from pipe
    if (commande->in != NULL) {
      dup2(fd_in[0], STDIN_FILENO);
      close(fd_in[1]);
    }
    // write stdout to pipe
    if (commande->out != NULL) {
      dup2(fd_out[1], STDOUT_FILENO);
      close(fd_out[0]);
    }
    execvp(cmd[0], cmd);
    printf("La commande n'a pas fonctionné.\n");
    exit(EXIT_FAILURE);
  } else {
    /********PARENT********/

    // run in background ?
    if (commande->backgrounded == NULL) {
      fg_pid = pid_fork;
    }

    // is there an input file ?
    if (commande->in != NULL) {
      // source = input file
      int src = open(commande->in, O_RDONLY);
      if (src == -1) {
        printf("Le fichier d'entrée n'a pas pu être lu\n");
        return;
      }
      redirect_pipe(src, fd_in[1]);
    }

    // is there an output file ?
    if (commande->out != NULL) {
      // dest = output file
      int dest = open(commande->out, O_WRONLY | O_CREAT, S_IRWXU);
      if (dest == -1) {
        printf("Le fichier de sortie n'a pas pu être créé\n");
        return;
      }

      redirect_pipe(fd_out[0], dest);
    }

    // wait for a signal from the child process
    if (commande->backgrounded == NULL) {
      while (fg_pid != 0) {
        pause();
      }
    }
  }
}

/**
 * Sets up signal handling for SIGCHLD, SIGTSTP and SIGINT.
 */
void setup_sig_action() {
  struct sigaction sa_chld;
  sa_chld.sa_handler = handler_sig_child;
  sigemptyset(&sa_chld.sa_mask);
  sa_chld.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa_chld, NULL);

  struct sigaction sa_tstp;
  sa_tstp.sa_handler = handler_sig_tstp;
  sigemptyset(&sa_tstp.sa_mask);
  sa_tstp.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &sa_tstp, NULL);

  struct sigaction sa_int;
  sa_int.sa_handler = handler_sig_int;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = SA_RESTART;
  sigaction(SIGINT, &sa_int, NULL);
}

/**
 * Main loop
 */
int main(void) {
  setup_sig_action();

  bool fini = false;
  while (!fini) {
    printf("> ");
    struct cmdline *commande = readcmd();

    if (commande == NULL) {
      // commande == NULL -> erreur readcmd()
      perror("erreur lecture commande \n");

    } else {

      if (commande->err) {
        // commande->err != NULL -> commande->seq == NULL
        printf("erreur saisie de la commande : %s\n", commande->err);

      } else {

        int indexseq = 0;
        char **cmd;
        while ((cmd = commande->seq[indexseq])) {
          if (cmd[0]) {
            if (strcmp(cmd[0], "exit") == 0) {
              fini = true;
              printf("Au revoir ...\n");
            } else {
              create_fork(cmd, commande);
            }

            indexseq++;
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

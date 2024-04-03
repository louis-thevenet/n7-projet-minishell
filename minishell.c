#include "readcmd.h"
#include "signal.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void traitement(int sig) {
  int status;
  int pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
  if (pid != -1) {
    if (WIFSTOPPED(status)) {
      printf("Processus interrompu\n");
    }
    if (WIFCONTINUED(status)) {
      printf("Processus continué\n");
    }
    if (WIFEXITED(status)) {
      printf(" Le processus fils s'est terminé avec le code %i\n ",
             WEXITSTATUS(status));
    }
  }

  printf("Exit code : %d\nPID : %d\n", status, pid);
}

int create_fork(char **cmd, char *backgrounded) {
  int pid_fork = fork();
  if (pid_fork == -1) {
    printf("La commande n'a pas fonctionné.");
  }

  if (pid_fork == 0) {
    execvp(cmd[0], cmd);
    printf("La commande n'a pas fonctionné.\n");
    exit(EXIT_FAILURE);
  } else {

    int status;
    if (backgrounded == NULL) {
      pause();
    } else {
      waitpid(pid_fork, &status, WNOHANG | WUNTRACED | WCONTINUED);
    }
  }
  return pid_fork;
}

void setup_sig_action() {
  struct sigaction action;
  action.sa_handler = traitement;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &action, NULL);
}

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

        /* Pour le moment le programme ne fait qu'afficher les commandes
        tapees et les affiche à l'écran.
        Cette partie est à modifier pour considérer l'exécution de ces
        commandes
        */
        int indexseq = 0;
        char **cmd;
        while ((cmd = commande->seq[indexseq])) {
          if (cmd[0]) {
            if (strcmp(cmd[0], "exit") == 0) {
              fini = true;
              printf("Au revoir ...\n");
            } else {
              create_fork(cmd, commande->backgrounded);
            }

            indexseq++;
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

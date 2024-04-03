#include "readcmd.h"
#include "signal.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void traitement(int sig) { printf("%d\n", sig); }

int create_fork(char **cmd) {
  int pid_fork = fork();
  if (pid_fork == -1) {
    printf("La commande n'a pas fonctionné.");
  }

  if (pid_fork == 0) {
    execvp(cmd[0], cmd);
    exit(0);
  }
  return pid_fork;
}

void wait_if_backgrounded(int pid_fork, char *backgrounded) {
  if (backgrounded == NULL) {
    int status;
    waitpid(pid_fork, &status, 0);
  }
}

void setup_sig_action() {
  struct sigaction action;
  action.sa_handler = traitement;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
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
              int pid_fork = create_fork(cmd);
              wait_if_backgrounded(pid_fork, commande->backgrounded);
            }

            indexseq++;
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

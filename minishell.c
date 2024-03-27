#include "readcmd.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
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
              int pid_fork = fork();
              if (pid_fork == -1) {
                printf("La commande n'a pas fonctionné.");
              }

              if (pid_fork == 0) {
                execvp(cmd[0], &cmd[0]);
              }
              if (commande->backgrounded == NULL) {
                int status;
                waitpid(pid_fork, &status, 0);
              }
            }

            indexseq++;
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

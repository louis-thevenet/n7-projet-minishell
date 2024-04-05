#include "job.h"
#include "readcmd.h"
#include "signal.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
int bg_pid;
job *jobs;

void traitement(int sig) {
  int status;
  int pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
  if (pid != -1) {
    if (WIFSTOPPED(status)) {
      printf("[Processus %d interrompu]\n", pid);
    }
    if (WIFCONTINUED(status)) {
      printf("[Processus %d continué]\n", pid);
    }
    if (WIFEXITED(status)) {
      printf("[Processus %d terminé avec le code %d]\n", pid,
             WEXITSTATUS(status));
      rm_job_pid(jobs, pid);
    }

    if (pid == bg_pid && WIFEXITED(status)) {
      bg_pid = 0;
    }
  }
}

void traiter_commande(char **cmd, char *backgrounded) {

  if (strcmp(cmd[0], "lj") == 0) {
    print_jobs(jobs);
    return;
  }

  if (strcmp(cmd[0], "sj") == 0) {
    int id = atoi(cmd[1]);
    stop_job_pid(jobs, id);
    return;
  }

  if (strcmp(cmd[0], "bg") == 0) {
    int id = atoi(cmd[1]);
    continue_job_bg_id(jobs, id);
    return;
  }
  if (strcmp(cmd[0], "fg") == 0) {
    int id = atoi(cmd[1]);
    wait_job_id(jobs, id);

    return;
  }

  int pid_fork = fork();

  if (pid_fork == -1) {
    printf("La commande n'a pas fonctionné.");
  }

  if (pid_fork == 0) {
    execvp(cmd[0], cmd);
    printf("La commande n'a pas fonctionné.\n");
    exit(EXIT_FAILURE);

  } else {
    char *cmd_copy = build_command_string(cmd);
    add_job(jobs, (job){pid_fork, ACTIVE, cmd_copy});

    if (backgrounded == NULL) {
      bg_pid = pid_fork;
      while (bg_pid != 0) {
        pause();
      }
    }
  }
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
  jobs = malloc(sizeof(job) * NB_JOBS_MAX);
  init_jobs(jobs);
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
              traiter_commande(cmd, commande->backgrounded);
            }

            indexseq++;
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

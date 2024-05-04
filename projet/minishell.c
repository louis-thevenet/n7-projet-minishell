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
job *jobs;

void handler_sig_child() {
  int status;
  int pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
  if (pid != -1) {
    if (WIFSTOPPED(status)) {
      DEBUG_PRINT(("[Child %d stopped]\n", pid));
      update_status_pid(SUSPENDED, jobs, pid);
    }
    if (WIFCONTINUED(status)) {
      DEBUG_PRINT(("[Child %d continued]\n", pid));
      update_status_pid(ACTIVE, jobs, pid);
    }
    if (WIFSIGNALED(status)) {
      DEBUG_PRINT(("[Child %d signaled]\n", pid));
      rm_job_pid(jobs, pid);
    }
    if (WIFEXITED(status)) {
      DEBUG_PRINT(("[Child %d exited]\n", pid));
      rm_job_pid(jobs, pid);
    }

    if (pid == fg_pid && !WIFCONTINUED(status)) {
      fg_pid = 0;
    }
  }
}

void handler_sig_tstp() {
  DEBUG_PRINT(("[SIGTSTP received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Stopping %d]\n", fg_pid));

    send_stop_job_pid(jobs, fg_pid);
    fg_pid = 0;
  }
}
void handler_sig_int() {
  DEBUG_PRINT(("[SIGINT received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Killing %d]\n", fg_pid));
    kill(fg_pid, SIGTERM);
    fg_pid = 0;
  }
}

void traiter_commande(char **cmd, char *backgrounded) {

  if (strcmp(cmd[0], "lj") == 0) {
    print_jobs(jobs);
    return;
  }

  if (strcmp(cmd[0], "sj") == 0) {
    int id = atoi(cmd[1]);
    send_stop_job_id(jobs, id);
    return;
  }

  if (strcmp(cmd[0], "bg") == 0) {
    int id = atoi(cmd[1]);
    continue_job_bg_id(jobs, id);
    return;
  }
  if (strcmp(cmd[0], "fg") == 0) {
    int id = atoi(cmd[1]);
    fg_pid = wait_job_id(jobs, id);
    while (fg_pid != 0) {
      pause();
    }
    return;
  }

  if (strcmp(cmd[0], "susp") == 0) {
    raise(SIGSTOP);
    return;
  }

  int pid_fork = fork();

  if (pid_fork == -1) {
    printf("La commande n'a pas fonctionné.");
  }

  if (pid_fork == 0) {

    sigset_t mask_set;
    sigemptyset(&mask_set);
    sigaddset(&mask_set, SIGINT);
    sigaddset(&mask_set, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask_set, NULL);

    execvp(cmd[0], cmd);
    printf("La commande n'a pas fonctionné.\n");
    exit(EXIT_FAILURE);

  } else {
    char *cmd_copy = build_command_string(cmd);
    add_job(jobs, (job){pid_fork, ACTIVE, cmd_copy});

    if (backgrounded == NULL) {
      fg_pid = pid_fork;
      while (fg_pid != 0) {
        pause();
      }
    }
  }
}

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

int main(void) {
  setup_sig_action();
  jobs = malloc(sizeof(job) * NB_JOBS_MAX);
  init_jobs(jobs);
  bool fini = false;
  struct cmdline *commande;
  while (!fini) {
    printf("> ");
    commande = readcmd();

    if (commande == NULL) {
      perror("erreur lecture commande \n");

    } else {
      if (commande->err) {
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
              traiter_commande(cmd, commande->backgrounded);
            }

            indexseq++;
          }
        }
      }
    }
  }
  freecmd(commande);
  free(commande);
  free(jobs);
  return EXIT_SUCCESS;
}

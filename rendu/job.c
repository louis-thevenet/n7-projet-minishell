#include "job.h"
#include "sys/wait.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_jobs(job jobs[]) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    jobs[i].pid = -1;
    jobs[i].state = FINISHED;
    jobs[i].command = NULL;
  }
}

int add_job(job *jobs, job new_job) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    if (jobs[i].state == FINISHED) {
      jobs[i] = new_job;
      return i;
    }
  }
  fprintf(stderr, "Too many jobs\n");
  return -1;
}
void rm_job_pid(job *jobs, int pid) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    if (jobs[i].pid == pid) {
      free(jobs[i].command);
      jobs[i].pid = -1;
      jobs[i].state = FINISHED;
    }
  }
}
void continue_job_bg_id(job *jobs, int id) {
  if (jobs[id].pid == -1 || id >= NB_JOBS_MAX) {
    fprintf(stderr, "No such job\n");
    return;
  }

  jobs[id].state = ACTIVE;
  kill(jobs[id].pid, SIGCONT);
}

void stop_job_pid(job *jobs, int id) {
  if (jobs[id].pid == -1 || id >= NB_JOBS_MAX) {
    fprintf(stderr, "No such job\n");
    return;
  }

  jobs[id].state = SUSPENDED;
  kill(jobs[id].pid, SIGSTOP);
}
char *build_command_string(char **cmd) {
  int size = 1;
  for (int i = 0; cmd[i] != NULL; i++) {
    size += strlen(cmd[i]) + 1;
  }
  char *command = malloc(size * sizeof(char));
  command[0] = '\0';
  for (int i = 0; cmd[i] != NULL; i++) {
    strcat(command, cmd[i]);
    strcat(command, " ");
  }
  return command;
}
void wait_job_id(job *jobs, int id) {
  if (jobs[id].pid == -1 || id >= NB_JOBS_MAX) {
    fprintf(stderr, "No such job\n");
    return;
  }

  jobs[id].state = ACTIVE;
  kill(jobs[id].pid, SIGCONT);
  waitpid(jobs[id].pid, NULL, 0);
}

void print_jobs(job *jobs) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    if (jobs[i].pid != -1) {
      printf("Job %d\n", i);
      printf("\tPID: %d\n", jobs[i].pid);
      printf("\tState: ");
      switch (jobs[i].state) {
      case ACTIVE:
        printf("Active");
        break;

      case SUSPENDED:
        printf("Suspended");
        break;

      case FINISHED:
        printf("Finished");
        break;
      default:
        printf("Unknown state");
        break;
      }
      printf("\tCommand: %s\n", jobs[i].command);
    }
  }
}

#include "job.h"
#include <stdio.h>
#include <stdlib.h>

void init_jobs(job jobs[]) {
    for (int i = 0; i < NB_JOBS_MAX; i++) {
        jobs[i].pid = -1;
        jobs[i].state = FINISHED;
        jobs[i].command = NULL;
    }
}

int add_job(job jobs[], job new_job) {
    for (int i = 0; i < NB_JOBS_MAX; i++) {
        if (jobs[i].state == FINISHED) {
            jobs[i]=new_job;
            return i;
    }
    }
    fprintf(stderr, "Too many jobs\n");
    return -1;
}
int rm_job_pid(job jobs[], int pid){
    for (int i = 0; i < NB_JOBS_MAX; i++) {
        if (jobs[i].pid == pid) {
            free(jobs[i].command);
            jobs[i].pid = -1;
            jobs[i].state = FINISHED;
        }
    }
}
void print_jobs(job jobs[]) {
    for (int i = 0; i < NB_JOBS_MAX; i++) {
        if (jobs[i].pid != -1) {
        printf("Job %d", i);
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
    }}
}

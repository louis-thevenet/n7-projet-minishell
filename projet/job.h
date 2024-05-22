#define NB_JOBS_MAX 50

// allowed job states
typedef enum job_state { ACTIVE, SUSPENDED, FINISHED } job_state;

// job structure
typedef struct job {
  int pid;         // associated process id
  int fd_pipe_out; // to redirect stdout
  job_state state; // job state
  char *command;   // associated command
} job;

/**
 * Initialize the jobs array.
 */
void init_jobs(job *jobs);

/**
 * Add a new job to the jobs array.
 */
int add_job(job *, job new_job);

/**
 * Remove a job from the jobs array using its process id.
 */
void rm_job_pid(job *jobs, int pid);

/**
 * Send a SIGSTOP signal to a job using its process id.
 */
void send_stop_job_pid(job *jobs, int pid);

/**
 * Send a SIGSTOP signal to a job using its internal id.
 */
void send_stop_job_id(job *jobs, int id);

/**
 * Send a SIGCONT signal to a job using its internal id.
 */
void continue_job_bg_id(job *jobs, int id);

/**
 * Send a SIGCONT signal to a job using its internal id and return its process
 * id.
 */
int continue_job_fg_id(job *jobs, int id);

/**
 * Set the state of a job using its internal id.
 */
void update_status_id(int state, job *jobs, int id);

/**
 * Set the state of a job using its process id.
 */
void update_status_pid(int state, job *jobs, int pid);

/**
 * Build a command string from an array of strings.
 */
char *build_command_string(char **cmd);

/**
 * Get a job's internal id from its process id.
 */
int id_from_pid(job *jobs, int pid);

/**
 * Print the jobs array.
 */
void print_jobs(job *jobs);

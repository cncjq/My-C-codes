#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <csse2310a3.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

// Userful constants
#define MIN_ARG_LEN 2
#define MAX_ARG_LEN 5
#define CMD_CONST 4
#define FD_CONST 2
#define READ_END 0
#define WRITE_END 1
#define SLEEP_ARGC 2
#define SIGNAL_ARGC 3
#define BASE_TEN 10
#define MAX_SIGNUM 31
#define SECOND 1000000
#define MILLISEC 1000

// Program exit values
enum ExitCode {
    USAGE_ERROR = 1,
    READ_JOBFILE_ERROR = 2,
    READ_INPUTFILE_ERROR = 3,
    EXEC_FAIL = 99
};

// The possible mode
enum JobthingMode {
    NORMAL = 0,
    VERBOSE
};

// Field numbers split from job line argument
enum SplitFields {
    RESTART_FIELD = 0,
    INPUT_FIELD = 1,
    OUTPUT_FIELD = 2,
    CMD_FIELD = 3
};

enum InputCommand {
    CMD = 0,
    JOBID = 1,
    SIGNUM = 2,
    SLEEP_SEC = 1
};

// Structure type to hold the program parameters..
// The jobthingMode is default unless "-v" is specified.
// The inputFile is the parameter after "-i" argument.
// The seeParamI specifies if there is a "-i" paramter.
// The seeParamV specifies if there is a "-v" parameter.
typedef struct {
    enum JobthingMode jobthingMode;
    char* inputFile;
    char* jobFile;
    bool seeParamI;
    bool seeParamV;
} Parameters;

// Structrue type to hold information needed in each job.
// command is the command part of a job line.
// numrestarts is the first field in a job line, stands for restart times.
// input is the second field in a job line, stands for job input.
// output is the third field in a job line, stands for job output.
// running states if the job is alive or not.
// jobNum is the order number of this job.
// childid is the process id of this job.
// fdInput is the file descriptor array to be used to handle input.
// fdOutput is the file descriptor array to be used to handle output.
// countRun is the number counts how many times this job has been started.
// runnable shows if this job is runnable or not.
// readFromJob is a file pointer pointing to the reading end of output pipe.
typedef struct {
    char* command;
    int numrestarts;
    char* input;
    char* output;
    bool running;
    int jobNum;
    pid_t childid;
    int fdInput[2];
    int fdOutput[2];
    int countRun;
    bool runnable;
    FILE* readFromJob;
    FILE* writeToJob;
    int linesto;
} Job;

// Structure type to hold information needed in input command.
// command is the command part of input.
// jobID is the job ID that the command sends to.
// sigNum is the signal number that the command sends to.
// millisec is the milli-sleep second after "*sleep" command.
// valid check if the command is valid.
typedef struct {
    char* command;
    int jobID;
    int sigNum;
    int millisec;
    bool valid;
} InputCmd;

// Structure type to hold information needed to handle signal.
// jobs is the job address it points to.
// jobNum is the jobNum address it points to.
typedef struct {
    Job*** jobs;
    int* jobNum;
} SigHandler;

void free_jobs(Job** jobs, int jobNum); // function prototype

SigHandler* sigHandler; // global variable to handle signal.

/* usage_error()
 * _____________
 * This function handles usage error. It will print out error message and exit
 * with correponding exit code.
 */ 
void usage_error(void) {
    fprintf(stderr, "Usage: jobthing [-v] [-i inputfile] jobfile\n");
    exit(USAGE_ERROR);
}

/* invalid_job_spec()
 * __________________
 * This function handles invalid job specification.
 * It will print error message to stderr if the programme is in vobose mode.
 */ 
void invalid_job_spec(char* jobline, bool verboseMode) {
    if (verboseMode) {
	fprintf(stderr, "Error: invalid job specification: %s\n", jobline);
    }
}

/* parse_command_line()
 * ____________________
 * This function parses command line and return Parameters struct.
 *
 * argc: The number of arguments of command line.
 * argv: the array of command line arguments.
 *
 * Returns: It will return a Parameters struct.
 * Errors: It will occur errors if the command line arguments are invalid.
 */
Parameters parse_command_line(int argc, char** argv) {
    if (argc < MIN_ARG_LEN || argc > MAX_ARG_LEN) {
	usage_error();
    }
    bool seeJobFile = false;
    Parameters param;
    memset(&param, 0, sizeof(Parameters));
    param.seeParamI = false;
    param.seeParamV = false;
    for (int i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-v")) {
	    if (param.seeParamV) {
		usage_error();
	    }
	    param.jobthingMode = VERBOSE;
	    param.seeParamV = true;
	} else if (!strcmp(argv[i], "-i")) {
	    if (i == argc - 1 || param.seeParamI) {
		usage_error();
	    }
	    param.inputFile = argv[++i];
	    param.seeParamI = true;
	} else {
	    if (seeJobFile == true) {
		usage_error();
	    }
	    param.jobFile = argv[i];
	    seeJobFile = true;
	}
    }
    if (seeJobFile == false) {
	usage_error();
    }
    return param;
}

/* check_jobfile()
 * _______________
 * This function check each job line and returns valid job.
 *
 * jobLine: Each job line read from job file.
 * verboseMode: If this programme is in verbose mode.
 *
 * Returns: It will return a valid job read from job file.
 */
Job* check_jobfile(char* jobLine, bool verboseMode) {
    if (jobLine[0] == '#' || strlen(jobLine) == 0) {
	return NULL;
    } 
    char** splitLine;
    int i = 0;
    char* cpLine = strdup(jobLine);
    char* end;
    splitLine = split_line(cpLine, ':');
    while (splitLine[i]) {
	i++;
    }
    if (i != CMD_CONST) {
	invalid_job_spec(jobLine, verboseMode);
	free(cpLine);
	return NULL;
    }
    if (strtol(splitLine[RESTART_FIELD], &end, BASE_TEN) < 0) {
	invalid_job_spec(jobLine, verboseMode);
	free(cpLine);
	return NULL;
    }
    if (splitLine[CMD_FIELD][0] == ' ' || 
	    strlen(splitLine[CMD_FIELD]) == 0) {
	invalid_job_spec(jobLine, verboseMode);
	free(cpLine);
	return NULL;
    }
    Job* job = malloc(sizeof(Job));
    if (strlen(splitLine[RESTART_FIELD]) == 0) {
	job->numrestarts = 0;
    } else {
	job->numrestarts = strtol(splitLine[RESTART_FIELD], &end, BASE_TEN);
    }
    job->input = splitLine[INPUT_FIELD];
    job->output = splitLine[OUTPUT_FIELD];
    job->command = splitLine[CMD_FIELD];
    free(splitLine);
    return job;
}

/* remove_quotes()
 * _______________
 * This function removes double quotes from a command field.
 *
 * command: The command needs to remove double quotes.
 *
 * return: It returns a new string of command line.
 */ 
char* remove_quotes(char* command) {
    char** splitCmd;
    char* cmd = strdup(command);
    splitCmd = split_line(cmd, '\"');
    char* newString;
    int partNum = 0;
    newString = strdup(splitCmd[partNum]);
    while (splitCmd[partNum + 1]) {
	newString = realloc(newString, sizeof(char) * (strlen(newString) 
		+ strlen(splitCmd[partNum + 1])));
	if (strlen(splitCmd[partNum + 1])) {
	    strcat(newString, splitCmd[partNum + 1]);
	}
	partNum++;
    }
    int newStrlen = strlen(newString);
    if (newString[newStrlen - 1] == ' ') {
	newString[newStrlen - 1] = '\0';
    }
    return newString;
}

/* read_file()
 * ___________
 * This function read input file and job file.
 *
 * params: The parameters structure of the program argumens.
 * JobNum: The total number of valid jobs.
 *
 * Returns: It will return a array of struct pointer storing all valid jobs.
 * Errors: It will occur errors if the input file or job file cannot be read.
 */
Job** read_file(Parameters params, int* jobNum) {
    Job** jobs;
    jobs = malloc(sizeof(Job*));
    Job* validJob;
    char* line;
    FILE* fpInput;
    FILE* fpJobFile;
    if (params.seeParamI == true) {
	fpInput = fopen(params.inputFile, "r");
	if (fpInput == NULL) {
	    fprintf(stderr, "Error: Unable to read input file\n");
	    free(jobs);
	    exit(READ_INPUTFILE_ERROR);
	}
	fclose(fpInput);
    }
    fpJobFile = fopen(params.jobFile, "r");
    if (fpJobFile == NULL) {
	fprintf(stderr, "Error: Unable to read job file\n");
	free(jobs);
	exit(READ_JOBFILE_ERROR);
    }
    while ((line = read_line(fpJobFile))) {
	jobs = realloc(jobs, sizeof(Job*) * (*jobNum + 1));
	if ((validJob = check_jobfile(line, params.seeParamV))) {
	    jobs[*jobNum] = validJob;
	    jobs[*jobNum]->jobNum = *jobNum + 1;
	    if (params.seeParamV) {
		char* cmd;
		cmd = remove_quotes(jobs[*jobNum]->command);
		printf("Registering worker %d: %s\n", jobs[*jobNum]->jobNum,
			cmd);
		fflush(stdout);
		free(cmd);
	    }
	    *jobNum = (*jobNum) + 1;
	    free(line);
	}	
    }
    fclose(fpJobFile);
    return jobs;
}

/* creating_pipe()
 * _______________
 * This function creates pipe.
 *
 * fd: The array of file descriptor to create a pipe.
 *
 * Error: It will occur errors and print error message when creating fails.
 */
void creating_pipe(int* fd) {
    if (pipe(fd) < 0) {
	perror("Creating pipe");
	exit(1);
    }
}

/* split_command()
 * _______________
 * This function split command read from each job line.
 *
 * job: The job that this function split from.
 *
 * Returns: It will return an array of strings, which is the command array.
 */
char** split_command(Job* job) {
    char** splitCmd;
    char* fullCmd = strdup(job->command);
    int numtok;
    splitCmd = split_space_not_quote(fullCmd, &numtok);
    return splitCmd;
}

/* jobthing_pipe_handle()
 * ______________________
 * This function handles pipe ends of jobthing.
 * It closes the unused ends if necessary.
 */
void jobthing_pipe_handle(Job* job) {
    if (!strlen(job->input)) {
	close(job->fdInput[READ_END]);
	job->writeToJob = fdopen(job->fdInput[WRITE_END], "w"); 
    }
    if (!strlen(job->output)) {
	close(job->fdOutput[WRITE_END]);
	job->readFromJob = fdopen(job->fdOutput[READ_END], "r"); 
    }
}

/* job_pipe_handle()
 * _________________
 * This function handles pipe ends of each job.
 * It redirects each end and closes unused ends if necessary.
 */
void job_pipe_handle(Job* job) {
    if (!strlen(job->input)) {
	close(job->fdInput[WRITE_END]);
	dup2(job->fdInput[READ_END], STDIN_FILENO);
	close(job->fdInput[READ_END]);
    } else {
	dup2(job->fdInput[READ_END], STDIN_FILENO);
	close(job->fdInput[READ_END]);
    }
    if (!strlen(job->output)) {
	close(job->fdOutput[READ_END]);
	dup2(job->fdOutput[WRITE_END], STDOUT_FILENO);
	close(job->fdOutput[WRITE_END]);
    } else {
	dup2(job->fdOutput[WRITE_END], STDOUT_FILENO);
	close(job->fdOutput[WRITE_END]);
    }
}

/* clean_pipe()
 * ____________
 * This function clean unused pipes.
 *
 * jobs: All valid jobs.
 * jobNum: The total number of valid jobs.
 */
void clean_pipe(Job** jobs, int jobNum) {
    if (!strlen(jobs[jobNum]->input) && jobs[jobNum]->writeToJob) {
	fclose(jobs[jobNum]->writeToJob);	
	jobs[jobNum]->writeToJob = NULL;
    }
    if (!strlen(jobs[jobNum]->output) && jobs[jobNum]->readFromJob) {
	fclose(jobs[jobNum]->readFromJob);
	jobs[jobNum]->readFromJob = NULL;
    }
}

/* check_job_input()
 * _________________
 * This function checks if the input of job is valid.
 * It will create a pipe is no input is specified.
 *
 * Errors: It will print error message to stderr if the input cannot be opened,
 * and make the job unnable.
 */
void check_job_input(Job* job) {
    if (strlen(job->input)) {
	job->fdInput[READ_END] = open(job->input, O_RDONLY);
	if (job->fdInput[READ_END] == -1) {
	    job->runnable = false;
	    fprintf(stderr, "Error: unable to open \"%s\" for reading\n",
		    job->input);
	}
    } else {
	creating_pipe(job->fdInput);
    }
}

/* check_job_output()
 * __________________
 * This function checks if the output of job is valid.
 * It will create a pipe if no output is specified.
 *
 * Errors: It will print error message to stderr if the output file cannot be 
 * opened, and make the job runnable.
 */
void check_job_output(Job* job) {
    if (strlen(job->output)) {
	job->fdOutput[WRITE_END] = open(job->output, O_WRONLY | O_CREAT 
		| O_TRUNC, S_IWUSR | S_IRUSR);
	if (job->fdOutput[WRITE_END] == -1) {
	    job->runnable = false;
	    job->running = false;
	    fprintf(stderr, "Error: unable to open \"%s\" for writing\n",
		    job->output);
	}	 
    } else {
	creating_pipe(job->fdOutput);
    }
}

/* job_startup()
 * _____________
 * This function handles the starts a job.
 *
 * job: The job to be started.
 * jobs: All valid jobs.
 * jobNum: The total number of valid jobs.
 * params: The parameters read from programme arguments.
 *
 * Returns: It will return the job after trying to start the job.
 */
Job* job_startup(Job* job, Job** jobs, int jobNum, Parameters params) {
    int cleanNum;
    pid_t childid;
    job->runnable = true;
    job->running = true;
    check_job_input(job); //runs pipe()
    if (!job->runnable) {
	return job;
    }
    check_job_output(job);
    if (!job->runnable) {
	return job;
    }
    if ((childid = fork())) {
	job->childid = childid;
	jobthing_pipe_handle(job);
	if (params.seeParamV) { 
	    if (!job->countRun) {
		fprintf(stdout, "Spawning worker %d\n", job->jobNum);
		fflush(stdout);
	    } else {
		fprintf(stdout, "Restarting worker %d\n", job->jobNum);
		fflush(stdout);
	    }
	}
    } else {
	if (job->countRun) {
	    cleanNum = jobNum;
	} else {
	    cleanNum = job->jobNum;
	}
	for (int i = 0; i < cleanNum; i++) {
	    if (jobs[i]->runnable) {
		clean_pipe(jobs, i);
	    }
	}
	job_pipe_handle(job);
	char** splitCmd = split_command(job);
	execvp(splitCmd[0], splitCmd);
	_exit(EXEC_FAIL);
    }
    return job;
}

/* restart_worker()
 * ________________
 * This function restart a not running job.
 *
 * jobs: All valid jobs.
 * jobNum: The total number of all valid jobs.
 * params: The parameters read from programme arguments.
 *
 */
void restart_worker(Job** jobs, int jobNum, Parameters params) {
    for (int i = 0; i < jobNum; i++) {
	if (jobs[i]->runnable && !jobs[i]->running) {
	    if (jobs[i]->numrestarts > jobs[i]->countRun || 
		    !jobs[i]->numrestarts) {
		jobs[i] = job_startup(jobs[i], jobs, jobNum, params);
		jobs[i]->countRun++;
	    } else {
		jobs[i]->runnable = false;
		continue;
	    }
	} else {
	    continue;
	}
    }
}

/* check_alive()
 * _____________
 * This function checks if a job has dead. It will print the exit status or
 * signal if a job is dead.
 *
 * jobs: All valid jobs.
 * jobNum: The total number of all valid jobs.
 */
void check_alive(Job** jobs, int jobNum) {
    int status;
    for (int i = 0; i < jobNum; i++) {
	if (!jobs[i]->runnable) {
	    continue;
	}
	pid_t pid = waitpid(jobs[i]->childid, &status, WNOHANG);
	if (!pid) {
	    continue;
	}
	if (WIFEXITED(status)) {
	    printf("Job %d has terminated with exit code %d\n", 
		    jobs[i]->jobNum, WEXITSTATUS(status));
	    fflush(stdout);
	    clean_pipe(jobs, jobs[i]->jobNum - 1);
	    jobs[i]->running = false;
	} else if (WIFSIGNALED(status)) {
	    printf("Job %d has terminated due to signal %d\n",
		    jobs[i]->jobNum, WTERMSIG(status));
	    fflush(stdout);
	    clean_pipe(jobs, jobs[i]->jobNum - 1);
	    jobs[i]->running = false;
	}
    }
}

/* send_to_job()
 * _____________
 * This function sends data to the job if there is connected pipe. It will 
 * print the data to stdout if there is a pipe.
 *
 * line: The line to be sent.
 * jobs: All valid jobs.
 * jobNum: The total number of all valid jobs.
 */
void send_to_job(char* line, Job** jobs, int jobNum) {
    for (int i = 0; i < jobNum; i++) {
	if (jobs[i]->running && !strlen(jobs[i]->input)) {
	    fprintf(jobs[i]->writeToJob, "%s\n", line);
	    fflush(jobs[i]->writeToJob);
	    jobs[i]->linesto++;
	    printf("%d<-\'%s\'\n", jobs[i]->jobNum, line);
	    fflush(stdout);
	}
    }    
}

/* read_from_job()
 * _______________
 * This function reads data from the job if there is a connected pipe. It will
 * print the data to stdout if there is a pipe.
 *
 * jobs: All valid jobs.
 * jobNum: The total number of all valid jobs.
 * params: The parameters read from programme arguments.
 */ 
void read_from_job(Job** jobs, int jobNum, Parameters params) {
    char* recv;
    for (int i = 0; i < jobNum; i++) {
	if (jobs[i]->running && !strlen(jobs[i]->output)) {
	    recv = read_line(jobs[i]->readFromJob);
	    if (!recv) {
		if (params.seeParamV) {
		    fprintf(stderr, "Received EOF from job %d\n",
			    jobs[i]->jobNum); 
		}
	    } else {
		printf("%d->\'%s\'\n", jobs[i]->jobNum, recv);
		fflush(stdout);
	    }
	}
    }
}

/* check_command()
 * _______________
 * This function checks the jobthing command, the input starts with "*".
 *
 * line: The command line from input.
 * valid: If this command is valid or not.
 * jobs: All valid jobs.
 * jobNum: The total number of valid jobs.
 *
 * Errors: It will print out error message to stdout when a command is ivalid.
 */ 
void process_command(char* line, Job** jobs, int jobNum) {
    char** splitCmd;
    char* splitLine;
    int numtoks;
    char* end;
    InputCmd cmd;
    splitLine = strdup(line);
    splitCmd = split_space_not_quote(splitLine, &numtoks);
    cmd.valid = true;
    if (strcmp(splitCmd[CMD], "*sleep") && strcmp(splitCmd[CMD], "*signal")) {
	printf("Error: Bad command \'%s\'\n", splitCmd[0]);
	cmd.valid = false;
    } else {
	if (!strcmp(splitCmd[0], "*sleep")) {
	    if (numtoks != SLEEP_ARGC) {
		printf("Error: Incorrect number of arguments\n");
		cmd.valid = false;
	    } else {
		cmd.millisec = strtol(splitCmd[SLEEP_SEC], &end, BASE_TEN);
		if (strcmp(end, "") || cmd.millisec < 0) { 
		    printf("Error: Invalid duration\n");
		    cmd.valid = false;
		}
	    }
	    if (cmd.valid) {
		usleep(cmd.millisec * MILLISEC);
	    }
	} else if (!strcmp(splitCmd[0], "*signal")) {
	    if (numtoks != SIGNAL_ARGC) {
		printf("Error: Incorrect number of arguments\n");
		cmd.valid = false;
	    } else {
		cmd.jobID = strtol(splitCmd[JOBID], &end, BASE_TEN); 
		if (strcmp(end, "") || cmd.jobID < 1 || cmd.jobID > jobNum 
			|| !jobs[cmd.jobID - 1]->runnable) {
		    printf("Error: Invalid job\n");
		    cmd.valid = false;
		}
		cmd.sigNum = strtol(splitCmd[SIGNUM], &end, BASE_TEN);
		if (strcmp(end, "") || cmd.sigNum < 1 || 
			cmd.sigNum > MAX_SIGNUM) {
		    printf("Error: Invalid signal\n");
		    cmd.valid = false;
		}
		if (cmd.valid) {
		    kill(jobs[cmd.jobID - 1]->childid, cmd.sigNum);
		}
	    }
	}
    }
}

/* jobthing_operation()
 * ____________________
 * This is the funcition handles the jobthing operation and command formate.
 * It will start a job if needed and then send or receive data from jobs.
 *
 * params: The parameters read from programme arguments.
 * jobs: All valid jobs.
 * jobNum: The total number of all valid jobs.
 */ 
void jobthing_operation(Parameters params, Job** jobs, int jobNum) {
    char* line;
    FILE* input = (params.seeParamI) ? fopen(params.inputFile, "r") : stdin; 
    while (1) {
	check_alive(jobs, jobNum);
	restart_worker(jobs, jobNum, params);
	int unrunnableJob = 0;
	for (int i = 0; i < jobNum; i++) {
	    if (!jobs[i]->runnable) {
		unrunnableJob++;
	    }
	}
	if (unrunnableJob == jobNum) {
	    fprintf(stderr, "No more viable workers, exiting\n");
	    break;
	}
	line = read_line(input);
	if (!line) {
	    for (int i = 0; i < jobNum; i++) {
		clean_pipe(jobs, i);
	    }
	    exit(0);
	}
	if (line[0] == '*') {
	    process_command(line, jobs, jobNum);
	    usleep(SECOND);
	    continue;
	} 
	send_to_job(line, jobs, jobNum);
	usleep(SECOND);
	read_from_job(jobs, jobNum, params);
	free(line);
    }
    fclose(input);
}

/* free_jobs()
 * ___________
 * This function frees allocated memories.
 *
 * jobs: All valid jobs.
 * JobNum: The total number of valid jobs.
 */ 
void free_jobs(Job** jobs, int jobNum) {
    for (int i = 0; i < jobNum; i++) {
	free(jobs[i]);
    }
    free(jobs);
}

/* stat_report();
 * ______________
 * This function is handling when signal received.
 *
 * s: Signal number.
 */
void stat_report(int s) {
    for (int i = 0; i < *sigHandler->jobNum; i++) {
	fprintf(stderr, "%d:%d:%d\n", sigHandler->jobs[0][i]->jobNum,
		sigHandler->jobs[0][i]->numrestarts, 
		sigHandler->jobs[0][i]->linesto);
	fflush(stderr);
    }
}

int main(int argc, char** argv) {
    Job** jobs;
    int jobNum = 0;
    sigHandler = malloc(sizeof(SigHandler));
    sigHandler->jobs = &jobs;
    sigHandler->jobNum = &jobNum;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = stat_report;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, 0);
    Parameters params = parse_command_line(argc, argv);
    jobs = read_file(params, &jobNum);
    for (int i = 0; i < jobNum; i++) {
	jobs[i]->countRun = 0;
	jobs[i] = job_startup(jobs[i], jobs, i, params);
	jobs[i]->countRun = 1;
	jobs[i]->linesto = 0;
    }
    usleep(SECOND);
    jobthing_operation(params, jobs, jobNum);
    free_jobs(jobs, jobNum);
    return 0;
}

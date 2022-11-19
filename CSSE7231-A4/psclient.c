#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <csse2310a3.h>
#include <unistd.h>
#include <pthread.h>

// Useful constants
#define MIN_ARGS 2
#define TOPICS_START 3

// Program exit values
enum ExitCode {
    USAGE_ERR = 1,  
    INVALID_NAME = 2,
    CONNECT_ERR = 3,
    CONNECT_TERMINATED = 4,
};

// Prameter positions
enum ParamPosition {
    PORTNUM = 1,
    NAME = 2,
};

// Structure type to hold the program parameters.
// The portNum is the port number to connect.
// The name is the required client name.
// The topics are possible topics this client wants to subscrible.
typedef struct {
    const char* portNum;
    char* name;
    char** topics;
} Parameters;

/* valid_name()
 * ____________
 * This function checks if the name is in valid format.
 *
 * name: The name of this client specified in programme command.
 *
 * Returns: It will return true if the name is valid, otherwise, it returns
 * false.
 */
bool valid_name(char* name) {
    if (!strlen(name)) {
	return false;
    } else {
	for (int i = 0; i < strlen(name); i++) {
	    if (name[i] == ' ' || name[i] == ':' || name[i] == '\n') {
		return false;
	    }
	}
    }
    return true;
}

/* parse_command_line()
 * ____________________
 * This function parses command line and returns Paratemers struct.
 *
 * argc: The number of arguments of command line.
 * argv: The array of command line arguments.
 *
 * Returns: It will return a Parameters struct.
 * Errors: It will occurs errors if the command line arguments are invalid.
 */
Parameters parse_command_line(int argc, char** argv) {
    if (argc < MIN_ARGS + 1) {
	fprintf(stderr, "Usage: psclient portnum name [topic] ...\n");
	exit(USAGE_ERR);
    }
    Parameters params;
    params.topics = malloc(sizeof(char*) * (argc - TOPICS_START));
    params.portNum = argv[PORTNUM];
    params.name = argv[NAME];
    for (int i = 0; i < argc - TOPICS_START; i++) {
	params.topics[i] = strdup(argv[i + TOPICS_START]);	
    }
    if (!valid_name(params.name)) {
	fprintf(stderr, "psclient: invalid name\n");
	exit(INVALID_NAME);
    }
    for (int i = NAME + 1; i < argc; i++) {
	if (!valid_name(argv[i])) { 
	    fprintf(stderr, "psclient: invalid topic\n");
	    exit(INVALID_NAME);
	}
    }
    return params;
}

/* receiving_info()
 * ________________
 * This function is to receive information from the server.
 *
 * arg: The parameters sent by a thread.
 *
 * Errors: It will returns errors when disconnected from server.
 */
void* receiving_info(void* arg) {
    char* line;
    FILE* receive = (FILE*) arg;
    while ((line = read_line(receive))) {
	printf("%s\n", line);
	fflush(stdout);
    }
    fprintf(stderr, "psclient: server connection terminated\n");
    exit(CONNECT_TERMINATED);
}

/* send_to_server()
 * ________________
 * This function is to send information to the server.
 *
 * arg: The parameters send by a thread.
 */
void* send_to_server(void* arg) {
    char* line;
    FILE* send = (FILE*) arg;
    while ((line = read_line(stdin))) {
	fprintf(send, "%s\n", line);
	fflush(send);
    }
    exit(0);
}

int main(int argc, char** argv) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    Parameters params;
    params = parse_command_line(argc, argv);
    if ((getaddrinfo("localhost", params.portNum, &hints, &ai))) {
	freeaddrinfo(ai);
	fprintf(stderr, "psclient: unable to connect to port %s\n", 
		params.portNum);
	exit(CONNECT_ERR);
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))) {
	fprintf(stderr, "psclient: unable to connect to port %s\n", 
		params.portNum);
	exit(CONNECT_ERR);
    }
    int fd2 = dup(fd);
    FILE* to = fdopen(fd, "w");
    FILE* from = fdopen(fd2, "r");
    for (int i = NAME; i < argc; i++) {
	if (i == NAME) {
	    fprintf(to, "name %s\n", argv[i]);
	    fflush(to);
	} else {
	    fprintf(to, "sub %s\n", argv[i]);
	    fflush(to);
	}
    }
    pthread_t tidRead;
    pthread_t tidSend;
    pthread_create(&tidSend, 0, send_to_server, to);
    pthread_create(&tidRead, 0, receiving_info, from);
    pthread_join(tidRead, NULL);
    pthread_join(tidSend, NULL);
    return 0;
}

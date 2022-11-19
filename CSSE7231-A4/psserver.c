#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <csse2310a3.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <csse2310a4.h>
#include <stringmap.h>
#include <signal.h>

// Useful constants
#define BASE_TEN 10
#define MIN_ARGS 2
#define MAX_ARGS 3
#define PORT_UPPER_BOUND 65535
#define PORT_LOWER_BOUND 1024
#define SPLIT_PARTS 2
#define RESPOND_CODE 200

// Position of arguments from command line
enum ParamPosition {
    CONNECTIONS = 1,
    PORT_NUM = 2,
};

// Program exit values
enum ExitCode {
    USAGE_ERR = 1,
    LISTEN_ERR = 2,
    HTTP_ERR = 3,
};

// Expected space numbers
enum SpaceNums {
    SUB = 1,
    PUB = 2,
    UNSUB = 1,
};

// Structure type to hold the programme parameters.
// The connections parameter indicates the maxinum number of permitted
// connections.
// The portNum indicates which localhost port to listen on.
typedef struct {
    int connections;
    char* portNum;
} Parameters;

// Structure type to hold the thread arguments.
// The fdPtr points to the accepting connection.
// The connGuard is the semaphore guard to limit connections.
// The sm is the StringMap to store sub/pub/unsub information.
// The smGuard is the semaphore guard to avoid race condition to sm.
// The writeTo is the end of client that the thread sends information to.
// The connectedClients is counting the connected clients.
// The completedClients is counting the completed clients.
// The pubOperations is counting the pub operations.
// The subOperations is counting the sub operations.
// The unsubOperations is counting the unsub operations.
// The set is a set of signal that will be blocked to the thread.
// The httpFd is the file descriptor that holds http connection port.
typedef struct {
    int* fdPtr;
    sem_t* connGuard;
    StringMap* sm;
    sem_t* smGuard;
    FILE* writeTo;
    int connectedClients;
    int completedClients;
    int pubOperations;
    int subOperations;
    int unsubOperations;
    sigset_t set;
    int httpFd;
} ThreadArgs; 

typedef struct Node Node;

typedef struct Client Client;

// The structure of Node.
// The data is the information of Client that is stored in this node.
// The next points to the next node.
struct Node {
    Client* data;
    Node* next;
};

// Structure type of a LinkedList that helps store data in StringMap.
// The head is the head node is the StringMap.
typedef struct {
    Node* head;
} LinkedList;

// The structure of Client that stores information of a client.
// The writeTo is the end of the client that the server can send message to.
// The name stands for the name of this client.
struct Client {
    FILE* writeTo;
    char* name;
};

/* usage_error()
 * _____________
 * This function handles usage error. It will print out error messages and exit
 * with corresponding exit code.
 */
void usage_error(void) {
    fprintf(stderr, "Usage: psserver connections [portnum]\n");
    exit(USAGE_ERR);
}

/* parse_command_line()
 * ____________________
 * This function parses command line and return Parameter struct.
 *
 * argc: The number of arguments from command line.
 * argv: The array of command line arguments.
 *
 * Returns: It will return a Parameter struct.
 * Errors: It will occur errors if the command line arguments are invalid. It
 * will then exit the programme with usage error code.
 */
Parameters parse_command_line(int argc, char** argv) {
    Parameters params;
    char* end;
    int portNum;
    if (argc < MIN_ARGS || argc > MAX_ARGS) {

	usage_error();
    }
    params.connections = strtol(argv[CONNECTIONS], &end, BASE_TEN);
    if (strcmp(end, "") || params.connections < 0) {
	usage_error();
    }
    if (argc == MIN_ARGS) {
	params.portNum = "0";
    } else {
	portNum = strtol(argv[PORT_NUM], &end, BASE_TEN);
	if (strcmp(end, "") || (portNum && portNum < PORT_LOWER_BOUND) || 
		portNum > PORT_UPPER_BOUND) {
	    usage_error();
	}
	params.portNum = argv[PORT_NUM];
    }
    return params;
}

/* open_listen()
 * _____________
 *
 * This function opens a socket for the server to listen.
 *
 * params: The Parameter struct received in command line.
 *
 * Returns: It will return the file descriptor that holds the listen port.
 * Errors: It will occur errors when the server is unable to listen on a port,
 * and then exit the program with listening error code.
 */
int open_listen(Parameters params) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 
    if ((getaddrinfo(NULL, params.portNum, &hints, &ai))) {
	freeaddrinfo(ai);
	fprintf(stderr, "psserver: unable to open socket for listening\n");
	fflush(stderr);
    }
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    int optVal = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &optVal,
	    sizeof(int)) < 0) {
	fprintf(stderr, "psserver: unable to open socket for listening\n");
	fflush(stderr);
	exit(LISTEN_ERR);
    }
    if (bind(listenFd, (struct sockaddr*)ai->ai_addr, 
	    sizeof(struct sockaddr))) {
	fprintf(stderr, "psserver: unable to open socket for listening\n");
	fflush(stderr);
	exit(LISTEN_ERR);
    }
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(listenFd, (struct sockaddr*)&ad, &len)) {
	fprintf(stderr, "psserver: unable to open socket for listening\n");
	fflush(stderr);
	exit(LISTEN_ERR);
    }
    fprintf(stderr, "%u\n", ntohs(ad.sin_port));
    fflush(stderr);
    if (listen(listenFd, SOMAXCONN) < 0) { 
	fprintf(stderr, "psserver: unable to open socket for listening\n");
	fflush(stderr);
	exit(LISTEN_ERR);
    }
    return listenFd;
}

/* find_space_nums()
 * _________________
 * This function finds the number of spaces in a line.
 *
 * line: The line to be find.
 * 
 * Returns: It returns the number of spaces.
 */
int find_space_nums(char* line) {
    int spaceNum = 0;
    for (int i = 0; i < strlen(line); i++) {
	if (line[i] == ' ') {
	    spaceNum++;
	}
    }
    return spaceNum;
}

/* check_valid()
 * _____________
 * This function checks if a command is valid.
 *
 * line: The command line to be check.
 *
 * Returns: It returns true if the line is valid, otherwise it returns false.
 */
bool check_valid(char* line) {
    char** cmds;
    char* cpLine = strdup(line);
    int countSpace;
    if (!strlen(cpLine)) {
	return false;
    }
    cmds = split_by_char(cpLine, ' ', SPLIT_PARTS);
    if (!(!strcmp(cmds[0], "pub") || !strcmp(cmds[0], "sub") 
	    || !strcmp(cmds[0], "name") || !strcmp(cmds[0], "unsub"))) {
	return false;
    }
    if (!strcmp(cmds[0], "name") || !strcmp(cmds[0], "sub")
	    || !strcmp(cmds[0], "unsub")) {
	countSpace = find_space_nums(line);
	if (countSpace != SUB) {
	    return false;
	} else {
	    if (!strlen(cmds[1])) {
		return false;
	    }
	    for (int i = 0; i < strlen(cmds[1]); i++) {
		if (cmds[1][i] == ':') {
		    return false;
		}
	    }
	}
    }
    if (!strcmp(cmds[0], "pub")) {
	countSpace = find_space_nums(line);
	if (countSpace < PUB) {
	    return false;
	}
	char** pubLine = split_by_char(cmds[1], ' ', SPLIT_PARTS);
	if (!strlen(pubLine[1])) {
	    return false;
	}
	for (int i = 0; i < strlen(pubLine[0]); i++) {
	    if (pubLine[0][i] == ':' || pubLine[0][i] == ' ') {
		return false;
	    }
	}
    }
    return true;
}

/* process_sub()
 * _____________
 * This function processes the sub command from a client.
 *
 * client: The client that the server connects to.
 * key: The key or topic to be searched in the StringMap.
 * tArgs: The arguments in the current thread.
 */
void process_sub(Client* client, char* key, ThreadArgs* tArgs) {
    StringMap* sm = tArgs->sm;
    LinkedList* linkedList;
    Node* node = malloc(sizeof(Node));
    bool dupSub = false;
    node->data = client;
    node->next = NULL;
    if ((linkedList = stringmap_search(sm, key))) {	
	Node* indicator = malloc(sizeof(Node));
	indicator = linkedList->head;
	while (indicator) {
	    if (indicator->data == client) {
		dupSub = true;
	    }
	    if (!indicator->next) {
		break;
	    }
	    indicator = indicator->next;
	}
	if (!dupSub) {
	    indicator->next = node;
	    tArgs->subOperations++;
	}
    } else {
	linkedList = malloc(sizeof(LinkedList));
	linkedList->head = node;
	stringmap_add(sm, key, linkedList);
	tArgs->subOperations++;
    }
}

/* process_pub()
 * _____________
 * This function processes the pub command from a client.
 *
 * client: The client that the server connects to.
 * key: The key or topic to be searched in the StringMap.
 * value: The value that is stored in the StringMap of the corresponding key.
 * tArgs: The arguments in the current thread.
 */
void process_pub(Client* client, char* key, char* value, ThreadArgs* tArgs) {
    StringMap* sm = tArgs->sm;
    LinkedList* linkedList = malloc(sizeof(LinkedList));
    if ((linkedList = stringmap_search(sm, key))) {
	Node* indicator = malloc(sizeof(Node));
	indicator = linkedList->head;
	while (indicator) {
	    fprintf(indicator->data->writeTo, "%s:%s:%s\n",
		    client->name, key, value);
	    fflush(indicator->data->writeTo);
	    indicator = indicator->next;
	}
    }
    tArgs->pubOperations++;
}

/* process_unsub()
 * _____________
 * This function processes the unsub command from a client.
 *
 * client: The client that the server connects to.
 * key: The key or topic to be searched in the StringMap.
 * tArgs: The arguments in the current thread.
 */
void process_unsub(Client* client, char* key, ThreadArgs* tArgs) {
    StringMap* sm = tArgs->sm;
    LinkedList* linkedList;
    if ((linkedList = stringmap_search(sm, key))) {	
	Node* indicator = malloc(sizeof(Node));
	indicator = linkedList->head;
	if ((indicator->data == client)) {
	    stringmap_remove(sm, key);
	}
	while (indicator) {
	    if (!indicator->next) {
		break;
	    }
	    if ((indicator->next->data == client)) {
		indicator->next = indicator->next->next;
	    }
	    indicator = indicator->next;
	}
	tArgs->unsubOperations++;
    }
}

/* process_communication()
 * _______________________
 * This function process a connection.
 *
 * client: The client that the server connects to.
 * line: The line that received from the connected client.
 * tArgs: The arguments in the current thread.
 */
void process_communication(Client* client, char* line, ThreadArgs* tArgs) {
    bool valid = check_valid(line);
    if (!valid) {
	fprintf(client->writeTo, ":invalid\n");
	fflush(client->writeTo);
    } else {
	char** cmds;	
	char* cpLine = strdup(line);
	cmds = split_by_char(cpLine, ' ', SPLIT_PARTS);
	if (!strcmp(cmds[0], "name") && !client->name) {
	    client->name = strdup(cmds[1]);
	}
	if (!strcmp(cmds[0], "sub") && client->name) {
	    char* topic = strdup(cmds[1]);
	    process_sub(client, topic, tArgs);
	}
	if (!strcmp(cmds[0], "unsub") && client->name) {
	    char* topic = strdup(cmds[1]);
	    process_unsub(client, topic, tArgs);
	}
	if (!strcmp(cmds[0], "pub") && client->name) {
	    char** pubLine = split_by_char(cmds[1], ' ', SPLIT_PARTS);
	    char* topic = strdup(pubLine[0]);
	    char* value = strdup(pubLine[1]);
	    process_pub(client, topic, value, tArgs);
	}
    }
}

/* client_thread()
 * _______________
 * This is a function to be used in a thread that handles a client connection.
 *
 * arg: The parameter that is sent to the thread.
 * 
 * Returns: It will return NULL when this function ends.
 */
void* client_thread(void* arg) {
    ThreadArgs* threadArgs = (ThreadArgs*)arg;
    Client* client = malloc(sizeof(Client));
    int fd = *threadArgs->fdPtr;
    int fd2 = dup(fd);
    FILE* readFrom = fdopen(fd, "r");
    client->writeTo = fdopen(fd2, "w");
    char* line;
    while ((line = read_line(readFrom))) {
	sem_wait(threadArgs->smGuard);
	process_communication(client, line, threadArgs);
	sem_post(threadArgs->smGuard);
    }
    StringMapItem* prev = NULL;
    while ((prev = stringmap_iterate(threadArgs->sm, prev))) {
	if (((LinkedList*)prev->item)->head->data == client) {
	    stringmap_remove(threadArgs->sm, prev->key);
	}
    }
    threadArgs->connectedClients--;
    threadArgs->completedClients++;
    sem_post(((ThreadArgs*)arg)->connGuard);
    fclose(readFrom);
    fclose(client->writeTo);
    return NULL;
}

/* sighup_handling()
 * _________________
 * This is a function to be used in a thread to handle sighup signal.
 *
 * arg: The parameter that is sent to the thread.
 *
 * Returns: It will return NULL when this function ends.
 */
void* sighup_handling(void* arg) {
    ThreadArgs* threadArgs = (ThreadArgs*)arg;
    sigset_t* set = &threadArgs->set;
    int s, sig;
    while (1) {
	s = sigwait(set, &sig); 
	if (!s) {
	    fprintf(stderr, "Connected clients:%d\n",
		    threadArgs->connectedClients);
	    fflush(stderr);
	    fprintf(stderr, "Completed clients:%d\n",
		    threadArgs->completedClients);
	    fflush(stderr);
	    fprintf(stderr, "pub operations:%d\n", threadArgs->pubOperations);
	    fflush(stderr);
	    fprintf(stderr, "sub operations:%d\n", threadArgs->subOperations);
	    fflush(stderr);
	    fprintf(stderr, "unsub operations:%d\n",
		    threadArgs->unsubOperations);
	    fflush(stderr);
	}
    }
    return NULL;
}

/* process_connection()
 * ____________________
 * This function processes normal connections other than http.
 *
 * fdServer: The file descriptor that holds the opened socket of server.
 * params: The parameter information received from command line.
 * threadArgs: The argument of thread information.
 *
 * Error: It will occur errors when the server is unable to listen on the
 * socket, it will exit the programme with listening exit code.
 */
void process_connection(int fdServer, Parameters params, 
	ThreadArgs* threadArgs) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    pthread_t tid;
    sigemptyset(&threadArgs->set);
    sigaddset(&threadArgs->set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &threadArgs->set, NULL);
    pthread_create(&tid, NULL, &sighup_handling, threadArgs);
    pthread_detach(tid);
    if (!params.connections) {
	sem_init(threadArgs->connGuard, 0, SOMAXCONN);
    } else {
	sem_init(threadArgs->connGuard, 0, params.connections);
    }
    sem_init(threadArgs->smGuard, 0, 1);
    while (1) {
	fromAddrSize = sizeof(struct sockaddr_in);
	sem_wait(threadArgs->connGuard);
	fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
	if (fd < 0) {
	    perror("Error accepting connection\n");
	    exit(1);
	}
	char hostname[NI_MAXHOST];
	if (getnameinfo((struct sockaddr*)&fromAddr,
		fromAddrSize, hostname, NI_MAXHOST, NULL, 0, 0)) {
	    fprintf(stderr, "Error getting hostname\n");
	    exit(LISTEN_ERR);
	}
	threadArgs->connectedClients++;
	*(threadArgs->fdPtr) = fd;
	pthread_t threadId;
	pthread_create(&threadId, NULL, client_thread, threadArgs);
	pthread_detach(threadId);
    }
}

/* open_http_listen()
 * __________________
 *
 * This function opens a socket to listen the http connection.
 *
 * port: The port number that the socket listens to.
 *
 * Returns: It will return a file descriptor that holds the listening port.
 * Errors: It will occur errors when the server is unable to listen on the http
 * port, and will exit the programme with HTTP error code.
 */
int open_http_listen(char* port) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 
    if ((getaddrinfo(NULL, port, &hints, &ai))) {
	freeaddrinfo(ai);
	fprintf(stderr, "psserver: unable to open HTTP "
		"socket for listening\n");
	fflush(stderr);
	exit(HTTP_ERR);
    }
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    int optVal = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &optVal,
	    sizeof(int)) < 0) {
	fprintf(stderr, "psserver: unable to open HTTP "
		"socket for listening\n");
	fflush(stderr);
	exit(HTTP_ERR);
    }
    if (bind(listenFd, (struct sockaddr*)ai->ai_addr, 
	    sizeof(struct sockaddr))) {
	fprintf(stderr, "psserver: unable to open HTTP "
		"socket for listening\n");
	fflush(stderr);
	exit(HTTP_ERR);
    }
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(listenFd, (struct sockaddr*)&ad, &len)) {
	fprintf(stderr, "psserver: unable to open HTTP "
		"socket for listening\n");
	fflush(stderr);
	exit(HTTP_ERR);
    }
    if (listen(listenFd, SOMAXCONN) < 0) { 
	fprintf(stderr, "psserver: unable to open HTTP "
		"socket for listening\n");
	fflush(stderr);
	exit(HTTP_ERR);
    }
    return listenFd;
}

/* http_handling()
 * _______________
 * This function is to be used in a thread that handles http request.
 *
 * arg: The parameter that is sent to the thread.
 *
 * Returns: It will return NULL when this function ends.
 */
void* http_handling(void* arg) {
    ThreadArgs* tArgs = (ThreadArgs*)arg;
    int fd = *tArgs->fdPtr;
    int fd2 = dup(fd);
    FILE* readFrom = fdopen(fd, "r");
    FILE* writeTo = fdopen(fd2, "w");
    char* method;
    char* address;
    char* body;
    HttpHeader** headers;
    while (1) {
	if (get_HTTP_request(readFrom, &method, &address, &headers, &body)) {
	    if (strcmp(method, "GET") || strcmp(address, "/stats")) {
		break;
	    }
	    int status = RESPOND_CODE;
	    char* statusExplanation = "OK";
	    int len = snprintf(NULL, 0, "%s:%d\n%s:%d\n%s:%d\n%s:%d\n%s:%d\n",
		    "Connected clients", tArgs->connectedClients,
		    "Completed clients", tArgs->completedClients,
		    "pub operations", tArgs->pubOperations,
		    "sub operations", tArgs->subOperations,
		    "unsub operations", tArgs->unsubOperations);
	    // get the length of memory that should be allocated next.
	    char* respondBody = malloc(sizeof(char) * (len + 1));
	    sprintf(respondBody, "%s:%d\n%s:%d\n%s:%d\n%s:%d\n%s:%d\n",
		    "Connected clients", tArgs->connectedClients,
		    "Completed clients", tArgs->completedClients,
		    "pub operations", tArgs->pubOperations,
		    "sub operations", tArgs->subOperations,
		    "unsub operations", tArgs->unsubOperations);
	    // constructing a respond body.
	    char* response = (construct_HTTP_response(status, 
		    statusExplanation, headers, respondBody));

	    fprintf(writeTo, "%s", response);
	    fflush(writeTo);
	} else {
	    break;
	}
    }
    fclose(readFrom);
    fclose(writeTo);
    return NULL;
}

/* process_http_connection()
 * _________________________
 * This function is to be used in a thread that processes http connection.
 *
 * arg: The parameter that is sent to the thread.
 *
 * Returns: It will return NULL when this function ends.
 * Error: It will occur errors when the server is unable to listen on the
 * http socket, it will exit the programme with HTTP exit code.
 */
void* process_http_connection(void* arg) {
    ThreadArgs* tArgs = (ThreadArgs*)arg;
    int fdHttp = tArgs->httpFd;
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    while (1) {
	fromAddrSize = sizeof(struct sockaddr_in);
	fd = accept(fdHttp, (struct sockaddr*)&fromAddr, &fromAddrSize);
	if (fd < 0) {
	    fprintf(stderr, "psserver: unable to open HTTP"
		    "socket for listening\n");
	    fflush(stderr);
	    exit(HTTP_ERR);
	}
	char hostname[NI_MAXHOST];
	if (getnameinfo((struct sockaddr*)&fromAddr,
		fromAddrSize, hostname, NI_MAXHOST, NULL, 0, 0)) {
	    fprintf(stderr, "Error getting hostname\n");
	    exit(HTTP_ERR);
	}
	*(tArgs->fdPtr) = fd;
	pthread_t threadId;
	pthread_create(&threadId, NULL, http_handling, tArgs);
	pthread_detach(threadId);
    }
}

int main(int argc, char** argv) {
    Parameters params;
    params = parse_command_line(argc, argv);
    int fdServer;
    fdServer = open_listen(params);
    ThreadArgs* threadArgs = malloc(sizeof(ThreadArgs));
    memset(threadArgs, 0, sizeof(ThreadArgs));
    StringMap* sm = stringmap_init();
    threadArgs->sm = sm;
    threadArgs->fdPtr = malloc(sizeof(int));
    threadArgs->connGuard = malloc(sizeof(sem_t));
    threadArgs->smGuard = malloc(sizeof(sem_t));
    char* httpPort;
    if (getenv("A4_HTTP_PORT")) {
	httpPort = strdup(getenv("A4_HTTP_PORT"));
	threadArgs->httpFd = open_http_listen(httpPort);
	pthread_t tid;
	pthread_create(&tid, NULL, process_http_connection, threadArgs);
	pthread_detach(tid);
    }    
    process_connection(fdServer, params, threadArgs);
    return 0;
}

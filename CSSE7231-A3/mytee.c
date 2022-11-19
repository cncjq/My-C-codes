#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <csse2310a3.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define MIN_ARG_LEN 2
#define MAX_ARG_LEN 3

// Program exit values
enum ExitCode {
    USAGE_ERROR = 1,
    READ_ERROR = 2
};

/* Types of expetcted commandline arguments
 * outFile is a mandatory argument that is the name of output file
 * seeParamA indicates if we see optional argument "-a" in command line
 */ 
typedef struct {
    char* outFile;
    bool seeParamA;
} Parameters;

/* usage_error()
 * -------------
 * This function handles usage_error. Not parameters nor return value.
 * It will print error message to stdout and exit this program to 1.
 */
void usage_error(void) {
    fprintf(stderr, "Usage: mytee [-a] outfile\n");
    exit(USAGE_ERROR);
}

/* parse_command_line()
 * --------------------
 * This function parses command line and return Parameters struct.
 *
 * argc: The number of arguments of command line.
 * argv: The array of command line arguments
 *
 * Returns: It will return a Parameters stuct 
 * Errors: It will occur errors if the command line arguments are invalid.
 */ 
Parameters parse_command_line(int argc, char** argv) {
    if (argc < MIN_ARG_LEN || argc > MAX_ARG_LEN) {
	usage_error();
    }

    bool seeOutFile = false;
    Parameters param;
    memset(&param, 0, sizeof(Parameters));
    param.seeParamA = false;
    for (int i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-a") && !param.seeParamA) {
	    param.seeParamA = true;
	} else {
	    if (seeOutFile == true) {
		usage_error();
	    }
	    param.outFile = argv[i];
	    seeOutFile = true;
	}
    }
    if (seeOutFile == false) {
	usage_error();
    }
    return param;
}

int main(int argc, char** argv) {
    Parameters params = parse_command_line(argc, argv);
    FILE* fp;
    if (params.seeParamA) {
	fp = fopen(params.outFile, "a+");
    } else {
	fp = fopen(params.outFile, "w");
    }
    if (!fp) {
	fprintf(stderr, "Error: unable to open %s for writing\n",
		params.outFile);
	exit(READ_ERROR);
    }
    char* line;
    while ((line = read_line(stdin))) {
	fprintf(fp, "%s\n", line);
	printf("%s\n", line);
	fflush(stdout);
	free(line);
    }
    fclose(fp);
    return 0;
}

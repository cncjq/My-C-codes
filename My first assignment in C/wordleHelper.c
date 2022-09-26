#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define USG_ERR_MSG "Usage: wordle-helper [-alpha|-best] [-len len] [-with let\
ters] [-without letters] [pattern]\n"
#define PATTERN_ERR_MSG "wordle-helper: pattern must be of length %d and only c\
ontain underscores and/or letters\n"
#define DICT_ERR "wordle-helper: dictionary file \"%s\" cannot be opened\n"
//enum()
void usg_err(); // function prototype

/* check_len()
 * _______________
 * check if "-len" input is valid.
 *
 * len: the next parameter after "-len" to be checked
 *
 * Returns: returns true if len is from integer 4 to 9
 * Errors: will return false if len is not from integer 4 to 9
 */
int check_len(char len) {
//    printf("%d\n", len);
    if (len >= 52 && len <= 57) { // ascii 4 is 52, 9 is 57
	return true;
    } else {
	return false;
    }
}

/* check_with()
 * ______________
 * check if "-with" input is valid.
 *
 * with: the next parameter after "-with" to be checked
 *
 * Returns: returns true if with is 'A' - 'Z' or  'a' - 'z' or ' '
 * Errors: will return false if with is not letters nor space
 */ 
bool check_with(char* with) {
    for (int i = 0; i < strlen(with); i++) {
	char p = with[i];
	if (p >= 97 && p <= 122) { // ascii 'a-z' are 97-122
	    continue;
	} else if (p >= 65 && p <= 90) { //ascii 'A-Z' are 65-90
	    continue;
	} else if (p == 32) { // ascii ' ' is 32
	    continue;
	} else {
	    return false;
	}
    }
    return true;
}

/* check_without()
 * ______________
 * check if "-without" input is valid.
 *
 * without: the next parameter after "-without" to be checked
 *
 * Returns: will return true if without is 'A' - 'Z' or 'a' - 'z' or ' '
 * Errors: will return false if without is not letters nor space
 */ 
bool check_without(char* without) {
    for (int i = 0; i < strlen(without); i++) {
	char p = without[i];
	if (isupper(p)) { 
	    continue;
	} else if (islower(p)) {
	    continue;
	} else if (p == ' ') {
	    continue;
	} else {
	    return false;
	}
    }
    return true;
}

/* check_pattern()
 * ______________
 * check if pattern is valid
 *
 * pattern: the pattern input to be checked
 *
 * Returns: will return true is pattern contains letters and/or underlines
 * Errors: will return false if pattern contains other than letters and/or '_'
 */
bool check_pattern(char* pattern) {
    for (int i = 0; i < strlen(pattern); i++) {
	char p = pattern[i];
	if (isupper(p)) { // ascii 'a-z' are 97-122
	    continue;
	} else if (islower(p)) { // ascii 'A-Z' are 65-90
	    continue;
	} else if (p == '_') { // ascii '_' is 95
	    continue;
	} else {
	    return false;
	}
    }
    return true;
}

/* pattern_err()
 * _____________
 * print error message to stderr and exit with code 2
 *
 * len: the parameter of "-len"
 *
 * print error message to stderr and exit with code 2
 */
void pattern_err(int len) {
    fprintf(stderr, PATTERN_ERR_MSG, len);
    exit(2); 
}

void dict_err(char* filename) {
    fprintf(stderr, DICT_ERR, filename);
    exit(3);
}

/* usg_err()
 * ____________
 * print error message to stderr and exit with code 1
 */ 
void usg_err(void) {
    fprintf(stderr, USG_ERR_MSG);
    exit(1);
}

// fix latter ********************
void open_dict() {
    char* filename = getenv("WORDLE_DICTIONARY");
    if (!filename) { // if environment variable not set
	filename = "/usr/share/dict/words"; // set default file name
    }
    FILE* fp = fopen(filename, "r");
    if (!fp) { // if file cannot be opened
	dict_err(filename);
    }     
} 

/* contain_checking()
 * __________________
 * check if input "word" contains letters in input "with"
 *
 * with: the parameter after "-with"
 *
 * word: each word read from dictionary
 *
 * Returns: will return true if "word" contains all sequences in "with"
 *	    will return false otherwise
 */
bool contain_checking(char* with, char* word) {
    int* withMap = calloc(26, sizeof(int) * 26);
    for (int i = 0; i < strlen(with); i++) {
	int offset = toupper(with[i]) - 'A';
	withMap[offset]++;
    }
    for (int j = 0; j < strlen(word) - 1; j++) {
	int offset = toupper(word[j]) - 'A';
	if (withMap[offset] != 0) {
	    withMap[offset]--;
	}
    }
    for (int i = 0; i < 26; i++) {
	if (withMap[i] != 0) {
	    free(withMap);
	    return false;
	}
    }
    free(withMap);
    return true;
}

/* exclude_checking()
 * __________________
 * check if input "word" excludes each letter in input "without"
 *
 * without: the parameter after "-without"
 *
 * word: each word read from dictionary
 *
 * Returns: will return true if "word" excludes every letter in "without"
 *	    will return false otherwise
 */
bool exclude_checking(char* without, char* word) {
    int* withoutMap = calloc(26, sizeof(int) * 26);
//    for (int i = 0; i < 26; i++) {
//	printf("%d\n", withoutMap[i]);
//    }
    for (int i = 0; i < strlen(without); i++) {
	int offset = toupper(without[i]) - 'A';
	if (withoutMap[offset] != 1) {
	    withoutMap[offset] = 1;
	}
    }

    for (int j = 0; j < strlen(word) - 1; j++) {
	int offset = toupper(word[j]) - 'A';
	if (withoutMap[offset] == 1) {
	    return false;
	}
    }
    free(withoutMap);

    return true;
}

int main(int argc, char** argv) {
    //    printf("%d\n", exclude_checking(argv[1], argv[2]));
    //    printf("%d\n", contain_checking(argv[1], argv[2]));
    //    exit(2);
    int countLen = 0; 
    int countWith = 0; 
    int countWithout = 0;
    int countSortArg = 0;
    int countPattern = 0;
    int length = 5;
    char patternArg[10] = " "; // fix latter
    char* inWith = NULL;
    char* inWithout = NULL;
    if (argc >= 10) {
	usg_err();
    }
    for (int i = 1; i < argc; i++) {
	char* input = argv[i];
	if (argv[i][0] == '-') {
	    if (!strcmp(input, "-len")) { // "-len" verifing
		if (i == argc - 1) {
		    usg_err(); // error if "-len" is the last input
		}
		if (countLen) {
		    usg_err(); // error if "-len" appears twice
		}
		char* inLen = argv[++i]; // get input after "-len"
		for (int i = 0; i < strlen(inLen); i++) {
		    if (check_len(inLen[i]) == false) {
			usg_err();
		    } else if (check_len(inLen[i]) == true) {
			continue;
		    }
		}

		length = atoi(inLen); // check if length is valid
		countLen += 1; // end of "-len"
	    } else if (!strcmp(input, "-with")) {
		if (i == argc - 1) {
		    usg_err(); // error if "-with" is the last input
		}
		if (countWith) {
		    usg_err(); // error if "-with" appears twice
		}
		inWith = (char*) realloc(inWith, sizeof(argv[++i]));
		inWith = argv[++i];
		//	        char* inWith = argv[++i]; // get the input after "-with"
		if (check_with(inWith) == false) {
		    usg_err();
		}
		countWith += 1; // end of "-with"
	    } else if (!strcmp(input, "-without")) {
		if (i == argc - 1) {
		    usg_err(); // error if "-without" is the last input
		}
		if (countWithout) {
		    usg_err(); //error if "-without" appears twice
		}
		inWithout = (char*) realloc(inWithout, sizeof(argv[++i]));
		inWithout = argv[++i]; // get the input after "-without"
		if (check_without(inWithout) == false) {
		    usg_err();
		}
		countWithout += 1; // end of "-without"
	    } else if (!strcmp(input, "-alpha")) {
		if (countSortArg) {
		    usg_err(); // error if "-alpha" appears again or with "best"
		}
		countSortArg++;
		// todo output
	    } else if (!strcmp(input, "-best")) {
		if (countSortArg) {
		    usg_err(); // error if "-best" appears again or with "alpha"
		}
		countSortArg++;
		// todo output
	    } else {
		usg_err();
	    } 
	} else {
	    if (countPattern) {
		usg_err();
	    }
	    countPattern++;
	    strcpy(patternArg, input);
	}	
    } 
    while (*patternArg != ' ') {
	if (strlen(patternArg) != length) {
	    pattern_err(length);
	} else if (check_pattern(patternArg) == false) {
	    pattern_err(length);
	}
	break;
    }
    //    open_dict();
    char* filename = getenv("WORDLE_DICTIONARY");
    if (!filename) { // if environment variable not set
	filename = "/usr/share/dict/words"; // set default file name
    }    
    FILE* fp = fopen(filename, "r");
//    char* c = calloc(50, 50); // assume each line is less than 50
    char c[50];
    c[0] = '\0';
//    c[0] = NULL;
//    printf("%s\n", c);
//    char* c = calloc(c, sizeof(char) * 50);
    if (!fp) { // if file cannot be opened
	dict_err(filename);
    }
//    char* cpDict = malloc(sizeof(char));
    int a = 0;
//    char** cpDict = malloc(sizeof(char*));
//    int r = 0;
    do { // looping whole file until EOF
	//	int countInwith = 0;
	//	int countSeq = 0;
	if (feof(fp)) {
	    break;
	}
	int notLetter = 0;
	int countPattern = 0;
//	int markRun = 0;
	fgets(c, sizeof(c), fp); // read and get each line
//	if (c) {
//	    markRun = 1;
//	}
	if (length + 1 != strlen(c)) {
	    continue;
	}
	for (int i = 0; i < length; i++) {
	    c[i] = toupper(c[i]); // print upper letters
	    if (c[i] != 10 && (c[i] < 65 || c[i] > 90)) {
		notLetter = 1;
		break;
	    } // end of checking prints
	    if (*patternArg != 32) { // ascii of space is 32
		if (patternArg[i] == 95) { // ascii of '_' is 95
		    continue;
		} else if (toupper(patternArg[i]) != c[i]) {
		    countPattern = 1; // show different to parameter
		}
	    } // end of pattern checking
	}
	if (notLetter) { // not print when the line contains not only letters
	    continue;
	} else if (countPattern) { // not print when pattern mismatched
	    continue;
	} 
	if (inWith) {
	    bool ifContain = contain_checking(inWith, c);
	    if (ifContain == false) {
		continue; // not print if not contain -with parameter
	    } 

	} 
	if (inWithout) {
	    bool ifExclude = exclude_checking(inWithout, c);
	    if (ifExclude == false) {
		continue;
	    }
	}
	a = 1;
//	int r = 0;
//	char** m[10][10];
//	cpDict = (char**) realloc(cpDict, sizeof(cpDict) * 2);
//	int row = sizeof(cpDict) / sizeof(cpDict[0]);
//	int col = sizeof(cpDict)[0] / sizeof(cpDict[0][0]);
//	cpDict[r] = c;
//	r++;
	printf("%s", c);
    } while (1);
//    for (int i = 0; i < r ; i++) {
//	printf("%s", cpDict[i]);	    
//    }
//    printf("%d\n", c[0]);
    if (c[0] != '\0' && a == 0) {
	exit(4);
    } 
    fclose(fp);
    return 0;
}

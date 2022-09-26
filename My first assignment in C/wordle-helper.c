#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define usgErrMsg "Usage: wordle-helper [-alpha|-best] [-len len] [-with letters] [-without letters] [pattern]"
//enum()
int countLen = 0;
int countWith = 0;

/* to be fixed
 *
 * int check_len(int len){
    if (len >= 4 && len <= 9){
	return len;
    }
    // tobe fixed usg_err();
} */

char* check_with(char* with){
    for (int i = 0; i < strlen(with); i++){
	char p = with[i];
	if (p >= 97 && p <= 122){ // ascii 'a-z' are 97-122
	    continue;
	} else if (p >= 65 && p <= 90){ //ascii 'A-Z' are 65-90
	    continue;
	} else if (p == 95){ // ascii '_' is 95
	    continue;
	}
	else {
	    // to be fixed usg_err();
	}
    }
    return with;
 }

int check_pattern(char* pattern, int len){

    // todo
    return 0;
}

void usg_err(void){
    printf("%s\n", usgErrMsg);
    exit(1);
}

int main(int argc, char** argv){
  
    // printf("%d\n", argc);
    /* some buuuuuuuuuugs to be fixed
     *
     * for(int i = 0; i < argc; i++){
	char* input = argv[i];
	// printf("%s\n", input);
	 if (!strcmp(input, "-len")){ // "-len" verifing
	    if (i == argc - 1){
		usg_err();
	    }
	    if (countLen){
		usg_err();
	    }
	    char* inLen = argv[++i];
	    check_len(atoi(inLen));
	    // printf("%s\n", inLen);
	    countLen += 1; // end of "-len"
	} else if (!strcmp(input, "-with")){
	    if (i == argc -1){
		usg_err();
	    }
	    if (countWith){
		usg_err();
	    }
	    char* inWith = argv[++i];
	    check_with(inWith);
	    countWith += 1; // end of "-with"
	}
	else{
	    // pattern
	    // printf("%s\n", input);
	    usg_err();
	} 
    } */

    // printf("%s\n", usgErrMsg);
    // exit(1);
    return 0;
}

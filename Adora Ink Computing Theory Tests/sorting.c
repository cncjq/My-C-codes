// sorting.c
// written by Jiaqiang Chen, Sep 2022

#include <stdio.h>
#include <stdlib.h>

#define ENTRIES_NUM 10000 // number of entries

// generating and return entries, each entry is an integer between 0 and 10000
int* generating_entries(void) {
    int* entries = malloc(sizeof(int) * ENTRIES_NUM);
    for (int i = 0; i < ENTRIES_NUM; i++) {
	entries[i] = rand() % (ENTRIES_NUM+1);
    }
    return entries;
}

// quick sorting in ascending order
// require 3 arguments in this function: entries array, start entry, end entry
int sorting(int* entries, int start, int end) {
    if (start >= end) {
	return 0;
    }
    int pivot = entries[end];
    int swp;
    int pointer = start;
    for (int i = start; i < end; i++) {
	if (entries[i] > pivot) {
	    if (pointer != i) {
		swp = entries[i];
		entries[i] = entries[pointer];
		entries[pointer] = swp;
	    }
	    pointer++;
	}
    }
    swp = entries[end];
    entries[end] = entries[pointer];
    entries[pointer] = swp;
    sorting(entries, start, pointer - 1);
    sorting(entries, pointer + 1, end);
    return 0;
}

int main(int argc, char** argv) {
    int* entries = generating_entries();
    sorting(entries, 0, ENTRIES_NUM - 1);
    for (int i = 0; i < ENTRIES_NUM; i++) {
	printf("%d\n", entries[i]);
    }
    free(entries);
    return 0;	
}

#include <stringmap.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Data structure stored in the stringMap
// stringMapItems is the StringMapItem entries in the stringMap.
// itemNums indicates how many items are in stringMap.
struct StringMap {
    StringMapItem* stringMapItems; 
    int itemNums;
};

StringMap* stringmap_init(void){
    StringMap* sm = malloc(sizeof(StringMap));
    sm->itemNums = 0;
    sm->stringMapItems = malloc(sizeof(StringMapItem));
    return sm;
}

void stringmap_free(StringMap* sm) {
    if (sm) {
	for (int i = 0; i < sm->itemNums; i++) {
	    free(sm->stringMapItems[i].key);
	}
	free(sm->stringMapItems);
	free(sm);
    }
}

void* stringmap_search(StringMap* sm, char* key) {
    if (!sm || !key) {
	return NULL;
    }
    for (int i = 0; i < sm->itemNums; i++) {
	if (!strcmp(sm->stringMapItems[i].key, key)) {
	    return sm->stringMapItems[i].item;
	}
    } 
    return NULL;
}

int stringmap_add(StringMap* sm, char* key, void* item) {
    if (!sm) {
	return 0;
    }
    bool repreatKey = false;
    for (int i = 0; i < sm->itemNums; i++) {
	if (!strcmp(sm->stringMapItems[i].key, key)) {
	    repreatKey = true;
	}
    }
    if (!repreatKey && key && item) {
	sm->itemNums++;
	sm->stringMapItems = realloc(sm->stringMapItems, 
		sizeof(StringMapItem) * sm->itemNums);
	sm->stringMapItems[sm->itemNums - 1].key = strdup(key);
	sm->stringMapItems[sm->itemNums - 1].item = item;
	return 1;
    }
    return 0;
}

int stringmap_remove(StringMap* sm, char* key) {
    if (sm && key) {
	for (int i = 0; i < sm->itemNums; i++) {
	    if (!strcmp(sm->stringMapItems[i].key, key)) {
		free(sm->stringMapItems[i].key);
		sm->stringMapItems[i] = sm->stringMapItems[sm->itemNums - 1];
		sm->itemNums--;
		return 1;
	    }
	}
    }
    return 0;
}

StringMapItem* stringmap_iterate(StringMap* sm, StringMapItem* prev) {
    if (!sm) {
	return NULL;
    }
    if (!prev) {
	if (!sm->itemNums) {
	    return NULL;
	}
	return sm->stringMapItems;
    } else {
	if (prev == &sm->stringMapItems[sm->itemNums - 1]) {
	    return NULL;
	}
	return prev + 1;
    }
}


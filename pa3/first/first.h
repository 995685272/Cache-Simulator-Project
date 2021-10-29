#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef unsigned long long uint64_t;

typedef enum 
{
    	FIFO, LRU,
} 
policy_t;

typedef struct 
{
    	int cacheSize;
    	int set;
    	int associativity;
    	int blockSize;
    	int t, s, b;
} 
cacheConfig_t;

typedef struct 
{
    	bool valid;
    	uint64_t tag;
} 
cacheLine_t;

typedef struct 
{
    	cacheConfig_t config;
    	policy_t policy;
    	cacheLine_t** cacheLine;
    	int* fifo;
    	int** lru;
} 
cache_t;

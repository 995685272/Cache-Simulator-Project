#include "first.h"

cache_t cache;

void initCache();
void freeCache();
void simulate(FILE* trace);

int main(int argc, char* argv[])
{
    	if(argc < 6){
        	fprintf(stderr, "we need more arguments.\n");
        	exit(-1);
    	}
    	int cacheSize, blockSize;
    	sscanf(argv[1], "%d", &cacheSize);
    	cache.config.cacheSize = cacheSize;
    	sscanf(argv[2], "%d", &blockSize);
   	cache.config.blockSize = blockSize;
    	if(strcmp(argv[3], "fifo") == 0)
	{
        	cache.policy = FIFO;
    	}
    	else if(strcmp(argv[3], "lru") == 0)
	{
        	cache.policy = LRU;
    	}
    	if(strcmp(argv[4], "direct") == 0)
	{
	        cache.config.associativity = 1;
	        cache.config.set = cacheSize / blockSize;
	}
    	else if(strcmp(argv[4], "assoc") == 0)
	{
        	cache.config.set = 1;
		cache.config.associativity = cacheSize / blockSize;
    	}
    	else
	{
        	sscanf(argv[4], "assoc:%d", &cache.config.associativity);
        	cache.config.set = cacheSize / (blockSize * cache.config.associativity);
    	}
    	char* traceFile = argv[5];
    	FILE* trace = fopen(traceFile, "r");
    	
	if(trace == NULL)
	{
	        fprintf(stderr, "cannot open trace file %s.\n", traceFile);
        	exit(-1);
    	}

    	initCache();
    	simulate(trace);
    	freeCache();
   	return 0;
}

static int getBitCount(int num)
{
    	for(int i = 0; i < 32; i++)
	{
        	if(num & (1 << i))
		{
        	    	return i;
        	}
    	}
    	return -1;
}

void initCache()
{
    	cache.config.s = getBitCount(cache.config.set);
    	cache.config.b = getBitCount(cache.config.blockSize);
    	cache.config.t = 64 - cache.config.s - cache.config.b;
    	cache.cacheLine = (cacheLine_t**)malloc(sizeof(cacheLine_t*) * cache.config.set);
    	for(int i = 0; i < cache.config.set; i++)
	{
        	cache.cacheLine[i] = (cacheLine_t*)malloc(sizeof(cacheLine_t) * cache.config.associativity);
        	for(int j = 0; j < cache.config.associativity; j++)
		{
            		cache.cacheLine[i][j].valid = false;
        	}
    	}
    	cache.fifo = NULL;
    	cache.lru = NULL;
    	if(cache.policy == FIFO)
	{
       	 	cache.fifo = (int*)malloc(sizeof(int) * cache.config.set);
        	memset(cache.fifo, 0, sizeof(int) * cache.config.set);
    	}
    	if(cache.policy == LRU)
	{
        	cache.lru = (int**)malloc(sizeof(int*) * cache.config.set);
        	for(int i = 0; i < cache.config.set; i++)
		{
            		cache.lru[i] = (int*)malloc(sizeof(int) * cache.config.associativity);
            		memset(cache.lru[i], 0, sizeof(int) * cache.config.associativity);
        	}
    	}
}

void freeCache()
{
    	for(int i = 0; i < cache.config.set; i++)
	{
        	free(cache.cacheLine[i]);
    	}
    	free(cache.cacheLine);
    	if(cache.policy == FIFO)
	{
        	free(cache.fifo);
    	}
    	if(cache.policy == LRU)
	{
        	for(int i = 0; i < cache.config.set; i++)
		{
            		free(cache.lru[i]);
        	}
        	free(cache.lru);
    	}
}

static bool hit(uint64_t address)
{
    	int s = cache.config.s;
    	int b = cache.config.b;
    	int t = cache.config.t;
    	uint64_t tag = address >> (s + b);
    	int si = (int)((address << t) >> (t + b));
    	
	for(int i = 0; i < cache.config.associativity; i++)
	{
        	if(cache.cacheLine[si][i].valid && cache.cacheLine[si][i].tag == tag)
		{
            		if(cache.policy == LRU)
			{
                		cache.lru[si][i] = -1;
           		}
            		return true;
        	}
    	}
    	return false;
}

static int evict(int si)
{
    	if(cache.policy == FIFO)
	{
        	int index = cache.fifo[si];
        	cache.fifo[si] = (index + 1) % cache.config.associativity;
        	return index;
    	}
    	if(cache.policy == LRU)
	{
        	int index = 0;
        	int max = cache.lru[si][0];
        	for(int i = 1; i < cache.config.associativity; i++)
		{
            		if(max < cache.lru[si][i])
			{
                		index = i;
                		max = cache.lru[si][i];
            		}
        	}
        	return index;
    	}
    	return -1;
}

static void put(uint64_t address)
{
    	int s = cache.config.s;
    	int b = cache.config.b;
    	int t = cache.config.t;
    	uint64_t tag = address >> (s + b);
    	int si = (int)((address << t) >> (t + b));
    	for(int i = 0; i < cache.config.associativity; i++)
	{
        	if(!cache.cacheLine[si][i].valid)
		{
            		cache.cacheLine[si][i].valid = true;
            		cache.cacheLine[si][i].tag = tag;
            		if(cache.policy == LRU)
			{
                		cache.lru[si][i] = -1;
            		}
            		return;
        	}
    	}

    	int index = evict(si);
    	cache.cacheLine[si][index].valid = true;
    	cache.cacheLine[si][index].tag = tag;
    	if(cache.policy == LRU)
	{
        	cache.lru[si][index] = -1;
    	}
    	return;
}

static void lru(uint64_t address)
{
    	int b = cache.config.b;
    	int t = cache.config.t;
    	int si = (int)((address << t) >> (t + b));
    	for(int i = 0; i < cache.config.associativity; i++)
	{
        	if(cache.cacheLine[si][i].valid)
		{
            		cache.lru[si][i]++;
        	}
    	}
}

void simulate(FILE* trace)
{
    	fseek(trace, 0, SEEK_SET);
    	int memoryRead = 0;
    	int memoryWrite = 0;
    	int cacheHit = 0;
    	int cacheMiss = 0;
    	char rw;
    	uint64_t address;
    	char buffer[1024];
    	while(fgets(buffer, 1024, trace) != NULL)
	{
        	if(buffer[strlen(buffer) - 1] == '\n')
		{
            		buffer[strlen(buffer) - 1] = '\0';
        	}
        	if(strcmp(buffer, "#eof") == 0)
		{
            		break;
        	}

        	sscanf(buffer, "%c %llx", &rw, &address);
        	if(hit(address))
		{
            		cacheHit++;
        	}
        	else
		{
            		cacheMiss++;
            		memoryRead++;
            		put(address);
        	}
        	if(cache.policy == LRU)
		{
            		lru(address);
        	}
        	if(rw == 'W')
		{
            	memoryWrite++;
        	}
    	}

    	printf("Memory reads: %d\n", memoryRead);
    	printf("Memory writes: %d\n", memoryWrite);
    	printf("Cache hits: %d\n", cacheHit);
    	printf("Cache misses: %d\n", cacheMiss);
}

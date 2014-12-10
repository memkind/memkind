/*
 * Copyright (C) 2014 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Basic hbwmalloc testing, will verify that hbw memory is available,
 * if so, then it will try to run over all functions 
*/

#include <stdio.h>
#include <stdlib.h>
#include <hbwmalloc.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

char use_hbw = 0;
uint64_t N = 100000000;
uint32_t num_threads = 1;

void bad_parameters(char name[]) {
    printf("Counts prime numbers up to a given bound using sieve of Eratosthenes.\n");
    printf("Usage:\n");
    printf("%s [-n <N>] [--hbw]\n", name);
    printf("\t-n <N> will change upper bound to N, default is 10^8\n");
    printf("\t--hbw Use hbw_calloc instead of calloc when allocating memory\n");
    printf("\t--threads <T>\n");
    printf("\t-t <T>\n");
    printf("\t\tUse T threads, every thread performing the same task using its own allocated memory, default is 1\n");
}

#define DATA_TYPE uint64_t

DATA_TYPE* reserve_bitmap(uint64_t N,char use_hbw) {
    int64_t number_of_elements = N/(8*sizeof(DATA_TYPE));
    if (N%(8*sizeof(DATA_TYPE))) {
        number_of_elements++;
    }
    if (use_hbw) {
        return (DATA_TYPE*)hbw_calloc(number_of_elements, sizeof(DATA_TYPE));
    }
    return (DATA_TYPE*)calloc(number_of_elements, sizeof(DATA_TYPE));
}

void set_bit(DATA_TYPE *bitset,uint64_t p) {
    const int bits_per_element = 8*sizeof(DATA_TYPE);
    uint64_t index = p/bits_per_element;
    uint64_t offset = p%bits_per_element;
    bitset[index]|=(1LL<<offset);
}

char read_bit(DATA_TYPE *bitset,uint64_t p) {
    const int bits_per_element = 8*sizeof(DATA_TYPE);
    uint64_t index = p/bits_per_element;
    uint64_t offset = p%bits_per_element;
    return (bitset[index]>>offset)&1;
}

uint64_t *reserve_primes(uint64_t N, char use_hbw) {
    double phi_bound_double;
    uint64_t phi_bound;
    double logN = log(N);
    phi_bound_double = N*(1.0+(1.2762/logN))/logN;
    phi_bound=phi_bound_double+10;
    if (use_hbw) {
        return (uint64_t*)hbw_malloc(sizeof(uint64_t)*phi_bound);
    }
    return (uint64_t*)malloc(sizeof(uint64_t)*phi_bound);
}

void make_composite(uint64_t product, uint64_t index, uint64_t count,DATA_TYPE *bitset, uint64_t *primes) {
    while (index<count) {
        uint64_t composite = product*primes[index];
        if (composite>N) {
            break;
        }
        do {
            set_bit(bitset, composite);
            make_composite(composite, index+1, count, bitset, primes);
            composite *= primes[index];
        } while(composite<=N);
        index++;
    }
}

void make_composite_2(uint64_t product, uint64_t index, uint64_t count,DATA_TYPE *bitset, uint64_t *primes) {
    uint64_t composite = product*product;
    while (composite<=N) {
        set_bit(bitset, composite);
        composite += 2*product;
    }
}

void *count_primes_thread(void *param) {
    uint64_t th_id = (uint64_t)param;
    uint64_t count;
    uint64_t n;
    DATA_TYPE *composite_bitset = reserve_bitmap(N+1, use_hbw);
    uint64_t *primes = reserve_primes(N, use_hbw);
    if (composite_bitset==NULL) {
        printf("Erro while trying to allocate memory for sieve on thread %lu\n", th_id);
        return (void*)-1;
    }
    // mark 0 and 1 as composite numbers
    set_bit(composite_bitset,0);
    set_bit(composite_bitset,1);
    // mark every pair as composite except for 2 and count it as prime
    count = 1;
    // there is no need to mark even numbers as they will not be checked
    /*
    for (uint64_t i=4;i<=N;i+=2) {
        set_bit(composite_bitset,i);
    }*/
    primes[0]=2;
    n = 3;
    while (n<=N) {
        if (read_bit(composite_bitset, n)==0) {
            // printf("%lu\n", n);
            //prime number
            primes[count]=n;
            count++;
            make_composite_2(n,0,count,composite_bitset, primes);
        }
        n+=2;
    }
    printf("Found %lu prime numbers on thread %lu\n", count, th_id);
    if (use_hbw) {
        hbw_free(composite_bitset);
    }
    else {
        free(composite_bitset);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // parse arguments
    uint64_t i=1;
    while (i<argc) {
        if ((strcmp(argv[i], "-t" )==0) || (strcmp(argv[i], "--threads" )==0)) {
            i++;
            if (i>=argc) {
                bad_parameters(argv[0]);
                return -1;
            }
            int res = sscanf(argv[i],"%u", &num_threads);
            if (res==0){
                bad_parameters(argv[0]);
                return -1;
            }
            i++;
        }
        else {
            bad_parameters(argv[0]);
            return -1;
        }
    }
    // Check if memory is available
    if (hbw_check_available()==0) {
        use_hbw=1;
    }
    else {
        use_hbw=0;
        printf("No hbw memory available\n");
    }
    
    printf("Counting prime numbers up to %lu, using %s memory and %u threads\n", N, use_hbw?"hbw":"regular", num_threads);
    // create threads
    pthread_t *threads = malloc(sizeof(pthread_t)*num_threads);
    for (i=0;i<num_threads;i++) {
        pthread_create(&threads[i], NULL, count_primes_thread, (void*)i);
    }
    // wait for threads to finish
    for (i=0;i<num_threads;i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    return 0;
}

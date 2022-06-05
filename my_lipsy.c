#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

#define MAX_INPUT_BUFF_SIZE 2048

typedef struct {
    unsigned long int total_allocated_mem;
    unsigned long int freed_mem; 
} Mem_info;

static char input_buf[MAX_INPUT_BUFF_SIZE];

static Mem_info mem_manager  = {
    .total_allocated_mem = 0,
    .freed_mem = 0    
};


void *mem_malloc (size_t size) {
    mem_manager.total_allocated_mem +=size;
    printf("Allocating %ld of mem\n",size);
    return malloc(size);
}
/*
void mem_free (void *ptr) {
    size_t size = malloc_size(ptr);
    mem_manager.freed_mem +=size;
    printf("Mem manager freeing %ld, mem\n",size);
    free(ptr);
    ptr = NULL;

}
*/
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(input_buf, MAX_INPUT_BUFF_SIZE, stdin);
    char *tmp_buf = mem_malloc(strlen(input_buf)+1);
    if (!tmp_buf) {
        printf("Error:%s, failed to allocate mem, errno:%d\n",__func__, errno);
        exit(1);
    }

    strcpy(tmp_buf, input_buf); 
    tmp_buf[strlen(tmp_buf)-1] = '\0';

    return tmp_buf;
}

void add_history(char* unused) {}

int main(int argc, char **argv) {
    puts("Lispy version 1.0.0");
    puts("press Ctrl+c to exit\n");

    while (1) {
        char * input =  readline("Lispy> ");
        printf("No you'r not %s\n", input);
       // printf("char_len:%ld mem:%ld\n",strlen(input),malloc_usable_size(input));
        free(input);
    }

    return 0;

}
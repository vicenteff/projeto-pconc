#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

    char *dir = malloc(100 * sizeof(char));
    char *order = malloc(100 * sizeof(char));
    int threads;

    strcpy(dir, argv[1]);
    threads = atoi(argv[2]);
    strcpy(order, argv[3]);

    printf("IN DIRECTORY: %s\nNUMBER OF THREADS: %d\nORDER BY: %s\n", dir, threads, order);

}
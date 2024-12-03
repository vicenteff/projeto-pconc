#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h>

char *get_extension(const char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot;
}

int listdir(char *directory, char *extension) {
  DIR *d;
  struct dirent *f;
  d = opendir(directory);
  if (d) {
    while (((f = readdir(d)) != NULL) /*&& !strcmp(get_extension(f->d_name), extension)*/) {
        if(!strcmp(get_extension(f->d_name), extension)) {
            printf("%s ", f->d_name);
            struct stat st;
            stat(f->d_name, &st);
            printf("%ld\n", st.st_size);
        }
    }
    closedir(d);
  }
  return(0);
}

int main(int argc, char *argv[]) {

    char *dir = malloc(100 * sizeof(char));
    char *order = malloc(100 * sizeof(char));
    int threads;

    strcpy(dir, argv[1]);
    threads = atoi(argv[2]);
    strcpy(order, argv[3]);

    if(threads < 1) {
        perror("INVALID NUMBER OF THREADS\n");
        exit(1);
    }

    if((strcmp(order, "-name") != 0) && (strcmp(order, "-size") != 0)) {
        perror("INVALID ORDERING CRITERION\n");
        exit(2);
    }

    listdir(argv[1], ".jpeg");





    printf("IN DIRECTORY: %s\nNUMBER OF THREADS: %d\nORDER BY: %s\n", dir, threads, order);

}
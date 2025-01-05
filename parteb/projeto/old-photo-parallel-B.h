#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <gd.h>
#include "image-lib.h"


//STRUCTURE FOR THE FILES' INFORMATION
typedef struct Fileinfo {
    char *filename;
    long int size;
    int exists;
} Fileinfo;

//STRUCTURE FOR PASSING ARGUMENTS INTO THREAD FUNCTION
typedef struct thread_args {
    int lower_limit;
    int upper_limit;
    Fileinfo *fileinfo;
    Fileinfo *read_fileinfo;
    gdImagePtr in_texture_img;
    char *directory;
} thread_args;

//STRUCTURE FOR PASSING ARGUMENTS TO STATISTICS FUNTION
typedef struct stats_args {
    int n_files;
    int files_done;
} stats_args;

int compare_by_name(const void *a, const void *b);
int compare_by_size(const void *a, const void *b);
char *get_extension(const char *filename);
int filecount(char *directory, char *extension);
int listdir(char *directory, char *extension, Fileinfo *fileinfo);
int sort_fileinfo(Fileinfo *fileinfo, char *order, int n_files);
void *applyfilter(void *args);
void *s_key(void *args);
int print_stats(int n_files, int files_done, struct timespec total_time);
int send_to_pipe(Fileinfo *fileinfo, int n_files);

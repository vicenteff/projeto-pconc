#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <gd.h>
#include "image-lib.h"
#include "old-photo-parallel-B.h"
#include <stdatomic.h>
#include <fcntl.h>

extern int pipefd[2];
extern atomic_int files_done;

extern int time_pipefd[2];
struct timespec total_time = {0,0};
pthread_mutex_t lock;

//FUNCTION TO COMPARE USING THE NAME
int compare_by_name(const void *a, const void *b) {
    Fileinfo *file1 = (Fileinfo *)a;
    Fileinfo *file2 = (Fileinfo *)b;
    return strcmp(file1->filename, file2->filename);
}

//FUNCTION TO COMPARE USING THE SIZE
int compare_by_size(const void *a, const void *b) {
    Fileinfo *file1 = (Fileinfo *)a;
    Fileinfo *file2 = (Fileinfo *)b;
    return file1->size - file2->size;
}

//FUNCTION TO GET THE EXTENSION OF A FILE
char *get_extension(const char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot;
}

//FUNCTION TO COUNT THE NUMBER OF FILES WITH A CERTAIN EXTENSION
int filecount(char *directory, char *extension) {
    DIR *d;
    int counter = 0;
    struct dirent *f;
    d = opendir(directory);
    if (d) {
        while ((f = readdir(d)) != NULL) {
            if(!strcmp(get_extension(f->d_name), extension)) {
                counter++;
            }
        }
        closedir(d);
    }
    return counter;
}

//FUNCTION TO STORE THE FILES WITH A CERTAIN EXTENSION IN A CERTAIN DIRECTORY
int listdir(char *directory, char *extension, Fileinfo *fileinfo) {
    DIR *d;
    int i = 0;
    struct dirent *f;
    d = opendir(directory);
    if (d) {
        while (((f = readdir(d)) != NULL)) {
            if(!strcmp(get_extension(f->d_name), extension)) {
                strcpy(fileinfo[i].filename, f->d_name);

                char fullpath[512];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", directory, f->d_name);

                struct stat st;
                if (stat(fullpath, &st) == -1) {
                    perror("stat");
                    continue;
                }
                fileinfo[i].size = st.st_size;

                i++;
            }
        }
        closedir(d);
    }
    return 0;
}

//FUNCTION TO SORT THE FILES BY NAME OR SIZE
int sort_fileinfo(Fileinfo *fileinfo, char *order, int n_files) {
    if(!strcmp(order, "-name")) {
        qsort(fileinfo, n_files, sizeof(Fileinfo), compare_by_name);
    } else if(!strcmp(order, "-size")) {
        qsort(fileinfo, n_files, sizeof(Fileinfo), compare_by_size);
    } else {
        return -1;
    }
    return 0;
}

//FUNCTION USED BY THE THREADS TO APPLY YHE FILTERS TO AN IMAGE
//HERE THE DIVISION OF FILES BY THREADS IS GIVEN BY:
// LOWER LIMIT : (i * N)/P, 
// UPPER LIMIT : ((i+1) * N)/P,
// i BEING THE THREAD ID, N THE MUBER OF IMAGES AND P THE NUMBER OF THREADS
void *applyfilter(void *args) {
    thread_args *t_args = (thread_args*) args;
    char out_file_name[100];
    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;
    struct timespec start_avg_time = {0,0}, end_avg_time = {0,0}, diff_time = {0,0};

    for (int i = t_args->lower_limit; i < t_args->upper_limit; i++) {
        if (!t_args->fileinfo[i].exists) {
            clock_gettime(CLOCK_MONOTONIC, &start_avg_time);

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", t_args->directory, t_args->fileinfo[i].filename);

            in_img = read_jpeg_file(fullpath);
            if (!in_img) {
                fprintf(stderr, "Cannot read %s image\n", fullpath);
                continue;
            }

            out_contrast_img = contrast_image(in_img);
            out_smoothed_img = smooth_image(out_contrast_img);
            out_textured_img = texture_image(out_smoothed_img, t_args->in_texture_img);
            out_sepia_img = sepia_image(out_textured_img);

            snprintf(out_file_name, sizeof(out_file_name), "./old_photo_PAR_B/%s", t_args->fileinfo[i].filename);
            if (!write_jpeg_file(out_sepia_img, out_file_name)) {
                fprintf(stderr, "Cannot write %s image\n", out_file_name);
            }

            gdImageDestroy(out_smoothed_img);
            gdImageDestroy(out_sepia_img);
            gdImageDestroy(out_contrast_img);
            gdImageDestroy(in_img);

            clock_gettime(CLOCK_MONOTONIC, &end_avg_time);
            diff_time = diff_timespec(&end_avg_time, &start_avg_time);

            atomic_fetch_add(&files_done, 1);
            write(pipefd[1], &files_done, sizeof(atomic_int));

            pthread_mutex_lock(&lock);

            total_time.tv_sec += diff_time.tv_sec;
            total_time.tv_nsec += diff_time.tv_nsec;

            write(time_pipefd[1], &total_time, sizeof(struct timespec));

            pthread_mutex_unlock(&lock);
        }
    }

    return NULL;
}

//LISTENS FOR S KEY AND PRINTS STATS
void *s_key(void *args) {
    char key;
    int files_done = 0, latest_files_done = 0; 
    
    struct timespec total_time, latest_total_time;

    stats_args *s_args = (stats_args*) args;

    //MAKES THE PIPE NON-BLOCKING
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    int time_flags = fcntl(time_pipefd[0], F_GETFL, 0);
    fcntl(time_pipefd[0], F_SETFL, time_flags | O_NONBLOCK);

    while(1) {

        if(scanf(" %c", &key) == 1) {
            
            //READS ALL VALUES IN PIPE AND KEEP THE LAST ONE WRITTEN
            while (read(pipefd[0], &latest_files_done, sizeof(latest_files_done)) > 0 ) {
                files_done = latest_files_done; 
            }

            while (read(time_pipefd[0], &latest_total_time, sizeof(struct timespec)) > 0) {
                total_time = latest_total_time; 
            }

            if (key == 'S' || key == 's') {
                print_stats(s_args->n_files, files_done, total_time);
            }
        }
    }
    return NULL;
}

int print_stats(int n_files, int files_done, struct timespec total_time) {

    printf("\t %d out of %d files processed so far\n\n", files_done, n_files);

    struct timespec avg_time;
    avg_time.tv_sec = total_time.tv_sec / files_done;
    avg_time.tv_nsec = total_time.tv_nsec / files_done;

    printf("\t Average time per file: %ld.%09ld\n\n", avg_time.tv_sec, avg_time.tv_nsec);

    return 0;
}

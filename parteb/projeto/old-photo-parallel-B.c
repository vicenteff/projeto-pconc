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

#define DIRECTORY argv[1]
#define THREADS argv[2]
#define ORDER argv[3]

int pipefd[2], time_pipefd[2];
atomic_int files_done = 0;

extern pthread_mutex_t lock;

int main(int argc, char *argv[]) {

    //DECLARES TIMEKEEPING STRUCTURES
    struct timespec start_time_total, end_time_total;
    struct timespec start_time_seq, end_time_seq;
    struct timespec *start_time_par = malloc(sizeof(struct timespec)*atoi(THREADS)), *end_time_par = malloc(sizeof(struct timespec)*atoi(THREADS));
    struct timespec *par_time = malloc(sizeof(struct timespec)*atoi(THREADS));

    pthread_mutex_init(&lock, NULL); // Initialize the mutex

	clock_gettime(CLOCK_MONOTONIC, &start_time_total);
	clock_gettime(CLOCK_MONOTONIC, &start_time_seq);
    int threads, n_files;
    pipe(pipefd);
    pipe(time_pipefd);

    threads = atoi(THREADS);

    //INPUT FILTERING
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <directory> <threads> <order>\n", argv[0]);
        return 1;
    }

    if(threads < 1) {
        perror("INVALID NUMBER OF THREADS\n");
        exit(1);
    }

    if((strcmp(ORDER, "-name") != 0) && (strcmp(ORDER, "-size") != 0)) {
        perror("INVALID SORTING CRITERION\n");
        exit(2);
    }

    n_files = filecount(DIRECTORY, ".jpeg");

    //ALLOCATE STRUCT TO STORE FILE INFORMATION
    Fileinfo *fileinfo = malloc(n_files * sizeof(Fileinfo));
    for (int i = 0; i < n_files; i++)
        fileinfo[i].filename = malloc(100 * sizeof(char));

    listdir(DIRECTORY, ".jpeg", fileinfo);
    sort_fileinfo(fileinfo, ORDER, n_files);

    char *path = malloc(100 * sizeof(char));

    //CHECKS IF FILE HAS ALREADY BEEN GENERATED
    for(int i = 0; i < n_files; i++) {
        strcpy(path, "./old_photo_PAR_B/");
        strcat(path, fileinfo[i].filename);

        if(access(path, F_OK) != -1) {
            printf("%s EXISTS ✅\n", fileinfo[i].filename);
            fileinfo[i].exists = 1;
        } else {
            printf("%s DOESN'T EXIST ❌\n", fileinfo[i].filename);
            fileinfo[i].exists = 0;
        }
    }

    //CREATE DIRECTORY AND FILE TO SAVE IMAGES AND TIME DATA
    mkdir("old_photo_PAR_B", 0777);
    FILE *fp;
    char timing[20];
    snprintf(timing, sizeof(timing), "timing_%s%s.txt", THREADS, ORDER);
    chdir("old_photo_PAR_B");
    fp = fopen(timing, "w");
    chdir("..");
    

    pthread_t *thread_ids = malloc((threads+1) * sizeof(pthread_t));
    thread_args *t_args = malloc(threads * sizeof(thread_args));
    stats_args *s_args = malloc(sizeof(stats_args));
    gdImagePtr in_texture_img = read_png_file("./paper-texture.png");

    if (in_texture_img == NULL) {
        fprintf(stderr, "Failed to read texture image: %s\n", "./paper-texture.png");
        exit(1);
    }

    //PAUSES SEQUENCIAL TIME KEEPING
    clock_gettime(CLOCK_MONOTONIC, &end_time_seq);

    //INITIALIZES ARGUMENTS, CREATES THREADS THAT RUN THE APPLYFILTER FUNCTION AND STARTS PARALLELIZED TIME KEEPING
    for(int i = 0; i < threads; i++) {
        t_args[i].lower_limit = (i * n_files) / threads;
        t_args[i].upper_limit = ((i + 1) * n_files) / threads;
        t_args[i].fileinfo = fileinfo;
        t_args[i].in_texture_img = in_texture_img;
        t_args[i].directory = DIRECTORY;

        pthread_create(&thread_ids[i], NULL, applyfilter, &t_args[i]);
        clock_gettime(CLOCK_MONOTONIC, &start_time_par[i]);
    }

    s_args->n_files = n_files;
    pthread_create(&thread_ids[threads], NULL, s_key, s_args);

    //WAITS FOR THE THREADS TO END, STOPS PARALLELIZED TIME KEEPING AND STORES TIME DATA
    for(int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        clock_gettime(CLOCK_MONOTONIC, &end_time_par[i]);
        par_time[i] = diff_timespec(&end_time_par[i], &start_time_par[i]);
        if(i == 0)
            fprintf(fp,"\tpar%d \t %10jd.%09ld\n",i, par_time[i].tv_sec, par_time[i].tv_nsec);
        else    
            fprintf(fp,"\tpar%d \t %10jd.%09ld\n",i, par_time[i].tv_sec - par_time[i-1].tv_sec, labs(par_time[i].tv_nsec - par_time[i-1].tv_nsec));
    }

    pthread_mutex_destroy(&lock);
    
    //RESUMES SEQUQNCIAL TIME KEEPING
    clock_gettime(CLOCK_MONOTONIC, &start_time_seq);

    //FREE ALLOCATED MEMORY
    free(thread_ids);
    free(t_args);
    free(path);

    for(int i = 0; i < n_files; i++)
        free(fileinfo[i].filename);

    free(fileinfo);


    gdImageDestroy(in_texture_img);
    
    free(start_time_par);

    free(end_time_par);

    free(par_time);

    //STOPS SEQUENCIAL TIME KEEPING AND STORES TIME DATA
    clock_gettime(CLOCK_MONOTONIC, &end_time_total);
    clock_gettime(CLOCK_MONOTONIC, &end_time_seq);

    struct timespec seq_time = diff_timespec(&end_time_seq, &start_time_seq);
    struct timespec total_time = diff_timespec(&end_time_total, &start_time_total);
    fprintf(fp,"\tseq \t %10jd.%09ld\n", seq_time.tv_sec, seq_time.tv_nsec);
    fprintf(fp,"total \t %10jd.%09ld\n", total_time.tv_sec, total_time.tv_nsec);

    fclose(fp);
    
    return 0;
}
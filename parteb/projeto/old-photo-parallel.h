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
    gdImagePtr in_texture_img;
    char *directory;
} thread_args;

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

    gdImagePtr in_img;
    gdImagePtr out_smoothed_img;
    gdImagePtr out_contrast_img;
    gdImagePtr out_textured_img;
    gdImagePtr out_sepia_img;

    for (int i = t_args->lower_limit; i < t_args->upper_limit; i++) { 

        if(!t_args->fileinfo[i].exists) {    
            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", t_args->directory, t_args->fileinfo[i].filename);
            printf("FILE: %s\n", t_args->fileinfo[i].filename);

            in_img = read_jpeg_file(fullpath);
            if (in_img == NULL) {
                fprintf(stderr, "Impossible to read %s image\n", fullpath);
                continue;
            }

            out_contrast_img = contrast_image(in_img);
            out_smoothed_img = smooth_image(out_contrast_img);
            out_textured_img = texture_image(out_smoothed_img, t_args->in_texture_img);
            out_sepia_img = sepia_image(out_textured_img);

            sprintf(out_file_name, "%s%s", "./old_photo_PAR_A/", t_args->fileinfo[i].filename);
            if(write_jpeg_file(out_sepia_img, out_file_name) == 0) {
                fprintf(stderr, "Impossible to write %s image\n", out_file_name);
            }
            gdImageDestroy(out_smoothed_img);
            gdImageDestroy(out_sepia_img);
            gdImageDestroy(out_contrast_img);
            gdImageDestroy(in_img);
        }
        else
            continue;
    }

    return NULL;
}

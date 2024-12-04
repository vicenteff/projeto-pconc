#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>

#define DIRECTORY argv[1]
#define THREADS argv[2]
#define ORDER argv[3]

typedef struct
{
    char *filename;
    long int size;
    int exists;
} Fileinfo;

int compare_by_name(const void *a, const void *b) {
    Fileinfo *file1 = (Fileinfo *)a;
    Fileinfo *file2 = (Fileinfo *)b;
    return strcmp(file1->filename, file2->filename);
}

int compare_by_size(const void *a, const void *b) {
    Fileinfo *file1 = (Fileinfo *)a;
    Fileinfo *file2 = (Fileinfo *)b;
    return file1->size - file2->size;
}

char *get_extension(const char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot;
}

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

int listdir(char *directory, char *extension, Fileinfo *fileinfo) {
  DIR *d;
  int i = 0;
  struct dirent *f;
  d = opendir(directory);
  if (d) {
    while (((f = readdir(d)) != NULL)) {
        if(!strcmp(get_extension(f->d_name), extension)) {
            //printf("%s ", f->d_name);
            strcpy(fileinfo[i].filename, f->d_name);

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", directory, f->d_name); //ADDS FILE PATH TO THE NAME

            struct stat st;
            stat(f->d_name, &st);
            //printf("%ld\n", st.st_size);
            fileinfo[i].size = st.st_size;

            i++;
        }
    }
    closedir(d);
  }
  return(0);
}

int sort_fileinfo(Fileinfo *fileinfo, char *order, int n_files) {

    if(!strcmp(order, "-name")) {
        qsort(fileinfo, n_files, sizeof(Fileinfo), compare_by_name);
    }
    else if(!strcmp(order, "-size")) {
        qsort(fileinfo, n_files, sizeof(Fileinfo), compare_by_size);
    }
    else    
        return -1;

    return 0;

}

int applyfilter() {
    
}
int main(int argc, char *argv[]) {

/*********************************************************************************/
/*PROGRAM ARGUMENTS                                                              */
/*********************************************************************************/

    char *dir = malloc(100 * sizeof(char));
    char *order = malloc(100 * sizeof(char));
    int threads, n_files;

    strcpy(dir, DIRECTORY);
    threads = atoi(THREADS);
    strcpy(order, ORDER);

    if(threads < 1) {
        perror("INVALID NUMBER OF THREADS\n");
        exit(1);
    }

    if((strcmp(order, "-name") != 0) && (strcmp(order, "-size") != 0)) {
        perror("INVALID ORDERING CRITERION\n");
        exit(2);
    }
/*********************************************************************************/

/*********************************************************************************/
/*FILE SORTING AND STORING                                                       */
/*********************************************************************************/
    n_files = filecount(DIRECTORY, ".jpeg");

    Fileinfo *fileinfo = malloc(n_files * sizeof(Fileinfo));
    for (int i = 0; i<n_files; i++)
        fileinfo[i].filename = malloc(100 * sizeof(char));

    listdir(DIRECTORY, ".jpeg", fileinfo);

    sort_fileinfo(fileinfo, ORDER, n_files);

/*********************************************************************************/

    /*for(int i = 0; i<n_files; i++) {
        printf("name: %s size: %ld \n", fileinfo[i].filename, fileinfo[i].size);
    }*/


    //printf("IN DIRECTORY: %s\nNUMBER OF THREADS: %d\nORDER BY: %s\n", dir, threads, order);

/*********************************************************************************/
/*FILE CHECKER                                                                   */
/*********************************************************************************/

char *path = malloc(100 * sizeof(char));

for(int i = 0; i<n_files; i++) {

    strcpy(path, "./old_photo_PAR_A/");
    strcat(path, fileinfo[i].filename);
    
    if( access(path, F_OK ) != -1){
        printf("%s EXISTE ✅\n", fileinfo[i].filename);
        fileinfo[i].exists = 1;
    }else{
        printf("%s NAO EXISTE ❌\n", fileinfo[i].filename);
        fileinfo[i].exists = 1;
    }

}
/*********************************************************************************/

/*********************************************************************************/
/*THREAD MANAGMENT                                                               */
/*********************************************************************************/

    pthread_t thread_id;

    //for(int i = 0; i < THREADS; i++);
     //   pthread_create(&thread_id, NULL, apllyfilter, NULL);


/*********************************************************************************/

/*CREATES RESULTS FOLDER                                                         */ 

    mkdir("old_photo_PAR_A", 0777);

}
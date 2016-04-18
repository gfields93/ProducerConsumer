//
//  main.c
//  OS_Project3
//  Glen Fields / U48602491
//  Created by Glen Fields on 10/26/15.
//
//

#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#define SHAREDMEM 1000
#define BUFFERSIZE 15
void * Producer(void *);
void * Consumer();
void DestroySemaphores();
void CreateSemaphores();

/*-------------------------------------------------------------------------*/
//Global variables
typedef struct myBuffer{
    char buf[BUFFERSIZE];
    unsigned bufItems;
    bool eofFlag;
} Buffer;
Buffer * buffer;
sem_t * mutex;
sem_t * full;
sem_t * empty;
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int main(int argc, const char * argv[]) {
    int shmid; //buffer id
    int thread1,thread2;
    pthread_t consumerThread,producerThread;
/*-------------------------------------------------------------------------*/
    //  create shared memory
    shmid = shmget(SHAREDMEM, sizeof(Buffer), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Error Creating Shared Memory");
        return 1;
    }
    buffer = (Buffer *) shmat(shmid, NULL, 0);
    if(buffer == (Buffer *)-1){
        perror("Error attaching Shared Memory");
        return 1;
    }

    CreateSemaphores();

    buffer->bufItems = 0;
    for (int i = 0; i < BUFFERSIZE; ++i) {
        buffer->buf[i] = '\0';
    }
    thread1 = pthread_create(&producerThread, NULL, Producer, (void*)argv);
    if (thread1 != 0) {
        perror("Error creating Producer Thread");
        return 1;
    }
    thread2 = pthread_create(&consumerThread, NULL, Consumer, NULL);
    if (thread2 != 0) {
        perror("Error creating Consumer Thread");
        return 1;
    }
    
    pthread_join(producerThread, NULL);
    pthread_join(consumerThread, NULL);

    DestroySemaphores();
    
    //    delete shared memory
    if(shmctl(shmid, IPC_RMID, NULL) < 0){
        perror("Error detaching Shared Memory");
        return 1;
    };
    
    return 0;
}
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Producer and Consumer function definitions */
void * Producer(void * arguments){
    char ** string = (char**)arguments; 
    int newChar;
    unsigned int i = 0;
    FILE * fp;
    fp = fopen(string[1], "r");
    if (fp == NULL) {
        printf("File could not be opened.\n");
        buffer->eofFlag = true;
        exit(1);
    }
    buffer->eofFlag = false;
    newChar = fgetc(fp);
    while (newChar != EOF) {
        sem_wait(empty);
        sem_wait(mutex);
        buffer->buf[i%BUFFERSIZE] = newChar;
        buffer->bufItems++;
        sem_post(mutex);
        sem_post(full);
        ++i;
        newChar = fgetc(fp);
    }
    buffer->eofFlag = true;
    fclose(fp);
    return NULL;
}
void * Consumer(){
    unsigned int i = 0;
    while (1) {
        if (buffer->eofFlag == true && buffer->bufItems == 0) {
            break;
        }
        else{
            sem_wait(full);
            sem_wait(mutex);
            printf("%c",buffer->buf[i%BUFFERSIZE]);
            buffer->buf[i%BUFFERSIZE] = '\0';
            buffer->bufItems--;
            sem_post(mutex);
            sem_post(empty);
            ++i;
            sleep(1);
            fflush(stdout);
        }
    }
    return NULL;
}
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* functions to create and  destroy semaphores */
void DestroySemaphores(void) {
    //    destroy semaphores
    if (sem_close(mutex) < 0) {
        perror("Error destroying Semaphore Mutex");
    }
    if (sem_close(full) < 0) {
        perror("Error destroying Semaphore Full");
    }
    if (sem_close(empty) < 0) {
        perror("Error destroying Semaphore Empty");
    }
    sem_unlink("/sem1");
    sem_unlink("/sem2");
    sem_unlink("/sem3");
}
void CreateSemaphores(void){
    mutex = sem_open("/sem1", O_CREAT | O_EXCL, 0666, 1);
    if (mutex == SEM_FAILED) {
        sem_close(mutex);
        sem_unlink("/sem1");
        mutex = sem_open("/sem1", O_CREAT, 0666, 1);
    }
    full = sem_open("/sem2", O_CREAT | O_EXCL, 0666, 0);
    if (full == SEM_FAILED) {
        sem_close(full);
        sem_unlink("/sem2");
        full = sem_open("/sem2", O_CREAT, 0666, 0);
    }
    empty = sem_open("/sem3", O_CREAT | O_EXCL, 0666, (int)BUFFERSIZE);
    if (empty == SEM_FAILED) {
        sem_close(empty);
        sem_unlink("/sem3");
        empty = sem_open("/sem3", O_CREAT, 0666, (int)BUFFERSIZE);
    }
}


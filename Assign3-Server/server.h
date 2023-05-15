//Isaac Ferguson
//Assingment 3 networked spell checker
//10/12/21

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>        
#include <arpa/inet.h>
#include <unistd.h>


#define _GNU_SOURCE

int N_THREADS;

#define BUFMAX 60
#define DEFAULT_PORT 9154
#define DEFAULT_DICTIONARY "dictionary.txt"
#define DICTIONARY_BUFF 99171
#define DEFAULT_CONN_BUFF_SIZE 20
#define DEFAULT_WORK_THREADS 10


void* workerThread(void*);
void* logThread(void*);

int addSock(int);
int removeSock();

int addLogQueue(char*, char*, int);
struct logData removeLogQueue();

//array for socket descriptors
int buffer[DEFAULT_CONN_BUFF_SIZE] = {0};
int readLoc, writeLoc, newDict = 1;
char diction[50];


//circle queue for socket connections
struct circleQueue{
    int addSocket;
    int removeSocket;
	int size;
};

//info sent ot log 
struct logData{
	char respose[30];
	char word[30];
	int sock;
};

//fifo queue for the log
struct logQueue{
	struct logData queue[BUFMAX];
	int size;
};



pthread_mutex_t sock_lock, log_lock;
pthread_cond_t sock_full, sock_empty, log_full, log_empty; 
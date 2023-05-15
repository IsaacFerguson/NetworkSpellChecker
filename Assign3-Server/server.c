//Isaac Ferguson
//Assingment 3 networked spell checker
//10/12/21
#include "server.h"

struct circleQueue socketQueue;
struct logQueue serverLog;
int main(int argc, char *argv[]){
    size_t i;
	//sets original values for main socket queue
	socketQueue.addSocket = 0;
	socketQueue.removeSocket = 0;
	socketQueue.size = 0;
	int port;
	

	//if cmd line ags are given set them order is port, dictionary
	if(argc == 3){
		port = atoi(argv[1]);
		strcpy(diction, argv[2]);
		newDict = 0;
	}
	else if(argc == 2){
		if(strstr(argv[1], "txt")){
			strcpy(diction, argv[1]);
			newDict = 0;
		} else{
			port = atoi(argv[1]);
		}
	}
	else{
		port = DEFAULT_PORT;
		strcpy(diction, DEFAULT_DICTIONARY);
	}

	//initalize cond and mutex
	pthread_mutex_init(&sock_lock, NULL);
	pthread_mutex_init(&log_lock, NULL);
	pthread_cond_init(&sock_full, NULL);
	pthread_cond_init(&sock_empty, NULL);
	pthread_cond_init(&log_full, NULL);
	pthread_cond_init(&log_empty, NULL);

    //spawn worker threads
    pthread_t threads[DEFAULT_WORK_THREADS];
    for(i = 0; i < DEFAULT_WORK_THREADS; i++){//makes the specified amount of workers
        if(pthread_create(&threads[i], NULL, workerThread , NULL) != 0){
            printf("Error failed to make thread\n");
            exit(1);
        }
    }

    //spawn the log thread
    pthread_t log;
    pthread_create(&log, NULL, logThread, NULL);


    //create lis socket descriptor
    int socket_desc;

    struct sockaddr_in server;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc == -1){
        printf("Error making socket\n");
    }

    //using exmaple curr need to change = vals
	
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
	
	//bind socket 
	if(bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("error binding socket");
	}

	//sets program to listen
    listen(socket_desc, 3);


    //main loop
    while(1){
        //trys to acc new socket
       int newSocket = accept(socket_desc, 0, 0);


        //if there is no conneciton try again at new socket
       if(newSocket == -1){
           printf("No connection made\n");
           continue;
       }

        //if buffer is full wait until opening
	
       pthread_mutex_lock(&sock_lock);
	   if(socketQueue.size == DEFAULT_CONN_BUFF_SIZE){
           pthread_cond_wait(&sock_empty,&sock_lock);
       }

        //add new socket to buffer
       if(addSock(newSocket) == -1){
           printf("error adding to buffer\n");
           continue;
       }

	   pthread_cond_signal(&sock_full);
	   pthread_mutex_unlock(&sock_lock);
	   //printf("mainend\n");
    }
	return 0;
}


void* workerThread(void* arg){
	//opens the dictionary and puts into a char**

	char **dict = malloc(DICTIONARY_BUFF * sizeof(char*) + 1);
	FILE* f;
	char dLine[100];
	f = fopen(DEFAULT_DICTIONARY, "r");//change to accept specififed dict
	if(f == NULL){
		perror("No dictionary found\n");
	}
	int q = 0;
	//convets the file to a char**
	while(fgets(dLine, 100, f) != NULL){
		dict[q] = (char*) malloc(strlen(dLine)* sizeof(char*) + 1);
		strcpy(dict[q], dLine);

		q++;
	}
	fclose(f);


    
    while(1){
        
		pthread_mutex_lock(&sock_lock);
		//if buffer is empty wait until there is a vaule
    	if(socketQueue.size == 0){
        	pthread_cond_wait(&sock_full, &sock_lock);
    	}

		//remove wanted socket
		int currSock = removeSock();
		pthread_cond_signal(&sock_empty);
		pthread_mutex_unlock(&sock_lock);

		char workBuff[BUFMAX] = "\0";
		
        //keeps reading until out of words
        while(recv(currSock, workBuff, sizeof(workBuff), 0) > 0){
			
            char toSend[10];
            int wasFound = 1;
			int k = 0;

			/*
    		char line[100];
			strcpy(line, workBuff); */

			/*
			for(k = 0; k < DICTIONARY_BUFF; k++){
				fprintf(test, "%s",dict[k]);
			}
			fclose(test);
			k = 0;  */

			//if connection is ended
			if(strcmp(workBuff, "999") == 10){//should be zero
				send(currSock, "THANKYOU", sizeof("THANKYOU"), 0);
				close(currSock);
				break;
			}

			//if no word is sent
			if(strcmp(workBuff, "\n") == 0){
				send(currSock, "No Word Sent", sizeof("No Word Sent"), 0);

			} else{//look through the dict for the word

    			for(k = 0; k < DICTIONARY_BUFF; k++){
				
        			if(strcasecmp(workBuff, dict[k]) == 0){
						wasFound = 0;
						break;
					}
				}
				k = 0;

				//writes ok or mispelled based on word
            	if(wasFound == 0){
                	strcpy(toSend, "OK");
            	}
            	else{
                	strcpy(toSend, "MISPELLED");
            	}//need to add quit 

				//printf("%s\n", toSend);

            	//send answer to client and add response to logQueue
            	send(currSock, toSend, sizeof(toSend), 0);
    		}

			
            

			//waits for the log to have an open space and then adds event to log queue
			pthread_mutex_lock(&log_lock);
			while(serverLog.size == BUFMAX){
				pthread_cond_wait(&log_empty, &log_lock);
			}
            addLogQueue(toSend, workBuff, currSock);
			pthread_cond_signal(&log_full);
			pthread_mutex_unlock(&log_lock);
			//printf("lastwork\n");
        }

		return 0;
    }
    



}

void* logThread(void* arg){
    FILE* logFile;
	logFile = fopen("logFile", "w");
	if(logFile == NULL){
		perror("error opening log file\n");
	}


	//main loop
	while(1){
		//lock and wait for server log to not be empty
		pthread_mutex_lock(&log_lock);
		if(serverLog.size == 0){
			pthread_cond_wait(&log_full, &log_lock);
		}
		struct logData addLog;

		//removes first log entry
		addLog = removeLogQueue();
		
		if(addLog.sock == 0){
			perror("error retriving log data\n");
		}

		pthread_mutex_unlock(&log_lock);
		pthread_cond_signal(&log_empty);
		

		//prints the log data to the log file
		pthread_mutex_lock(&log_lock);

		fprintf(logFile,"Word: %s  Response: %s  Socket %d\n", addLog.word, addLog.respose, addLog.sock);
		
		
		pthread_cond_signal(&log_empty);
		pthread_mutex_unlock(&log_lock);

	
	}
	fclose(logFile);
}

int addSock(int socket){

	//add socket to buffer at current add location
    buffer[socketQueue.addSocket] = socket;


	//if the add pointer is at the end of the circle queue set it to start
    if(socketQueue.addSocket == DEFAULT_CONN_BUFF_SIZE - 1){
        socketQueue.addSocket = 0;

    }else{//increase by 1 from current loaction
        socketQueue.addSocket++;
    }
	socketQueue.size++;
    return 0;
}

int removeSock(){
    int sock;
    //if there is noting in the buffer
    if(socketQueue.size == 0){
        return -1;
    }

	//take socket at current revmove loaction
    sock = buffer[socketQueue.removeSocket];

	//if the remove pointer is at the end of the circle queue set it to start
    if(socketQueue.removeSocket == DEFAULT_CONN_BUFF_SIZE -1){
        socketQueue.removeSocket = 0;
    }else{
        socketQueue.removeSocket++;
    }
	return sock;
}

int addLogQueue(char *response, char* word, int sock){//FIFO queue not Priority

	//if the log queue is full must wait
	if(serverLog.size == BUFMAX){
		return 1;
	}

	//create the data to add 
	struct logData toAdd;

	//set the values of toAdd to the given values
	strcpy(toAdd.respose, response);
	strcpy(toAdd.word, word);
	toAdd.sock = sock;

	//enter the data into the queue
	serverLog.queue[serverLog.size] = toAdd;
	serverLog.size++;
	return 0;
}

struct logData removeLogQueue(){
	int i;
	struct logData toSend;
	toSend.sock = 0;

	if(serverLog.size == 0){//if the log is empty send blank data
		return toSend;
	}

	//takes first data in queue
	toSend = serverLog.queue[0];
	for(i = 0; i < serverLog.size - 1; i++){
		serverLog.queue[i] = serverLog.queue[i + 1];
	}
	serverLog.size--;
	return toSend;
}




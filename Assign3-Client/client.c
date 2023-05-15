#include "client.h"



int main( int argc, char *argv[]){

    //if port is given us otherwise use default port must have ip add to work
    int port = DEFAULT_PORT;


	/*
    if(argc == 3){
        port = atoi(argv[2]);
		twoarg = 0;

    }else if(argc == 2){
        if(!strstr(argv[1], ".")){
			printf("no ip given");
			return -1;
		}

    } else{
		printf("No ip give\n");
		return -1;
	}
	*/

    char *word = NULL;
	char reply[20];
    int sock;
    struct sockaddr_in serv_addr;
	size_t len = 0;

    
    //creating the socket
    if((sock = socket(AF_INET, SOCK_STREAM,0)) == -1){
		printf("error creating socket\n");
        return -1;
    }

    //setting socket values
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    //convert address to binary
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
		printf("error converting address\n");
        return -1;
		
    }

    //makes connection
    if(connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        printf("error making connection\n");
		return -1;
    }


	while(1){

		printf("Word to Check: ");
		getline(&word, &len, stdin);

		//looked up how to remove whitespaces
		/*
		int i, j, leng;
		leng = sizeof(word)/word[0];

		for(i = 0; i < leng; i++){
			if(word[i] == ' '){
				for(j = i; j < leng; j++){
					word[j] = word[j + 1];
				}
				leng--;
			}
		} */


		//if the quit cmd is used end connection
		if(strcmp(word, "999") == 10){ //should be 0 but for some reason does not work with zero
			send(sock, &word, sizeof(word), 0);
			close(sock);
			break;
		}


		//send word to server
    	if(send(sock, word, sizeof(word), 0) < 0){
			printf("error sending word\n");
		}

    	//gets output from server
    	if(recv(sock, reply, 20, 0) < 0){
			printf("error reciving\n");
		}

    	printf("%s\n", reply);
	}
    
    

	return 0;
}
#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <arpa/inet.h>    //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include "msg.h"
#include <pthread.h>
#include <assert.h>
     
#define TRUE   1 
#define FALSE  0 
#define MAX_BUFFER 1024
#define MAX_DB_SIZE 1000

typedef struct msg message;
struct record db[MAX_DB_SIZE];
int db_iterator = 0;
char buffer[MAX_BUFFER]; 

void * HandleClient(void *);

void Pthread_create(pthread_t *t, const pthread_attr_t *attr,
            void *(*start_routine)(void *), void *arg) {
    int rc = pthread_create(t, attr, start_routine, arg);
    assert(rc == 0);
}

void Pthread_join(pthread_t thread, void **value_ptr) {
    int rc = pthread_join(thread, value_ptr);
    assert(rc == 0);
}

char* itoa(int input){
    static char arr[32] = {0};
    int num = 30;
    for(int num = 30; input && num ; --num, input /= 10)
        arr[num] = "0123456789abcdef"[input % 10];
    return &arr[num+1];
}

void * HandleClient(void * client) {
	FILE * fp;
	fp = fopen("db.txt", "a+");
	int found = 0;	
	int sd = *(int *)client;
	message received_msg;
	memcpy(&received_msg, buffer, sizeof(received_msg));	
	//printf ("name : %s\n", received_msg.rd.name);
    //printf ("id : %d\n", received_msg.rd.id);
	if (received_msg.type == 1) {
		printf("Saving data to database.\n");
    	strncpy(db[db_iterator].name, received_msg.rd.name, 127);
		db[db_iterator].name[127] = '\0';
		//printf("Stored name: %s", db[db_iterator].name);
    	db[db_iterator].id = received_msg.rd.id;
    	db_iterator++;
    	fputs(received_msg.rd.name, fp);
        fputs(itoa(received_msg.rd.id), fp);
        fflush(fp);
    }
    else if (received_msg.type == 2) {
    	printf("Fetching data from database.\n");
    	for (int i = 0; i < db_iterator; i++) {
    		if (db[i].id == received_msg.rd.id) {
        		message client_msg;
        		client_msg.type = 3;
        		client_msg.rd.id = db[i].id;
        		strcpy(client_msg.rd.name,db[i].name);
        		write(sd, &client_msg, sizeof(client_msg));
        		found = 1;
        		break;
            }
        }
        if (found == 0) {
        	message client_msg;
        	client_msg.type = 4;
        	client_msg.rd.id = received_msg.rd.id;
            strcpy(client_msg.rd.name,"NO MATCH");
            write(sd, &client_msg, sizeof(client_msg));
        }
  	}
   	memset(&received_msg, 0, sizeof(received_msg));
	fclose(fp);
	pthread_exit(NULL);
	return (void *) 1;
}

int main(int argc , char *argv[])  
{  
    if (argc < 2) {
		printf("dbserver PORT\n");
		return 1;
	}
	int PORT = atoi(argv[1]);
	int opt = TRUE;  
    int master_socket , addrlen , new_socket , client_socket[100] , 
          clients = 100, activity, i , valread , sd;  
    int max_sd;  
    struct sockaddr_in address;  
         
         
    //set of socket descriptors 
    fd_set readfds;  
         
    //initialise all indices of client_socket to 0 so it is not checked 
    for (i = 0; i < clients; i++)  
    {  
        client_socket[i] = 0;  
    }  
         
    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
     
    //set master socket to allow multiple connections
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
         
    //bind the socket to the port 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
         
    if (listen(master_socket, 3) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  
         
    while(TRUE)  
    {  
        //clear the socket set 
        FD_ZERO(&readfds);  
     
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
             
        //add child sockets to set 
        for ( i = 0 ; i < clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                 
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
     
        //wait as long as needed for activity on one of the sockets 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
       
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
        // handling incoming connection     
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
             
            // print socket number 
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
			 new_socket , inet_ntoa(address.sin_addr) , ntohs (address.sin_port));  
            //add new socket to array of sockets 
            for (i = 0; i < clients; i++)  
            {  
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                    break;  
                }  
            }  
        }  
           
        //must be some IO operation on some other socket
        for (i = 0; i < clients; i++)  
        {  
            sd = client_socket[i];  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                // check if it was for closing 
                if ((valread = read( sd , buffer, sizeof(buffer))) == 0)  
                {  
                    // connection details of disconnected client 
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Client disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    // close the socket, mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                }  
                else {  
                    // set the string terminating NULL byte on the end 
                    buffer[valread] = '\0';  
					pthread_t thread;
					Pthread_create(&thread, NULL, HandleClient, &sd);
			        Pthread_join(thread, NULL);
	            }  
            }  
        }  
    }  
    return 0;  
}  

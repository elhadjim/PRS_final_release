#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

#define BUFFERSIZE 1494
#define ACKSIZE 7
#define MAXIMUM_SEGMENT_SIZE 1500 //BUFFERSIZE + ACKSIZE
#define MSG_CONTROL_SIZE 20
#define FILENAME_SIZE 255

#define PORTDATA 8080



int sizeFile(const char * filename);


int main(int argc, char *argv[]){

    
    printf("\n========================= Server Scenario 1 =========================\n");

    int socket_control;
    int socket_data;
    struct sockaddr_in server, client;
    socklen_t server_size = sizeof(server);
    socklen_t client_size = sizeof(client);
    int port;
    int valid = 1;
    int msgSize;
    char *syn_ack_msg = "SYN-ACK8080";
    char *end_msg = "FIN";
    char rcv_msg_control[MSG_CONTROL_SIZE];
    char rcv_msg_requested_file[FILENAME_SIZE];

    char buffer[BUFFERSIZE];
    char ack[ACKSIZE];
    char segment[MAXIMUM_SEGMENT_SIZE];

    if (argc <2){
        printf("Execution : ./server1 <port_number> \n");
        exit(-1);
    }
    port =  atoi(argv[1]);

    // Creating the socket control

    socket_control = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_control  < 0){
        printf("Echec de la creation du socket controle \n");
        exit(-1);
    }

    setsockopt(socket_control, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(socket_control, (struct sockaddr*) &server, sizeof(server)) == -1) {
		printf("Echec du processus de bind du socket controle\n");
        exit(-1);
	}

    // 3way handshake processus

    printf("\nDebut du processus de 3way handshake ... \n\n");
    
    msgSize = recvfrom(socket_control,rcv_msg_control,MSG_CONTROL_SIZE, 0, (struct sockaddr *) &client, &client_size);
    rcv_msg_control[msgSize] = '\0';
    printf("\nServer recoit : %s de %s:%d\n", rcv_msg_control, inet_ntoa(client.sin_addr),ntohs(client.sin_port));

    sendto(socket_control,(const char *) syn_ack_msg, strlen(syn_ack_msg), 0, (const struct sockaddr *)&client, client_size);
    printf("\nServer envoit: %s\n",syn_ack_msg);

    msgSize = recvfrom(socket_control,rcv_msg_control,MSG_CONTROL_SIZE, 0, (struct sockaddr *) &client, &client_size);
    rcv_msg_control[msgSize] = '\0';
    printf("\nServer recoit : %s de %s:%d\n", rcv_msg_control, inet_ntoa(client.sin_addr),ntohs(client.sin_port));

    

    // Creating the socket data

    socket_data = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_data <0){
        printf("Echec de la creation du socket data \n");
        exit(-1);
    }

    setsockopt(socket_data, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
    server.sin_port = htons(PORTDATA);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    

    if(bind(socket_data, (struct sockaddr*) &server, sizeof(server)) < 0){
        printf("Echec du processus de bind du socket data \n");
        exit(-1);
    }

    // Reception of the requested file

    msgSize = recvfrom(socket_data,rcv_msg_requested_file,FILENAME_SIZE, 0, (struct sockaddr *) &client, &client_size);
    rcv_msg_requested_file[msgSize] = '\0';
    char *filename = rcv_msg_requested_file;

    //Lecture du fichier demandé par le client

    printf("Le fichier demandé par le client : %s\n", filename);
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL){
        printf("Echec d'ouverture de fichier \n");
        exit(-1);
    }

    // Variables utilisees pour la fenetre
    
    int seg_number = 0;
    int window_size = 20;
    int ack_received;
    int b_fread;
    long int number_of_fragment = sizeFile(filename)/BUFFERSIZE +1;
    int size_of_last_segment;
    fd_set desc;
    
    

    struct timeval debut,fin,timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 700;
 
    int duplicates_ack;
    
    //construction de packet

    typedef struct pacquet{

        char data[BUFFERSIZE];

    } pkt;
    
    pkt* packet=malloc(sizeof(struct pacquet)*number_of_fragment);
    
    int j=0;
    for (int i=1; i<=number_of_fragment; i++){
        if((b_fread = fread(buffer, sizeof(char), BUFFERSIZE, fp))> 0){
            memcpy(packet[i].data,buffer,BUFFERSIZE);
            size_of_last_segment = b_fread;
            
        }
    }
    gettimeofday(&debut, NULL);
    while (seg_number <= number_of_fragment){

        
        for (long int i = seg_number+1; i <=window_size+seg_number && i<=number_of_fragment; i++){
            
            
            sprintf(ack, "%06ld" ,i);
            memcpy(&segment[0], ack, ACKSIZE);
            
            if(i == number_of_fragment){
                memcpy(&segment[6], packet[i].data, size_of_last_segment+6);
                sendto(socket_data, segment, size_of_last_segment+6, 0, (struct sockaddr *)&client, client_size);
            }
            else{
                memcpy(&segment[6], packet[i].data, MAXIMUM_SEGMENT_SIZE);
                sendto(socket_data, segment, MAXIMUM_SEGMENT_SIZE, 0, (struct sockaddr *)&client, client_size);
            }
        }
        
    
        FD_SET(socket_data, &desc);
        select(socket_data+1, &desc, NULL, NULL,&timeout);
        if(FD_ISSET(socket_data, &desc)!=0){
            
            msgSize = recvfrom(socket_data,rcv_msg_control,MSG_CONTROL_SIZE,0, (struct sockaddr *)&client, &client_size);
            rcv_msg_control[msgSize] = '\0';
            ack_received = atoi(&rcv_msg_control[3]);
            
            if (ack_received > seg_number){
                seg_number = ack_received+1;
                duplicates_ack = 0;
            }else if (ack_received == seg_number+window_size){
                seg_number = ack_received +1;
                window_size=window_size*2;
                duplicates_ack = 0;
                
            }
            else if(ack_received == seg_number){
                
                duplicates_ack++;

                if (duplicates_ack == 3){//FASTRETRANSMIT
                    
                    sprintf(ack, "%06d" ,ack_received+1);
                    memcpy(&segment[0], ack, ACKSIZE);
                
                    if(ack_received+1 == number_of_fragment){
                        memcpy(&segment[6], packet[ack_received+1].data, size_of_last_segment+6);
                        sendto(socket_data, segment, size_of_last_segment+6, 0, (struct sockaddr *)&client, client_size);
                    }
                    else{
                        memcpy(&segment[6], packet[ack_received+1].data, MAXIMUM_SEGMENT_SIZE);
                        sendto(socket_data, segment, MAXIMUM_SEGMENT_SIZE, 0, (struct sockaddr *)&client, client_size);
                    }

                }
                seg_number = ack_received+1;
            }
        
        }else{//timeout on retransmet le paquet et on diminue la taille de la fenêtre

            sprintf(ack, "%06d" ,seg_number);
            memcpy(&segment[0], ack, ACKSIZE);
            
            if(seg_number == number_of_fragment){
                memcpy(&segment[6], packet[seg_number].data, size_of_last_segment+6);
                sendto(socket_data, segment, size_of_last_segment+6, 0, (struct sockaddr *)&client, client_size);
            }
            else{
                memcpy(&segment[6], packet[seg_number].data, MAXIMUM_SEGMENT_SIZE);
                sendto(socket_data, segment, MAXIMUM_SEGMENT_SIZE, 0, (struct sockaddr *)&client, client_size);
            }
            
            
            
        }
        
        
    }
    gettimeofday(&fin, NULL);
    long double elapsed_time = fin.tv_sec - debut.tv_sec + 0.000001* (fin.tv_usec - debut.tv_usec) ;
    printf("Taille du fichier %d\n",sizeFile(filename));
    long double debit = sizeFile(filename)/elapsed_time;
    printf("\nTaille d'un segment : %d\n", BUFFERSIZE);
    printf("Nombre de fragments %ld\n", number_of_fragment);
    printf("Le debit est %.3Lf Mo/s \n", debit/1048576);
    free(packet);
    sendto(socket_control,(const char *) end_msg, strlen(end_msg), 0, (const struct sockaddr *)&client, client_size);
    printf("\nLe Server envoit : %s\n",end_msg);
    
    close(socket_data);
    close(socket_control);

}


int sizeFile(const char * filename){
    
    FILE * f;
    long int size = 0;
    f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);

    size = ftell(f);

    if (size != -1)
        return(size);
    return 0;
 }

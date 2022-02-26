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
int max (int a, int b);


int main(int argc, char *argv[]){

    
    printf("\n========================= Server Scenario 3 =========================\n");

    int socket_control;
    int socket_data;
    struct sockaddr_in server, client;
    socklen_t server_size = sizeof(server);
    socklen_t client_size = sizeof(client);
    int port;
    int valid = 1;
    int msgSize;
    char syn_ack_msg[20] = "SYN-ACK";
    char *end_msg = "FIN";
    char rcv_msg_control[MSG_CONTROL_SIZE];
    char rcv_msg_requested_file[FILENAME_SIZE];

    char buffer[BUFFERSIZE];
    char ack[ACKSIZE];
    char segment[MAXIMUM_SEGMENT_SIZE];

    
    fd_set readfds_conn;
    
    
    fd_set readfds_trans;
    

    if (argc <2){
        printf("Execution : ./server1 <port_number> \n");
        exit(-1);
    }
    port =  atoi(argv[1]);

    // Creating the socket control

    socket_control = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_control  < 0){
        printf("Echec de la creation du socket data \n");
        exit(-1);
    }

    setsockopt(socket_control, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(socket_control, (struct sockaddr*) &server, sizeof(server)) == -1) {
		printf("Echec du processus de bind du socket controle \n");
        exit(-1);
	}

    // 3way handshake processus

    typedef struct pacquet{

        char data[BUFFERSIZE];

    } pkt;
    
    int port_d=8080;

    while(1){

        FD_ZERO(&readfds_conn);
        FD_SET(socket_control, &readfds_conn);
        
        int socket_activity = select(socket_control+1, &readfds_conn, NULL, NULL,NULL);
        if (socket_activity < 0){
            printf("Erreur du select \n");
            exit(-1);
        }
        
        if (FD_ISSET(socket_control, &readfds_conn)){

            recvfrom(socket_control,rcv_msg_control,MSG_CONTROL_SIZE, 0, (struct sockaddr *) &client, &client_size);
            //rcv_msg_control[msgSize] = '\0';
            //printf("\n <==> SERVER RECEIVE : %s FROM %s:%d\n", rcv_msg_control, inet_ntoa(client.sin_addr),ntohs(client.sin_port));
            port_d = port_d+1;
            sprintf(syn_ack_msg +7, "%d", port_d);
            //char *syn_ack_msg = strcat("SYN-ACK", port_s);
            
            sendto(socket_control,(const char *) &syn_ack_msg, strlen(syn_ack_msg), 0, (const struct sockaddr *)&client, client_size);
            
            struct sockaddr_in server_udp,cli;
            memset((char*)&server_udp, 0, sizeof(server_udp));

            socket_data = socket(AF_INET, SOCK_DGRAM, 0);
            if(socket_data <0){
                printf("FAILURE CREATING SOCKET DATA \n");
                exit(-1);
            }

            setsockopt(socket_data, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
            //int p = port_d + PORTDATA;
            server_udp.sin_port = htons(port_d);
            server_udp.sin_family = AF_INET;
            server_udp.sin_addr.s_addr = htonl(INADDR_ANY);
            
            
            //struct sockaddr_in cli;
            

            if(bind(socket_data, (struct sockaddr*) &server_udp, sizeof(server_udp)) < 0){
                printf("Echec du processus de bind du socket data \n");
                exit(-1);
            }

            // Reception of the requested file
            //struct sockaddr init_udp;
            socklen_t cdp = sizeof(cli);
 
            
            msgSize = recvfrom(socket_control,rcv_msg_control,MSG_CONTROL_SIZE, 0, (struct sockaddr *) &cli, &client_size);
            rcv_msg_control[msgSize] = '\0';
            
            int child = fork();
            if(child > 0){
                close(socket_data);
            }
            else if(child == 0){
                close(socket_control);  
                msgSize = recvfrom(socket_data,rcv_msg_requested_file,FILENAME_SIZE, 0, (struct sockaddr *) &cli, &cdp);
                rcv_msg_requested_file[msgSize] = '\0';
                char *filename = rcv_msg_requested_file;

                //Lecture du fichier demandé par le client

                
                FILE *fp = fopen(filename, "rb");
                if(fp == NULL){
                    printf("Echec ouverture du fichier \n");
                    exit(-1);
                }
                
                
                
                // Variables utilisees pour la fenetre
                
                long int seg_number = 0;
                int window_size = 20;
                long int ack_received;
                int b_fread;
                long int number_of_fragment = sizeFile(filename)/BUFFERSIZE +1;
                int size_of_last_segment;
                
                struct timeval debut,fin,timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 700;
            
                int duplicates_ack;
                
                //construction de packet
                pkt* packet=malloc(sizeof(struct pacquet)*number_of_fragment);
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
                            sendto(socket_data, segment, size_of_last_segment+6, 0, (struct sockaddr *)&cli, cdp);
                        }
                        else{
                            memcpy(&segment[6], packet[i].data, MAXIMUM_SEGMENT_SIZE);
                            sendto(socket_data, segment, MAXIMUM_SEGMENT_SIZE, 0, (struct sockaddr *)&cli, cdp);
                        }
                    
                    }  
                    
                    FD_ZERO(&readfds_trans); 
                    FD_SET(socket_data, &readfds_trans);
                    select(socket_data+1, &readfds_trans, NULL, NULL,&timeout);
                    if(FD_ISSET(socket_data, &readfds_trans)!=0){
                        
                        msgSize = recvfrom(socket_data,rcv_msg_control,MSG_CONTROL_SIZE,0, (struct sockaddr *)&cli, &cdp);
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
                                sprintf(ack, "%06ld" ,ack_received+1);
                                memcpy(&segment[0], ack, ACKSIZE);
                            
                                if(ack_received+1 == number_of_fragment){
                                    memcpy(&segment[6], packet[ack_received+1].data, size_of_last_segment+6);
                                    sendto(socket_data, segment, size_of_last_segment+6, 0, (struct sockaddr *)&cli, cdp);
                                }
                                else{
                                    memcpy(&segment[6], packet[ack_received+1].data, MAXIMUM_SEGMENT_SIZE);
                                    sendto(socket_data, segment, MAXIMUM_SEGMENT_SIZE, 0, (struct sockaddr *)&cli, cdp);
                                }

                            }
                            seg_number = ack_received+1;
                        }
                    
                    }else{//timeout on retransmet le paquet et on diminue la taille de la fenêtre

                        sprintf(ack, "%06ld" ,seg_number);
                        memcpy(&segment[0], ack, ACKSIZE);
                        
                        if(seg_number == number_of_fragment){
                            memcpy(&segment[6], packet[seg_number].data, size_of_last_segment+6);
                            sendto(socket_data, segment, size_of_last_segment+6, 0, (struct sockaddr *)&cli, cdp);
                        }
                        else{
                            memcpy(&segment[6], packet[seg_number].data, MAXIMUM_SEGMENT_SIZE);
                            sendto(socket_data, segment, MAXIMUM_SEGMENT_SIZE, 0, (struct sockaddr *)&cli, cdp);
                        }
                        

                        
                    }
                    
                    
                }
                gettimeofday(&fin, NULL);
                long double elapsed_time = fin.tv_sec - debut.tv_sec + 0.000001* (fin.tv_usec - debut.tv_usec) ;
                printf("\nFichier : %s taille : %d\n",filename, sizeFile(filename));
                double debit = sizeFile(filename)/elapsed_time;
                //printf("Le debit est %.3f ko/s \n", debit/1024);
                printf("\nNombre de fragments : %ld\n", number_of_fragment);
                printf("\nTaille d'un segment : %d\n", BUFFERSIZE);
                printf("\nDebit pour le client %s:%d : %.3f Mo/s \n",inet_ntoa(client.sin_addr),ntohs(client.sin_port),debit/1048576);
                sendto(socket_data,(const char *) end_msg, strlen(end_msg), 0, (const struct sockaddr *)&cli, cdp);
                printf("\nMessage %s envoye \n",end_msg);
                close(socket_data);
                printf("\nFichier correctement envoye  %s:%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
                printf("\n----------------------------------------------------\n");
                free(packet);
                fclose(fp);
                exit(0);
                
            } 
            
        }    
        
    }
    close(socket_control);
    exit(0);
    return(0);

    


    


}



int max (int a, int b){
    if (a> b){
        return a;   
    }
    return b;
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

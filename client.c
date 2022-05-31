#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>

#define bufferSize 500
#define windowSize 5

struct segement{
	int ID;
	int length;
	char data[bufferSize];
	};

int main(int argc, char *argv[]){

	//Declaring Local Variables
	int sock, length, n;
	struct sockaddr_in server, client;
	struct hostent *hp;
	char buffer[bufferSize];
	struct segement sgmt;
	long int totalSegements = 0;
	long int bytes_recv = 0, i = 0, a;
	FILE *file;
	long int ack_num;
	int m, j, x;
	struct segement recvArray[5];

	//Appling a check on Commandline arguments
	if(argc != 3){
		fprintf(stderr, "ERROR! Commandline Arguments not provided!\nProgram Terminated!\n");
		exit(0);
	}

	//Creatng a Socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	//Checking for failure in socket opening
	if(sock < 0){
		fprintf(stderr, "ERROR! Could not open socket\nProgram Terminated!\n");
		exit(0);
	}

	server.sin_family = AF_INET;		//for Internet Address we use AF_INET
	hp = gethostbyname(argv[1]);		//get the host from the provided IP
	
	//Checking for failure in getting host
	if(hp == 0){
		fprintf(stderr, "ERROR! Could not get Host\nProgram Terminated\n");
		exit(0);
	}
	
	bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);	//copying server data to host
	server.sin_port = htons(atoi(argv[2]));								//providing the port number
	length = sizeof(struct sockaddr_in);

	bzero(buffer, sizeof(buffer));

	
	//Sending 'get' Command to Server for the file
	sendto(sock, "get" , strlen("get"), 0, (struct sockaddr *)&server, length);

	//Creating a File for Flushing the data from the Server
	file = fopen("Client File.mp4", "wb");

	//START RECIEVING FILE

	//Recieveing Total Number of Segements from the Server
	recvfrom(sock, &(totalSegements), sizeof(totalSegements), 0, NULL, NULL);
	//Sending the ACK for Number of Segements recieved from the Server
	sendto(sock, &(totalSegements), sizeof(totalSegements), 0, (struct sockaddr *)&server, length);

	if(totalSegements > 0){
		//sendto(sock, &(totalSegements), sizeof(totalSegements), 0, (struct sockaddr *)&server, length);
		printf("Total length: %ld\n", totalSegements);
		
		//Receiving file
		for(i = 0; i<totalSegements; i = i+windowSize){
		
			if(totalSegements - i >= windowSize){
				memset(&recvArray, 0, sizeof(recvArray));					
				//Recieving Window
				for(j=i; j<i+windowSize; j++){
					memset(&sgmt, 0, sizeof(sgmt));	//Clearing segement
					
					//Recieving segement from server and copying it 
					recvfrom(sock, &(sgmt), sizeof(sgmt), 0, NULL, NULL);
					
					//Sening back the ID of Segement as ACK to Server
					sendto(sock,&(sgmt.ID),sizeof(sgmt.ID),0,(struct sockaddr *)&server, length);

					//Storing Recieved Segements in an Array	
					recvArray[j%windowSize] = sgmt;
					
					//Breaking loop if all segements are Recieved
					if(i == (totalSegements - 1)){
						printf("File received\n");
						break;
					}		
				}//end for Recieving Window						
				
				//RELIABILITY CHECK
				a = 0;
				while(a < windowSize){						
					if((a+i) != (long)recvArray[a].ID){													
						while((a+i) != recvArray[a].ID){
							memset(&sgmt, 0, sizeof(sgmt));								
							recvfrom(sock, &(recvArray[(a+i)%windowSize]), sizeof(sgmt), 0, NULL, NULL);
							sendto(sock,&(recvArray[(a+i)%windowSize].ID),sizeof(sgmt.ID),0,(struct sockaddr *)&server, length);
						}
						recvArray[(sgmt.ID)%windowSize] = sgmt;
					}
					a++;
				}
				//END RELIABILITY										
				
				for(int k=0; k<windowSize; k++){
					printf("recieved: %d ", recvArray[k].ID);
					fwrite(recvArray[k].data, 1, recvArray[k].length, file);
				}
				printf("\n");				
			
				printf("end of i:%ld \n\n", i);
			}
						
			else{
				printf("\n**************\nLAST WINDOW\n**************\n");
			
				int lastWindow = totalSegements - i;					
				struct segement recvArray_last[lastWindow];
				memset(&recvArray_last, 0, sizeof(recvArray_last));
				
				for(j=i; j<i+lastWindow; j++){					
					memset(&sgmt, 0, sizeof(sgmt));	//Clearing our Segement
					
					//Recieving Segement from Server and Copying in our Variable
					recvfrom(sock, &(sgmt), sizeof(sgmt), 0, NULL, NULL);						
					
					//Sening back the ID of Segement as ACK to Server
					sendto(sock,&(sgmt.ID),sizeof(sgmt.ID),0,(struct sockaddr *)&server, length);

					//Storing Recieved Segements in an Array
					recvArray_last[j%lastWindow] = sgmt;						

					//Breaking loop if all segements are Recieved
					if(j == (totalSegements - 1)){
						printf("File received\n");
						break;
					}			
				}

				//RELIABILITY CHECK
				a = 0;
				while(a < windowSize){						
					if((a+i) != (long)recvArray_last[a].ID){													
						while((a+i) != recvArray_last[a].ID){
							memset(&sgmt, 0, sizeof(sgmt));								
							recvfrom(sock, &(recvArray_last[(a+i)%windowSize]), sizeof(sgmt), 0, NULL, NULL);
							sendto(sock,&(recvArray_last[(a+i)%windowSize].ID),sizeof(sgmt.ID),0,(struct sockaddr *)&server, length);
						}
						recvArray_last[(sgmt.ID)%windowSize] = sgmt;
					}
					a++;
				}
				//END RELIABILITY/
				
				for(x=0; x<lastWindow; x++){
					printf("recieved: %d,%d ", recvArray_last[x].ID, j);
					fwrite(recvArray_last[x].data, 1, recvArray_last[x].length, file);
				}
				printf("\n");
			
				printf("end of last window i: %ld\n", totalSegements);
			}
		}
	}
	//END RECIEVING FILE

	close(sock);
	return 0;
}

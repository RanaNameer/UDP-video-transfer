#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#define bufferSize 500
#define windowSize 5

struct segement{
	int ID;
	int length;
	char data[bufferSize];
	};

long calFileLen(const char *fileName){
	FILE *file = fopen("Video File.mp4", "rb");
	fseek(file, 0, SEEK_END);
	long len = ftell(file);
	fclose(file);

	return len;
}

int calPacketNumber(long len){
	int totalSegements;
	if((len % bufferSize) != 0){
		totalSegements = (len/bufferSize) + 1;
	}
	else{
		totalSegements = (len/bufferSize);
	}
	
	return totalSegements;
}

// A comparator function used by qsort 
int compare(const void * a, const void * b){ 
    return ( *(int*)a - *(int*)b ); 
} 

bool arrayAreEqual(int arr1[], int arr2[]){
  
    // Sort both arrays 
    qsort(arr1, windowSize, sizeof(int), compare); 
    qsort(arr2, windowSize, sizeof(int), compare); 
  
    //Linearly compare elements 
    for (int i = 0; i < windowSize; i++){
        if (arr1[i] != arr2[i]){
            return false;
		}
	} 
  
    //If all elements were same. 
    return true; 
} 

int main(int argc, char *argv[]){

	//Declaring Local Variables
	int  sock, length, clientlen, n;
	struct sockaddr_in server;
	struct sockaddr_in client;
	char buffer[bufferSize];
	int totalSegements;
	struct segement sgmt;
	long int i = 0;	
	FILE *file;
	int ack_num = 0;
	struct segement sent_sgmt[windowSize];
	int recievedIDs[windowSize];

	//Checking for proper Commandline Arguments
	if(argc < 2){
		fprintf(stderr, "ERROR! Port number not provided\nProgram Terminated!\n");
		exit(0);
	}

	//Creating a Socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	//Putting Check for failure of socket creation
	if(sock < 0){
		fprintf(stderr, "ERROR! Could not open Socket\nProgram Terminated!");
		exit(0);
		}

	length = sizeof(server);
	bzero(&server, length);   					//clearing the server variable for any garbage data
	server.sin_family = AF_INET;    				//for Internet Address we use AF_INET
	server.sin_addr.s_addr = INADDR_ANY; 				//its own address will be the systems address
	server.sin_port = htons(atoi(argv[1]));  			//providing the port number to the server

	//Binding the socket and the port number and checking for failure 	
	if(bind(sock, (struct sockaddr *)&server, length) < 0){
		fprintf(stderr, "ERROR! Could not bind\nProgram Terminated!");
		exit(0);
	}

	clientlen = sizeof(struct sockaddr_in);

	bzero(buffer, sizeof(buffer));	//Clearing Buffer


	//START OF SENDING FILE

	//Recives "get" command from Client 
	n = recvfrom(sock, buffer, bufferSize, 0, (struct sockaddr *)&client, (socklen_t *)&clientlen);
	printf("Client: %s\n", buffer);
	
	//Comparing the Command from Client
	if((strcmp(buffer,"get") != 0)){
		printf("Comparison failed\n");
		exit(0);
	}	
	else{
		printf("Client Asked for the file\n\n");
		const char *fileName = "Video File.mp4";
		file = fopen(fileName, "rb"); //Opening File to Read		
		//if invalid file
		if(file == NULL){
			printf("File does not exist");
		}
		//if valid file
		else{	
			long len = calFileLen(fileName);
			printf("length of file: %ld\n", len);

			//Calculating Number of Packets		
			totalSegements = calPacketNumber(len);

			printf("Total number of Packets: %d\n", totalSegements);
		
			sendto(sock, &(totalSegements),sizeof(totalSegements), 0, (struct sockaddr *)&client, clientlen);
			recvfrom(sock, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&client, (socklen_t *)&clientlen);

			//Keep Sending 'totalSegements' until recieved by Client
			while(ack_num != totalSegements){				
				sendto(sock, &(totalSegements),sizeof(totalSegements), 0, (struct sockaddr *)&client, clientlen);
				recvfrom(sock, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&client, (socklen_t *)&clientlen);
			}    

			printf("Acknowledgement Received for Total Segemenets: %d\n\n", ack_num);

			//Transmitting Data
			for(i=0; i<totalSegements; i = i+windowSize){
					
					//Sending Windows of windowSize
					if(totalSegements - i >= windowSize){
						for(int j=i; j<i+windowSize; j++){
							memset(&(sgmt), 0, sizeof(sgmt));
							sgmt.ID = j;
							sent_sgmt[j%windowSize] = sgmt; //keeping record of sent segments
							sgmt.length = fread(sgmt.data, 1, bufferSize, file);

							//Sending Segement
							sendto(sock, &(sgmt), sizeof(sgmt), 0, (struct sockaddr *)&client, clientlen);

							//Waiting for ACK
							recvfrom(sock, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&client, (socklen_t *)&clientlen);
							
							recievedIDs[j%windowSize] = ack_num;

						}

						//RELIABILITY CHECK
						int k = 0;
						while(k < windowSize){
							if((sent_sgmt[k]).ID != recievedIDs[k]){
								printf("\n***********************\nNACK received for: %d\n*****************\n",sent_sgmt[k].ID);
								
								while(ack_num != (sent_sgmt[k]).ID){
									sendto(sock, &(sent_sgmt[k]),0,sizeof(sgmt),(struct sockaddr *)&client,clientlen);
									recvfrom(sock,&(ack_num),sizeof(ack_num),0,(struct sockaddr *)&client,(socklen_t *)&clientlen);
								}				
							}
							else{
								printf("ACK received for: %d\n",recievedIDs[k]);
							}
							k++;
						}//end RELIABILITY CHECK			
						
						for(int x=0; x<windowSize; x++){
							if(x == windowSize-1){
								printf("recievedIDs: %d\n\n", recievedIDs[x]);
							}
							else{
								printf("recievedIDs: %d ", recievedIDs[x]);								
							}
						}
											
					}			

					//Sending Last Window smaller than windowSize
					else{
						printf("\n**************\nLAST WINDOW\n**************\n");
						
						int lastWindow = totalSegements % windowSize;

						struct segement sent_sgmt_last[lastWindow];
						int recievedIDs_last[lastWindow];

						for(int m=i; m < totalSegements; m++){
							memset(&(sgmt), 0, sizeof(sgmt));
							sgmt.ID = m;
							sent_sgmt_last[m%lastWindow] = sgmt; //keeping record of sent segments
							sgmt.length = fread(sgmt.data, 1, bufferSize, file);

							//Sending Segement
							sendto(sock, &(sgmt), sizeof(sgmt), 0, (struct sockaddr *)&client, clientlen);

							//Waiting for ACK
							recvfrom(sock, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&client, (socklen_t *)&clientlen);
							
							recievedIDs_last[m%lastWindow] = ack_num;
						}

						//RELIABILITY CHECK
						int k = 0;
						while(k < lastWindow){
							if((sent_sgmt_last[k]).ID != recievedIDs_last[k]){
								printf("***********************\nNACK received for: %d\n*****************\n",sent_sgmt_last[k].ID);

								while(ack_num != (sent_sgmt_last[k]).ID){
									sendto(sock, &(sent_sgmt[k]),0,sizeof(sgmt),(struct sockaddr *)&client,clientlen);
									recvfrom(sock,&(ack_num),sizeof(ack_num),0,(struct sockaddr *)&client,(socklen_t *)&clientlen);
								}				
						
							}
							else{
								printf("ACK received for: %d\n",recievedIDs_last[k]);
							}
							k++;
						}

						for(int x=0; x<lastWindow; x++){
							printf("recievedIDs: %d ", recievedIDs_last[x]);
						}
						printf("\n");
							
					}		

				if(sgmt.length < bufferSize){
					if(feof(file)){
						printf("End of file!\nFile completely sent\n");
					}
					if(ferror(file)){
						printf("Encountered an error!\nProgram Terminated");
					}
					break;
				}					
			}
		}
	}
	//END OF SENDING FILE

	close(sock);
	return 0;
}

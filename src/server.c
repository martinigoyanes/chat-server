#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"
#include "optparser.h"
#include <fcntl.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "assignment3.h"
#include <pthread.h>


pthread_mutex_t clients_mutex, rooms_mutex;
void *handleClient(void *clntSock);
uint8_t size_of(char name [255]);
int lenght_of(char *p);
int main(int argc, char *argv[])
{
	struct server_arguments args = server_parseopt(argc, argv);
	
	in_port_t servPort = args.port; // First arg:  local port

  // Create socket for incoming connections
  int servSock; 					// Socket descriptor for server
  int clntSock;                    /* Socket descriptor for client */
  struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */              
  pthread_t tid ;
  

  if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithSystemMessage("socket() failed");

  // Construct local address structure
  struct sockaddr_in servAddr;                  // Local address
  memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure
  servAddr.sin_family = AF_INET;                // IPv4 address family
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
  servAddr.sin_port = htons(servPort);          // Local port

  // Bind to the local address
  if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
    DieWithSystemMessage("bind() failed");

  // Mark the socket so it will listen for incoming connections
  if (listen(servSock,20) < 0)
    DieWithSystemMessage("listen() failed");
    
  //setup client list
  create_clntls();
  create_rooms();

  for (;;)
  { 	// Run forever
		struct sockaddr_in clntAddr; // Client address
		// Set length of client address structure (in-out parameter)
		socklen_t clntAddrLen = sizeof(clntAddr);
        
		// Wait for a client to connect
		clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
		if (clntSock < 0)
		  DieWithSystemMessage("accept() failed");

		// clntSock is connected to a client!

		char clntName[INET_ADDRSTRLEN]; // String to contain client address
		if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
			sizeof(clntName)) == NULL)
		  puts("Unable to get client address");
		
		/* Create separate memory for client argument */
        if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) 
               == NULL)
            DieWithSystemMessage("malloc() failed");
        threadArgs -> clntSock = clntSock;
		
		
		pthread_create(&tid, NULL, &handleClient, (void*)threadArgs);
		
		 
   }
   deallocate_mem();
   return 0;
}
void *handleClient(void *threadArgs)
{	
	int socket= ((struct ThreadArgs *) threadArgs) -> clntSock;
    free(threadArgs);              /* Deallocate memory for argument */
    
    pthread_detach(pthread_self());    /* Guarantees that thread resources are deallocated upon return */
	
	int numbytes;
	//int disconnect =0;
	struct init firstconnect;
	struct request req;
	struct nickname nick;
	struct chatroom_req room_req;
	struct message_req msg_req;
	
	
	struct clnt *client;
	
	char *resp;// variable to hold all the different string responses we give ouy
	char  trash[41];//to throw away what is left from a 1st connection
	uint8_t ack [8];//variable to answer with hexadecimal
    
	while(1) //wait for client requests forever and ever
	{
		/*receive greetings from client*/
		int remaining = 7;
		char req_msg[7];
		
		do{
			if ((numbytes = recv(socket,&req_msg, 7, 0)) < 0)
				DieWithSystemMessage("recvinit() failed");
			if(numbytes == 0)
			{	
				pthread_mutex_lock(&clients_mutex);
					
				char empty_name[255];// no room 
				memset(empty_name,0,255);
				if(strcmp(client->room,empty_name)!=0) //client inside room so get him out 
				{
					pthread_mutex_lock(&rooms_mutex);
					struct chatroom *room;
					room = find_room(client->room);
						
					room->n_clients--; //reduce room size
					
					memcpy(&client->room,&empty_name[0],255);//unlink client from the room
						
					if(room->n_clients == 0)
					{
						del_room(room->name);
						room = NULL;
					}
							
					pthread_mutex_unlock(&rooms_mutex);
				}
				
				//kick client outside our database
				del_client(client->name);
				client=NULL;
					
				print_rooms();
				print_clnts();
					
				pthread_mutex_unlock(&clients_mutex);
				break;
			}
			remaining = remaining - numbytes;
			
		}while(remaining > 0);
		
		//disconnect 
		
		
		
		memcpy(&req.data_len,&req_msg[0],4);
		memcpy(&req.type,&req_msg[6],1);
			
		req.data_len = be32toh(req.data_len);
		//i dont need to do ntoh to req.type since its only 1 byte
			
		//clear up req_msg for next iteration
		bzero(&req_msg,7);
		
		switch(req.type)
		{	
				
			case 255 : //just connected -> send client his name 
			
			pthread_mutex_lock(&clients_mutex);
			client = add_clnt(socket); //updating client info
			uint8_t name_len = size_of(client->name);
			
			firstconnect.type = htobe32(7);
			firstconnect.version = htobe32(68681216);
			
			memcpy(&firstconnect.name,client->name,name_len);
				
			remaining = 41;
			do{// now we empty from the 41 bytes left of  the read buffer so in our next read we dont override data
				if ((numbytes = recv(socket,&trash, 41, 0)) < 0)
				DieWithSystemMessage("recvinit() failed");
				
				remaining = remaining - numbytes;
			
			}while(remaining > 0);
				
			memset(trash,0,41);// throw away what was left
			remaining = 14;
								
			do{
				numbytes = send(socket,&firstconnect, 14, 0);
				if (numbytes < 0)
					DieWithSystemMessage("sendinit() failed");
				remaining = remaining - numbytes;
			}while(remaining > 0);
			
			
			print_clnts();
			pthread_mutex_unlock(&clients_mutex);
			break;
				
			case 14: //nickname change
				remaining = 1;
				do{
						if ((numbytes = recv(socket,&nick.data_len, 1, 0)) < 0)
						DieWithSystemMessage("recvnicklen() failed");
						
						remaining = remaining - numbytes;
				
				}while(remaining > 0);
					
			  remaining = nick.data_len; //no need to to ntoh since its 1 byte
					
			  do{
					if ((numbytes = recv(socket,&nick.name, nick.data_len, 0)) < 0)
						DieWithSystemMessage("recvnickname() failed");
						
					remaining = remaining - numbytes;
				
				}while(remaining > 0);
				
				pthread_mutex_lock(&clients_mutex);
				if(find_client(nick.name) == 0x0)// this name is not hold by anyone then we allow the change
				{
					memset(client->name,0,255);
					strcpy(client->name,nick.name);//we update our database
					
					ack[0]=0x00;
					ack[1]=0x00;
					ack[2]=0x00;
					ack[3]=0x01;
					ack[4]=0x04;
					ack[5]=0x17;
					ack[6]=0xfe;
					ack[7]=0x00;
							
					remaining = 8;			
					do{
						numbytes = send(socket,&ack, 8, 0);
						if (numbytes < 0)
							DieWithSystemMessage("sendnick() failed");
						remaining = remaining - numbytes;
					}while(remaining > 0);
							
					memset(ack,0,8);
					memset(&nick,0,257);
				}
				
				
				else
				{//nickname is already held by someone so we dont update
				
					ack[0]=0x00;
					ack[1]=0x00;
					ack[2]=0x00;
					ack[3]=0x2b;
					ack[4]=0x04;
					ack[5]=0x17;
					ack[6]=0xfe;
					ack[7]=0x01;
							
					resp = "This nick has been nicked by someone else.";
							
					uint8_t msg[50];
							
					memcpy(&msg[0],&ack[0],8);//hex values in the message
					memcpy(&msg[8],resp,42);
							
					remaining = 50;			
					do{
						numbytes = send(socket,&msg, 50, 0);
						if (numbytes < 0)
							DieWithSystemMessage("sendnick() failed");
						remaining = remaining - numbytes;
					}while(remaining > 0);
							
					memset(ack,0,8);
					
						
				}
				
				print_clnts();
				pthread_mutex_unlock(&clients_mutex);
				break;
				case 10:
					remaining = 1;
					
					//clear room request up
					bzero(&room_req,512);
					do{
						if ((numbytes = recv(socket,&room_req.name_len, 1, 0)) < 0)
						DieWithSystemMessage("recvroomlen() failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					remaining = room_req.name_len;
					
					
					do{
						if ((numbytes = recv(socket,&room_req.name, room_req.name_len, 0)) < 0)
						DieWithSystemMessage("recvroomname failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					//password in msg? --> store pasword
					
					remaining = 1;
					
					
					do{
						if ((numbytes = recv(socket,&room_req.pass_len, 1, 0)) < 0)
						DieWithSystemMessage("recvroomname failed");
						
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					if(room_req.pass_len > 0)
					{
						remaining = room_req.pass_len;
						do{
							if ((numbytes = recv(socket,&room_req.pass, room_req.pass_len, 0)) < 0)
								DieWithSystemMessage("recvroomname failed");
						
						
							remaining = remaining - numbytes;
				
						}while(remaining > 0);	
					}
					pthread_mutex_lock(&rooms_mutex);
					pthread_mutex_lock(&clients_mutex);
					
					
					if(strcmp(room_req.name,client->room)==0)// you are trying to reenter a room you are in
					{
								ack[0]=0x00;
								ack[1]=0x00;
								ack[2]=0x00;
								ack[3]=0x4f;
								ack[4]=0x04;
								ack[5]=0x17;
								ack[6]=0xfe;
								ack[7]=0x01;
							
								resp = "You attempted to bend space and time to reenter where you aready are.You fail.";
							
								uint8_t msg[86];
								
								remaining =86;
								
								memcpy(&msg[0],&ack[0],8);//hex values in the message
								memcpy(&msg[8],resp,78);
								
								do{
									numbytes = send(socket,&msg,86, 0);
									if (numbytes < 0)
										DieWithSystemMessage("sendnick() failed");
									remaining = remaining - numbytes;
								}while(remaining > 0);	
								
					}
					
					else
					{	
						struct chatroom *oldroom;
						struct chatroom *room ;
						oldroom = find_room(client->room);
						room = find_room(room_req.name);
						
						
						if(room != 0x0) //room already exists?
						{
							if(strcmp(room->pass,room_req.pass)==0) //if pass? correct pass?
							{		
								
									//leave previous room
									if(oldroom != NULL)
									{
										oldroom->n_clients --;
										if(oldroom->n_clients == 0)
										{
											del_room(client->room);
											room=NULL;
										}
									}
									//join new room
									room->n_clients++;										
									strcpy(client->room,room->name);                     
									remaining = 8;
									
									ack[0]=0x00;
									ack[1]=0x00;
									ack[2]=0x00;
									ack[3]=0x01;
									ack[4]=0x04;
									ack[5]=0x17;
									ack[6]=0xfe;
									ack[7]=0x00;
											
									do{
										numbytes = send(socket,&ack,8, 0);
										if (numbytes < 0)
											DieWithSystemMessage("sendnick() failed");
										remaining = remaining - numbytes;
									}while(remaining > 0);
									
							}
							else  // wrong pass
							{
									ack[0]=0x00;
									ack[1]=0x00;
									ack[2]=0x00;
									ack[3]=0x26;
									ack[4]=0x04;
									ack[5]=0x17;
									ack[6]=0xfe;
									ack[7]=0x01;
								
									resp = "Invalid password.You shall not pass.";
								
									uint8_t msg[45];
									
									remaining =45;
									
									memcpy(&msg[0],&ack[0],8);//hex values in the message
									memcpy(&msg[8],resp,37);
									
									do{
										numbytes = send(socket,&msg,45, 0);
										if (numbytes < 0)
											DieWithSystemMessage("sendnick() failed");
										remaining = remaining - numbytes;
									}while(remaining > 0);	
							}		
						}
						
						
						else // room dont exist already then create it with pass given , if pass null , room without pass
						{
							//leave previous room
							if(oldroom != NULL)
							{
								oldroom->n_clients --;
								if(oldroom->n_clients == 0)
								{
									del_room(client->room);
									room=NULL;
								}
							}
							
							//join new room
							add_room(room_req.name,room_req.pass);
							
							strcpy(client->room,room_req.name);
							remaining = 8;
									
									ack[0]=0x00;
									ack[1]=0x00;
									ack[2]=0x00;
									ack[3]=0x01;
									ack[4]=0x04;
									ack[5]=0x17;
									ack[6]=0xfe;
									ack[7]=0x00;
											
									do{
										numbytes = send(socket,&ack,8, 0);
										if (numbytes < 0)
											DieWithSystemMessage("sendnick() failed");
										remaining = remaining - numbytes;
									}while(remaining > 0);
						}
					}
					print_rooms();
					print_clnts();
					
					pthread_mutex_unlock(&rooms_mutex);	
					pthread_mutex_unlock(&clients_mutex);
					
					memset(&room_req,0,526);
					break;
				case 15://msg user
				
					remaining = 1;
					
					do{
						if ((numbytes = recv(socket,&msg_req.name_len, 1, 0)) < 0)
						DieWithSystemMessage("recvroomlen() failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					remaining = msg_req.name_len;
					
					do{
						if ((numbytes = recv(socket,&msg_req.name, msg_req.name_len+1, 0)) < 0)
						DieWithSystemMessage("recvroomlen() failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					struct clnt *reciever;
					
					pthread_mutex_lock(&clients_mutex);
					reciever =  find_client(msg_req.name);//who we are sending the message to
					
					if(reciever == NULL) // sender is trying to send to someone who doesnt exist
					{
						ack[0]=0x00;
						ack[1]=0x00;
						ack[2]=0x00;
						ack[3]=0x11;
						ack[4]=0x04;
						ack[5]=0x17;
						ack[6]=0xfe;
						ack[7]=0x01;
								
						resp = "Nick not present";
								
						uint8_t msg[24];
									
						remaining =24;
									
						memcpy(&msg[0],&ack[0],8);//hex values in the message
						memcpy(&msg[8],resp,16);
									
						do{
							numbytes = send(socket,&msg,24, 0);
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);				
					}	
					else //receiver actually exists
					{
						//tell the sender his recevier exists
						ack[0]=0x00;
						ack[1]=0x00;
						ack[2]=0x00;
						ack[3]=0x01;
						ack[4]=0x04;
						ack[5]=0x17;
						ack[6]=0xfe;
						ack[7]=0x00;
								
						remaining =8;
												
						do{
							numbytes = send(socket,&ack,8, 0);
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);
						
						//send message to receiver
						//finish up reading msg from sender
						remaining = 1;
					
						do{
							if ((numbytes = recv(socket,&msg_req.msg_len, 1, 0)) < 0)
							DieWithSystemMessage("recvroomlen() failed");
						
							remaining = remaining - numbytes;
				
						}while(remaining > 0);
						
						remaining = msg_req.msg_len;
					
						do{
							if ((numbytes = recv(socket,&msg_req.msg, msg_req.msg_len, 0)) < 0)
							DieWithSystemMessage("recvroomlen() failed");
						
							remaining = remaining - numbytes;
				
						}while(remaining > 0);
						
						// now i sett up the msg that will be forwarded to the reciever
						
						uint16_t total_data[1]; //size of my payload w/o the "ack"(3)  and first 3 empty bytes
						uint16_t total_data_net[1];
						uint8_t  sender_len[1] ;
						uint8_t  msg_len[1];
						uint8_t  header[5];
						
						header[0]=0x00;
						header[1]=0x00;
						
						header[2]=0x04;
						header[3]=0x17;
						header[4]=0x0f;
						
						sender_len[0]= size_of(client->name)-1;
						total_data[0] = sender_len[0] + 3 + msg_req.msg_len;
						total_data_net[0] = htobe16(total_data[0]);
						msg_len[0] = msg_req.msg_len;
						
						
						char message[total_data[0] + 7];
						
						bzero(&message[0],total_data[0]+7);//clean up the buffer
						
						memcpy(&message[0],&header[0],2);// first 2 0 bytes
						memcpy(&message[2],&total_data_net[0],2);//#bytes payload
						memcpy(&message[4],&header[2],3);//header 3 bytes
						memcpy(&message[7],&sender_len[0],1);//#bytes of sender's name
						memcpy(&message[8],&client->name[0],sender_len[0]+1);//sender's name with 0 byte
						memcpy(&message[8+sender_len[0] + 1],&msg_len[0],1);//#bytes of msg
						memcpy(&message[8+sender_len[0] + 2],msg_req.msg,msg_req.msg_len);//msg
						
						do{
							numbytes = send(reciever->clntsocket,&message,total_data[0] + 7, 0); //send to reciever socket
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);
						
					}	
					
				pthread_mutex_unlock(&clients_mutex);
				break;
				case 16: //message room
				
					pthread_mutex_lock(&clients_mutex);
					
					char void_name[255];// no room 
					memset(void_name,0,255);
					if(strcmp(client->room,void_name)==0)//client not in room so we send him the not-in-room message
					{
						ack[0]=0x00;
						ack[1]=0x00;
						ack[2]=0x00;
						ack[3]=0x2a;
						ack[4]=0x04;
						ack[5]=0x17;
						ack[6]=0xfe;
						ack[7]=0x01;
								
						resp = "You shout into the void and hear nothing.";
								
						uint8_t msg[49];
									
						remaining =24;
									
						memcpy(&msg[0],&ack[0],8);//hex values in the message
						memcpy(&msg[8],resp,41);
									
						do{
							numbytes = send(socket,&msg,49, 0);
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);	
					}
					
					else
					{ //client is actually inside a room so he can communicate
					remaining = 1;
					
					//we tell the client the in-room message
					ack[0]=0x00;
					ack[1]=0x00;
					ack[2]=0x00;
					ack[3]=0x01;
					ack[4]=0x04;
					ack[5]=0x17;
					ack[6]=0xfe;
					ack[7]=0x00;
					
					remaining =8;
									
				    do{
						numbytes = send(socket,&ack,8, 0);
						if (numbytes < 0)
							DieWithSystemMessage("sendnick() failed");
						remaining = remaining - numbytes;
					}while(remaining > 0);	
					
					
					//we read the request
					do{
						if ((numbytes = recv(socket,&msg_req.room_len, 1, 0)) < 0)
						DieWithSystemMessage("recvroomlen() failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					remaining = msg_req.room_len;
					
					do{
						if ((numbytes = recv(socket,&msg_req.room, msg_req.room_len+1, 0)) < 0)
						DieWithSystemMessage("recvroomlen() failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					remaining = 1;
					
					do{
						if ((numbytes = recv(socket,&msg_req.msg_len,1, 0)) < 0)
						DieWithSystemMessage("recvroomlen() failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					remaining = msg_req.msg_len;
					
					do{
						if ((numbytes = recv(socket,&msg_req.msg,msg_req.msg_len, 0)) < 0)
						DieWithSystemMessage("recvroomlen() failed");
						
						remaining = remaining - numbytes;
				
					}while(remaining > 0);
					
					// we finished reading request so now spread the message
					
					struct clnt *reciever;
					struct clnt *clnts_gl;
					clnts_gl = get_globalclnts();
					
					// now i sett up the msg that will be forwarded to the reciever
						
						uint16_t total_data[1]; //size of my payload w/o the "ack"(3)  and first 3 empty bytes
						uint16_t total_data_net[1];
						uint8_t  sender_len[1] ;
						uint8_t  room_len[1] ;
						uint8_t  msg_len[1];
						uint8_t  header[5];
						
						header[0]=0x00;
						header[1]=0x00;
						
						header[2]=0x04;
						header[3]=0x17;
						header[4]=0x10;
						
						sender_len[0]= size_of(client->name)-1;
						total_data[0] = sender_len[0] + 4 + msg_req.msg_len + msg_req.room_len;
						total_data_net[0] = htobe16(total_data[0]);
						msg_len[0] = msg_req.msg_len;
						room_len [0]=msg_req.room_len; 
						
						
						char message[total_data[0] + 7];
						
						bzero(&message[0],total_data[0]+7);//clean up the buffer
						
						memcpy(&message[0],&header[0],2);// first 2 0 bytes
						memcpy(&message[2],&total_data_net[0],2);//#bytes payload
						memcpy(&message[4],&header[2],3);//header 3 bytes
						memcpy(&message[7],&room_len[0],1);//length of room name
						memcpy(&message[8],&msg_req.room,room_len[0]);//room name
						memcpy(&message[8 + room_len[0]],&sender_len[0],1);//#bytes of sender's name
						memcpy(&message[8 + room_len[0] + 1],&client->name[0],sender_len[0]+1);//sender's name with 0 byte
						memcpy(&message[8 + room_len[0] + 1 + sender_len[0] + 1],&msg_len[0],1);//#bytes of msg
						memcpy(&message[8 + room_len[0] + 1 + sender_len[0] + 1 + 1],msg_req.msg,msg_req.msg_len);//msg
					
					// we look for the clients in the room that are not ourselver
					for (reciever = clnts_gl->next; reciever != clnts_gl; reciever= reciever->next){
						if (strcmp(reciever->room, msg_req.room) == 0 && strcmp(client->name, reciever->name) != 0){						
						do{
							numbytes = send(reciever->clntsocket,&message,total_data[0] + 7, 0); //send to reciever socket
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);
						
						}
					}
					
					
					}
					pthread_mutex_unlock(&clients_mutex);
					break;
				case 11: //leave
					pthread_mutex_lock(&clients_mutex);
					
					char empty_name[255];// no room 
					memset(empty_name,0,255);
					if(strcmp(client->room,empty_name)==0)
					{
						del_client(client->name);
						client=NULL;
					}
					
					else// client inside a room so we get him out
					{	
						pthread_mutex_lock(&rooms_mutex);
						struct chatroom *room;
						room = find_room(client->room);
						
						room->n_clients--; //reduce room size
						
						memcpy(&client->room,&empty_name[0],255);//unlink client from the room
						
						if(room->n_clients == 0)
						{
							del_room(room->name);
							room=NULL;
						}
							
						pthread_mutex_unlock(&rooms_mutex);
					}
					
					print_rooms();
					print_clnts();
					
					pthread_mutex_unlock(&clients_mutex);
					
					remaining = 8;
									
					ack[0]=0x00;
					ack[1]=0x00;
					ack[2]=0x00;
					ack[3]=0x01;
					ack[4]=0x04;
					ack[5]=0x17;
					ack[6]=0xfe;
					ack[7]=0x00;
											
					do{
						numbytes = send(socket,&ack,8, 0);
						if (numbytes < 0)
							DieWithSystemMessage("sendnick() failed");
						remaining = remaining - numbytes;
					}while(remaining > 0);
					
				break;
				case 13://list users
					pthread_mutex_lock(&clients_mutex);
					
					char no_name[255];// no room 
					memset(no_name,0,255);
					if(strcmp(client->room,no_name)==0)//client is outside a chat room
					{
						struct clnt *aux;
						struct clnt *clnts_gl;
						clnts_gl = get_globalclnts();
						
						uint32_t total_data[1];
						uint32_t total_data_net[1];
						uint8_t name_len[1];
						int offset = 0;
						char payload[30000];
						
						total_data[0] = 0;
						//traverse clients and store their names and update the lenght of datagram
						for (aux=clnts_gl->next; aux != clnts_gl; aux=aux->next){
								name_len[0] = size_of(aux->name)-1;
								total_data[0] =total_data[0] + name_len[0] + 1;//plus 1 for the byte that holds the name_len
								
								memcpy(&payload[offset],&name_len[0],1);
								memcpy(&payload[offset + 1],&aux->name[0],name_len[0]);
								
								offset = offset +1 + name_len[0];
						}
						
						uint8_t ack[4];
						ack[0]=0x04;
						ack[1]=0x17;
						ack[2]=0xfe;
						ack[3]=0x00;
						
						total_data[0] =total_data[0] +1;
						char message[total_data[0] + 7]; //message sent to client
						total_data_net[0] = htobe32(total_data[0]);
						
						memcpy(&message[0],&total_data_net[0],4);
						memcpy(&message[4],&ack[0],4);
						memcpy(&message[8],&payload[0],total_data[0]);
						
						//send message
						remaining = total_data[0] + 7;
						do{
							numbytes = send(socket,&message,total_data[0] + 7, 0);
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);
						
						bzero(payload,30000);
						bzero(message,total_data[0]+7);
					}
					else
					{//client is inside a chatroom
						
						struct clnt *aux;
						struct clnt *clnts_gl;
						clnts_gl = get_globalclnts();
						
						uint32_t total_data[1];
						uint32_t total_data_net[1];
						uint8_t name_len[1];
						int offset = 0;
						char payload[30000];
						
						total_data[0] = 0;
						//traverse clients and ONLY store their names and update the lenght of datagram
												//IF they are in the room together
						for (aux=clnts_gl->next; aux != clnts_gl; aux=aux->next){
							if(strcmp(client->room,aux->room)==0){
								name_len[0] = size_of(aux->name)-1;
								total_data[0] =total_data[0] + name_len[0] + 1;//plus 1 for the byte that holds the name_len
								
								memcpy(&payload[offset],&name_len[0],1);
								memcpy(&payload[offset + 1],&aux->name[0],name_len[0]);
								
								offset = offset +1 + name_len[0];
							}
						}
						
						uint8_t ack[4];
						ack[0]=0x04;
						ack[1]=0x17;
						ack[2]=0xfe;
						ack[3]=0x00;
						total_data[0] =total_data[0] +1;
						total_data_net[0] = htobe32(total_data[0]);
						char message[total_data[0] + 7]; //message sent to client
						
						memcpy(&message[0],&total_data_net[0],4);
						memcpy(&message[4],&ack[0],4);
						memcpy(&message[8],&payload[0],total_data[0]);
						
						//send message
						remaining = total_data[0] + 7;
						do{
							numbytes = send(socket,&message,total_data[0] + 7, 0);
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);
						bzero(payload,30000);
						bzero(message,total_data[0]+7);
						
					}
					pthread_mutex_unlock(&clients_mutex);
				break;
				case 12://list rooms
					if(1){
						pthread_mutex_lock(&rooms_mutex);
						struct chatroom *aux;
						struct chatroom *rooms_gl;
						rooms_gl = get_globalrooms();
						
						uint32_t total_data[1];
						uint32_t total_data_net[1];
						uint8_t name_len[1];
						int offset = 0;
						char payload[30000];
						
						total_data[0] = 0;
						//traverse rooms and store their names and update the lenght of datagram
						for (aux=rooms_gl->next; aux != rooms_gl; aux=aux->next){
								name_len[0] = size_of(aux->name)-1;
								total_data[0] =total_data[0] + name_len[0] + 1;//plus 1 for the byte that holds the room_len
								
								memcpy(&payload[offset],&name_len[0],1);
								memcpy(&payload[offset + 1],&aux->name[0],name_len[0]);
								
								offset = offset +1 + name_len[0];
						}
						
						uint8_t ack[4];
						ack[0]=0x04;
						ack[1]=0x17;
						ack[2]=0xfe;
						ack[3]=0x00;
						
						total_data[0] = total_data[0] + 1;
						
						char message[total_data[0] + 7]; //message sent to client
						total_data_net[0] = htobe32(total_data[0]);
						
						memcpy(&message[0],&total_data_net[0],4);
						memcpy(&message[4],&ack[0],4);
						memcpy(&message[8],&payload[0],total_data[0]);
						
						//send message
						remaining = total_data[0] + 7;
						do{
							numbytes = send(socket,&message,total_data[0] + 7, 0);
							if (numbytes < 0)
								DieWithSystemMessage("sendnick() failed");
							remaining = remaining - numbytes;
						}while(remaining > 0);
						
						bzero(payload,30000);
						bzero(message,total_data[0]+7);
						pthread_mutex_unlock(&rooms_mutex);
					}
				break;
				default:
				break; 
			} 
			
			if(client == NULL)//client disconnected then kill this thread and close the socket
			{
				close(socket);
				
				pthread_exit(0);
			}
			//clear the request up
			bzero(&req,5);
		}
	
}
uint8_t size_of(char name [255])
{	
	uint8_t i =0;
	for(i = 0 ; i<255; i++){
		if(name[i] == '\0')
			break;
	}
	return i +1;
}

/* C standard library */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <uuid/uuid.h>

/* My defined libraries */
#include "chatlib.h"

#define TRUE   1
#define FALSE  0
#define PORT 2001
#define MAX_CLIENT 30
#define CLIENTNAME_SIZE 50
#define BUFFER_SIZE 500
#define MESSAGE_SIZE 500
#define INI_FILE_NAME "Server.ini"


int main()
{
	int master_socket , addrlen , new_socket , activity;
	int max_sd;
	struct sockaddr_in address;
	Client_Socket client_socket[MAX_CLIENT];

	char buffer[BUFFER_SIZE];
	char message[MESSAGE_SIZE];

	FILE* init_fp;

	//open init file
	init_fp = fopen( INI_FILE_NAME, "a+" );

	//set of socket descriptors
	fd_set readfds;

	//Initialize all client_socket[] to 0/NULL
	for( int i = 0; i < MAX_CLIENT; i++ ){
		client_socket[i].sd = 0;
		memset( client_socket[i].username, 0, sizeof( client_socket[i].username ) );
	}

	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0 ){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons( PORT );

	//bind the socket
	if( bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0 ){
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);

	//try to specify maximum of 5 pending connections for the master socket
	if( listen(master_socket, 5) < 0 ){
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//waiting for the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	while(TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add fd of stdin to set
		FD_SET( STDIN_FILENO, &readfds );
		max_sd = STDIN_FILENO;

		//add master socket to set
		FD_SET( master_socket, &readfds );
		max_sd = (master_socket>STDIN_FILENO)?master_socket:STDIN_FILENO;

		//add child sockets to set
		for( int i = 0 ; i < MAX_CLIENT ; i++ ){
			//if valid socket descriptor then add to read list
			if( client_socket[i].sd > 0 ){
				FD_SET( client_socket[i].sd , &readfds);
			}

			//highest file descriptor number, need it for the select function
			if( client_socket[i].sd > max_sd ){
				max_sd = client_socket[i].sd;
			}
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL );

		if( (activity < 0) && (errno!=EINTR) ){
			printf("select error");
		}

		//Inputs from STDIN
		if( FD_ISSET(STDIN_FILENO, &readfds) ){

			memset( buffer, 0, BUFFER_SIZE );
			read( STDIN_FILENO, buffer, BUFFER_SIZE );

			if( strcmp(buffer, "exit\n") != 0 ){
				printf("Undefined command. Try again...\n");

			}
			//received exit message from server, close all opening sockets
			else{

				//close opened FilePointer
				fclose(init_fp);

				//close all client socket
				printf("Server closed!\n");
				for( int i = 0; i < MAX_CLIENT; i++ ){
					close(client_socket[i].sd);
				}

				//close listening socket
				close(master_socket);

				exit(EXIT_SUCCESS);
			}
		}

		//Something happening on the master socket, then it's an incoming connection
		if( FD_ISSET(master_socket, &readfds) ){
			if( (new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0 ){
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

			//send new connection greeting message
			memset( message, 0, MESSAGE_SIZE );
			strncpy( message, "Your connect request got accepted...\nServer is waiting for your login info OR register request...", MESSAGE_SIZE );
			write( new_socket, message, strlen(message) );

			//add new socket to array of sockets
			for( int i = 0; i < MAX_CLIENT; i++){
				//if position is empty
				if( client_socket[i].sd == 0 ){
					client_socket[i].sd = new_socket;
					printf("Adding to list of sockets as %d\n" , i);
					break;
				}
			}
		}

		//Loop among online Clients
		for( int current_client = 0; current_client < MAX_CLIENT; current_client++ ){

			//Something happening on online Clients excluding STDIN
			if( FD_ISSET( client_socket[current_client].sd , &readfds) && (client_socket[current_client].sd) ){
				memset(buffer, 0, BUFFER_SIZE);
				//Client socket was closed
				if( read( client_socket[current_client].sd , buffer, BUFFER_SIZE) == 0 ){
					//this socket gets disconnected , get its details and print
					getpeername(client_socket[current_client].sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

					//Copy leaving client name for further goodbye message
					strcpy(buffer, client_socket[current_client].username);

					//Close the socket and reset its values
					close( client_socket[current_client].sd );
					client_socket[current_client].sd = 0;
					memset(client_socket[current_client].username, 0, CLIENTNAME_SIZE);
					memset(client_socket[current_client].uuid_str, 0, UUIDSTRING_SIZE);

					//
					sprintf( message, "Goodbye %s", buffer );
					server_broadcast( client_socket, MAX_CLIENT, message, MESSAGE_SIZE );
					server_broadcastAliveClientList( client_socket, MAX_CLIENT );
				}

				//Client socket is still alive, so it's a message from client
				else{
					//Check if this client is on board
					if( strlen(client_socket[current_client].username) != 0 ){

						//Private message from client
						if( strstr( buffer, "@private@" ) ){

							char pointed_username[50];
							int pointed_sockfd;

							memset(message, 0, MESSAGE_SIZE);
							server_getPrivateMessage( buffer, pointed_username, message );

							//client not available
							if( !server_isClientLogin( pointed_username, &pointed_sockfd, client_socket, MAX_CLIENT ) ){
								memset(message, 0, MESSAGE_SIZE);
								sprintf( message, "Pointed User is not online OR invalid." );
								write(client_socket[current_client].sd, message, strlen(message));
							}
							//client is online
							else{
								//private message is empty
								if( strlen( message ) == 0 ){
									memset(message, 0, MESSAGE_SIZE);
									sprintf( message, "Private message is empty. Try again." );
									write(client_socket[current_client].sd, message, strlen(message));
								}
								//send private message
								else{
									server_sendPrivateMessage( pointed_sockfd , client_socket[current_client].username, message );
									server_sendPrivateAck( client_socket[current_client].sd, pointed_username, message );
								}
							}
						}

						//Generic message
						else{
							//Broadcast targeted client's message to others
							for( int i = 0; i < MAX_CLIENT; i++ ){
								//Avoid sending to STDIN & targeted client & client who is registering...
								if( (client_socket[i].sd != STDIN_FILENO) && (client_socket[i].sd != client_socket[current_client].sd) && (strlen(client_socket[i].username) != 0) ){

									//client name
									char name[50];
									sprintf(name, "%s : ", client_socket[current_client].username);

									write(client_socket[i].sd , name , strlen(name));
									write(client_socket[i].sd , buffer , strlen(buffer));
								}
							}
						}
					}

					//This client socket is not boarding...
					else{
						//check if client authentication info sending
						if( strstr( buffer, "@authentication\n" ) ){
							char username[50], uuid_str[36];
							memset(username, 0, 50);
							memset(uuid_str, 0, 36);

							//authentication info confirmed
							if( server_getncompareAuthenticationInfo( buffer, BUFFER_SIZE, username, uuid_str, init_fp) ){
								memset(message, 0, MESSAGE_SIZE);
								sprintf( message, "@authentication\nvalid");
								write( client_socket[current_client].sd, message, MESSAGE_SIZE);
								puts("authentication valid");

								//add client info to client managing struct
								strcpy( client_socket[current_client].username, username );
								strcpy( client_socket[current_client].uuid_str, uuid_str );

								//broadcast greeting new client to everyone
								memset(message, 0, MESSAGE_SIZE);
								sprintf(message, "Welcome %s to our group.", client_socket[current_client].username);
								server_broadcast( client_socket, MAX_CLIENT, message, MESSAGE_SIZE );
								server_broadcastAliveClientList( client_socket, MAX_CLIENT );
							}
							//authentication info invalid
							else{
								memset(message, 0, MESSAGE_SIZE);
								sprintf( message, "@authentication\ninvalid");
								write( client_socket[current_client].sd, message, MESSAGE_SIZE);
								puts("authentication invalid");
							}
						}

						//client register request received
						else if( strstr( buffer, "@register\nusername=" ) ){
							uuid_t uuid;
							char username[50], uuid_str[36];
							char* token;

							memset(username, 0, 50);
							memset(uuid_str, 0, 36);

							token = strtok( buffer, "\n" );
							token = strtok( NULL, "=");
							token = strtok( NULL, "=");
							strcpy( username, token );

							//proposed user name unique
							if( !server_checkDuplicatedUsername( username, init_fp ) ){
								//generate new UUID
								uuid_generate(uuid);
								uuid_unparse(uuid, uuid_str);

								//loop until unique UUID generated
								while( server_checkDuplicatedUUID( uuid_str, init_fp ) ){

									memset( message, 0, MESSAGE_SIZE );
									sprintf( message, "generated UUID : %s is duplicated inside %s file. Getting new UUID again...", uuid_str, INI_FILE_NAME );
									puts( message );

									//generate new UUID
									uuid_generate(uuid);
									uuid_unparse(uuid, uuid_str);
								}

								//register info is valid from server side; send back valid username & new generated uuid to client
								memset(message, 0, MESSAGE_SIZE);
								sprintf(message, "@register@username\n%s\nuuid\n%s", username, uuid_str);
								write(client_socket[current_client].sd, message, strlen(message));

								//save new client's authentication info to ini file
								server_addNewAuthenticationInfo( username, uuid_str, init_fp );
								init_fp = freopen( INI_FILE_NAME, "a+", init_fp);

								//save new client's authentication info to struct
								strcpy(client_socket[current_client].username, username);
								strcpy(client_socket[current_client].uuid_str, uuid_str);
							}
							//proposed username is already registered
							else{
								memset(message, 0, MESSAGE_SIZE);
								strncpy(message,"@register\ninvalid",MESSAGE_SIZE);
								write(client_socket[current_client].sd, message, strlen(message));
							}
						}
						//register procedure completed from client
						else if( strstr( buffer, "@register\ncompleted" ) ){
							//broadcast greeting new client to everyone
							memset(message, 0, MESSAGE_SIZE);
							sprintf(message, "Welcome %s to our group.\n", client_socket[current_client].username);
							server_broadcast( client_socket, MAX_CLIENT, message, MESSAGE_SIZE );
						}
					}
				}
			}
		}
	}

	return 0;
}

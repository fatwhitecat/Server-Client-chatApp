/* C standard library */
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

/* My defined libraries */
#include "chatlib.h"

#define TRUE   1
#define FALSE  0
#define PORT 2001
#define BUFFER_SIZE 500
#define MESSAGE_SIZE 500
#define INI_FILE_NAME "Client.ini"

int main()
{
	int sockfd, max_sd, activity;
	struct sockaddr_in address;
	char buffer[BUFFER_SIZE], message[MESSAGE_SIZE];

	FILE* ini_fp;
	UserInfo userinfo;
	memset( &userinfo, 0, sizeof(userinfo) );

	//create socket
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0 ){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//assign its address
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(PORT);

	//connect the client socket to server socket
	if( connect(sockfd, (struct sockaddr*)&address, sizeof(address)) != 0 ){
		perror("connect failed");
		exit(EXIT_FAILURE);
	}else{
		puts("Server connected...");
	}

	//ini file exists
	if( (ini_fp = fopen( INI_FILE_NAME, "r+" )) != NULL ){
		puts("Authentication info available. Send it to server to login..");
		client_getAuthenticalInfo( ini_fp, &userinfo );
		client_sendAuthenticalInfo( sockfd, &userinfo );
	}else{
		ini_fp = fopen( INI_FILE_NAME, "w+" );
		puts("Authentication info not available. Provide a user name to register.");
	}

	//set of socket descriptors
	fd_set readfds;

	while(TRUE){
		//clear the socket set
		FD_ZERO(&readfds);

		//add fd of stdin to set
		FD_SET(STDIN_FILENO, &readfds);

		//add socket to set
		FD_SET(sockfd, &readfds);
		max_sd = (sockfd>STDIN_FILENO)?sockfd:STDIN_FILENO;

		//wait for an activity on one of the sockets
		activity = select( max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno!=EINTR))
		{
			puts("select error");
		}

		//Check if what happened in STDIN
		if( FD_ISSET( STDIN_FILENO, &readfds ) ){
			memset(buffer, 0, BUFFER_SIZE);
			read(STDIN_FILENO, buffer, BUFFER_SIZE);

			//when user decide to exit
			if( !strcmp(buffer, "exit\n") ){
				//break the while loop to close socket
				break;
			}
			//this is message to server, when user already login,
			else if( (strlen(userinfo.name) != 0) && (strlen(userinfo.uuid_str) != 0) ){
				//cut off the newline character in message
				buffer[strcspn(buffer, "\n")] = 0;
				write( sockfd, buffer, BUFFER_SIZE );
			}
			//user is registering an user name...
			else{
				//cut off the newline character in message
				buffer[strcspn(buffer, "\n")] = 0;
				client_sendregisteredUsername( sockfd, buffer );
			}

		}

		//Check if any incoming message from server
		if( FD_ISSET(sockfd, &readfds) ){

			memset(buffer, 0, BUFFER_SIZE);

			//if server is closed
			if( read(sockfd, buffer, BUFFER_SIZE) == 0 ){
				//server is dead, break the loop and close socket
				puts("Server is dead\n");
				break;
			}
			//server is still alive, this is actual message from server
			else{
				//invalid authentication info, have to register again
				if( strstr( buffer, "@authentication\ninvalid" ) ){
					//Clear all contents inside ini file & open new instance
					ini_fp = freopen( INI_FILE_NAME, "w+", ini_fp );
					//reset login info to 0
					memset( &userinfo, 0, sizeof(userinfo) );
					puts("Provided authentication info invalid. Register a new user name.");
				}
				//valid authentication info
				else if( strstr( buffer, "@authentication\nvalid" ) ){
					memset( message, 0, MESSAGE_SIZE );
					sprintf( message, "Authentication info valid. Login successfully with user name : %s", userinfo.name );
					puts(message);
				}

				//invalid registered name
				else if( strstr( buffer, "@register\ninvalid") ){
					puts("This user name has been used. Try another.");
				}
				//register successfully
				else if( strstr( buffer, "@register@username" ) ){
					client_getNewAuthenticationInfo( buffer, userinfo.name, userinfo.uuid_str );
					client_addNewAuthenticationInfo( userinfo.name, userinfo.uuid_str, ini_fp );
					fclose(ini_fp);
					puts("You can use this user name. Server generated uuid has given to you.");

					memset( message, 0, MESSAGE_SIZE);
					sprintf(message, "@register\ncompleted" );
					write( sockfd, message, MESSAGE_SIZE );
				}

				//received private ACK
				else if( strstr( buffer, "@privatesentsuccessfully@" ) ){

					FILE* private_fd;
					char private_logname[50];
					char private_username[USERNAME_SIZE];
					char private_message[MESSAGE_SIZE];

					client_getPrivateMessage( buffer, private_username, private_message );
					sprintf( private_logname, "private_%s.txt", private_username );
					private_fd = fopen( private_logname, "a+" );
					client_savelogPrivateMessage( private_fd, "You", private_message );
					fclose( private_fd );
				}

				//received private message
				else if( strstr( buffer, "@private@" ) ){

					FILE* private_fd;
					char private_logname[50];
					char private_username[USERNAME_SIZE];
					char private_message[MESSAGE_SIZE];

					client_getPrivateMessage( buffer, private_username, private_message );
					sprintf( private_logname, "private_%s.txt", private_username );
					private_fd = fopen( private_logname, "a+" );
					client_savelogPrivateMessage( private_fd, private_username, private_message );
					fclose( private_fd );

					memset( message, 0, MESSAGE_SIZE );
					sprintf( message, "Private from %s to You : %s", private_username, private_message );
					puts( message );
				}

				//it's generic message from server
				else{
					puts(buffer);
				}
			}
		}
	}

	// close the socket
	close(sockfd);
	exit(EXIT_SUCCESS);
}

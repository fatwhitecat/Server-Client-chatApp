#ifndef CHATLIB_H
#define CHATLIB_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define USERNAME_SIZE 50
#define UUIDSTRING_SIZE 36

typedef struct {
	char name[USERNAME_SIZE];
	char uuid_str[UUIDSTRING_SIZE];
} UserInfo;

typedef struct {
	int sd;
	char username[USERNAME_SIZE];
	char uuid_str[UUIDSTRING_SIZE];
} Client_Socket;

void client_sendregisteredUsername( int sockfd, char* username );
void client_getNewAuthenticationInfo( char* svmessage, char* username, char* uuid_str );
void client_addNewAuthenticationInfo( char* username, char* uuid_str, FILE* fp );
void client_getAuthenticalInfo( FILE* fp, UserInfo* infoPtr );
void client_sendAuthenticalInfo( int sockfd, UserInfo* infoPtr );
void client_getPrivateMessage( char* given_message, char* username, char* private_message );
void client_savelogPrivateMessage( FILE* private_fd, char* private_username, char* private_message );


void server_broadcast( Client_Socket client_socket[], int max_client, char* message, int message_size );
void server_broadcastAliveClientList( Client_Socket client_socket[], int max_client );
short server_checkDuplicatedUUID( char* uuid_str, FILE* fp );
short server_checkDuplicatedUsername( char* username, FILE* fp );
short server_compareAuthenticationInfo( char* username, char* uuid_str, FILE* fp );
void server_addNewAuthenticationInfo( char* username, char* uuid_str, FILE* fp );
void server_getPrivateMessage( char* given_message, char* username, char* private_message );
short server_isClientLogin( char* pointed_username, int* pointed_sockfd_ptr, Client_Socket client_socket[], int max_client );
void server_sendPrivateMessage( int pointed_sockfd , char* username ,char* private_message );
void server_sendPrivateAck( int sockfd, char* pointed_username, char* private_message );

short server_getncompareAuthenticationInfo( char* message, int message_size, char* username, char* uuid_str, FILE* fp );


#endif

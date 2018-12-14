#include "chatlib.h"

/* broadcast given message to all client which already login in Client_Socket struct */
void server_broadcast( Client_Socket client_socket[], int max_client, char* message, int message_size ){

	for( int i = 0; i < max_client; i++ ){
		if( (client_socket[i].sd != STDIN_FILENO) && (strlen(client_socket[i].username) != 0) ){
			write( client_socket[i].sd, message, message_size );
		}
	}
}

/* broadcast all current login user name in Client_Socket struct */
void server_broadcastAliveClientList( Client_Socket client_socket[], int max_client ){

	char message[200];

	memset( message, 0, 200);
	strcpy( message, "Now we have these guys online : " );

	for( int i = 0; i < max_client; i++ ){
		if( strlen( client_socket[i].username ) != 0 ){
			strcat( message, client_socket[i].username );
			strcat( message, " ");
		}
	}

	server_broadcast( client_socket, max_client, message, 100 );
}

/* compare given uuid with existing one in FilePointer
 * return 1 when identical - 0 when not existing */
short server_checkDuplicatedUUID( char* uuid_str, FILE* fp ){

	char buffer[50];
	char* token;

	//navigate to the top of FilePointer
	fseek( fp, 0, SEEK_SET );

	//lookup and compare given uuid with existing uuid in FilePointer
	while( fgets( buffer, 50, fp ) != NULL ){
		if( strstr( buffer, "uuid=" ) != NULL ){
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "=" )) != NULL ){
				if( strcmp( uuid_str, token ) == 0 ){
					return 1;
				}
			}
		}
		memset( buffer, 0, 50 );
	}

	return 0;
}

/* compare given username with existing one in FilePointer
 * return 1 when identical - 0 when not existing */
short server_checkDuplicatedUsername( char* username, FILE* fp ){

	char buffer[100];
	char* token;

	//navigate to the top of FilePointer
	fseek( fp, 0, SEEK_SET );

	//lookup and compare given username with existing username in FilePointer
	while( fgets( buffer, 100, fp ) != NULL ){
		if( strstr( buffer, "username=" ) != NULL ){
			//cut off the newline character in message
			buffer[strcspn(buffer, "\n")] = 0;
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "=" )) != NULL ){
				if( strcmp( username, token ) == 0 ){
					return 1;
				}
			}
		}
		memset( buffer, 0, 50 );
	}

	return 0;
}

/* lookup authentication info given from Client among Server's init FilePointer
 * return 1 when existing - 0 when not existing */
short server_compareAuthenticationInfo( char* username, char* uuid_str, FILE* fp ){

	char buffer[60];
	char* token;
	int nameFlag = 0, uuidFlag = 0;

	//lookup in Server's init FilePointer
	while( fgets( buffer, 60, fp ) != NULL ){
		if( strstr( buffer, "username=" ) != NULL ){
			// reset uuidFlag for new instance
			uuidFlag = 0;
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "=" )) != NULL ){
				if( strcmp( username, token ) ){
					nameFlag = 1;
				}else{
					nameFlag = 0;
				}
			}
		}else if( strstr( buffer, "uuid=" ) != NULL ){
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "=" )) != NULL ){
				if( strcmp( uuid_str, token ) ){
					uuidFlag = 1;
				}else{
					uuidFlag = 0;
				}
			}
		}

		if( nameFlag & uuidFlag ){
			return 1;
		}

		memset( buffer, 0, 60);
	}

	return 0;
}

/* append given authentication info to given FilePointer */
void server_addNewAuthenticationInfo( char* username, char* uuid_str, FILE* fp ){

	char buffer[200];

	fseek( fp, 0, SEEK_END );

	memset( buffer, 0, 200 );
	strcpy( buffer, "\n\n[client information]" );
	fwrite( buffer, 1, strlen(buffer), fp );

	memset( buffer, 0, 200);
	sprintf( buffer, "\nusername=%s\nuuid=%s", username, uuid_str );
	fwrite( buffer, 1, strlen(buffer), fp);
}


/* retrieve username & private message info from given message --> then save it */
void server_getPrivateMessage( char* given_message, char* username, char* private_message ){

	char* token;

	//retrieve username & private information
	token = strtok( given_message, "@" );
	if( (token = strtok( NULL, "@" )) != NULL ){
		strcpy( username, token );
	}
	if( (token = strtok( NULL, "@" )) != NULL ){
		strcpy( private_message ,token );
	}
}

/* check if a username already login in Client_Socket struct --> then save its socketfd
 * return 1 when already login - 0 when not exiting in Client_Socket */
short server_isClientLogin( char* pointed_username, int* pointed_sockfd_ptr, Client_Socket client_socket[], int max_client ){

	for( int i = 0; i < max_client; i++ ){
		if( strcmp( pointed_username, client_socket[i].username ) == 0 ){
			*pointed_sockfd_ptr = client_socket[i].sd;
			return 1;
		}
	}

	return 0;
}

/*  */
void server_sendPrivateMessage( int pointed_sockfd , char* pointed_username ,char* private_message ){

	char buffer[500];

	memset( buffer, 0, 500 );
	sprintf( buffer, "@private@%s@%s", pointed_username, private_message );
	write( pointed_sockfd, buffer, 500 );
}

/* */
void server_sendPrivateAck( int sockfd, char* pointed_username, char* private_message ){

	char buffer[500];

	memset( buffer, 0, 500 );
	sprintf( buffer, "@privatesentsuccessfully@%s@%s", pointed_username, private_message );
	write( sockfd, buffer, 500 );
}

/* lookup init info given from Client in Server's init FilePointer
 * return 1 when existing - 0 when not existing */
short server_getncompareAuthenticationInfo( char* message, int message_size, char* username, char* uuid_str, FILE* fp ){

	char buffer[100];
	char* token;
	int nameFlag = 0, uuidFlag = 0;

	//retrieve username & uuid_str information
	token = strtok( message, "\n");
	for( int i = 0; i < 2; i++ ){
		if( (i == 0) && ((token = strtok(NULL, "\n")) != NULL) ){
			strcpy( username, token );
		}else if( (i == 1) && ((token = strtok(NULL, "\n")) != NULL) ){
			strcpy( uuid_str, token );
		}
	}

	//navigate to the top of FilePointer
	fseek( fp, 0, SEEK_SET );
	//lookup in Server's init FilePointer
	while( fgets( buffer, 100, fp ) != NULL ){
		if( strstr( buffer, "username=" ) != NULL ){
			// reset uuidFlag for new instance
			uuidFlag = 0;
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "\n" )) != NULL ){
				if( !strcmp( username, token ) ){
					nameFlag = 1;
				}else{
					nameFlag = 0;
				}
			}
		}else if( strstr( buffer, "uuid=" ) != NULL ){
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "\n" )) != NULL ){
				if( !strcmp( uuid_str, token ) ){
					uuidFlag = 1;
				}else{
					uuidFlag = 0;
				}
			}
		}

		if( nameFlag & uuidFlag ){
			return 1;
		}

		memset( buffer, 0, 100);
	}
	return 0;
}


/*  */
void client_addNewAuthenticationInfo( char* username, char* uuid_str, FILE* fp ){

	char buffer[200];

	fseek( fp, 0, SEEK_END );

	memset( buffer, 0, 200 );
	strcpy( buffer, "\n[client information]" );
	fwrite( buffer, 1, strlen(buffer), fp );

	memset( buffer, 0, 200);
	sprintf( buffer, "\nusername=%s\nuuid=%s", username, uuid_str );
	fwrite( buffer, 1, strlen(buffer), fp);
}

/*  */
void client_getNewAuthenticationInfo( char* svmessage, char* username, char* uuid_str ){

	char* token;

	token = strtok( svmessage, "\n");

	if( (token = strtok( NULL, "\n")) != NULL ){
		strcpy( username, token );
	}

	token = strtok( NULL, "\n");

	if( (token = strtok( NULL, "\n")) != NULL ){
		strcpy( uuid_str, token );
	}
}

/*  */
void client_sendregisteredUsername( int sockfd, char* username ){

	char message[100];

	memset( message, 0, 100);
	sprintf( message, "@register\nusername=%s", username);
	write( sockfd, message, 100);
}

/* get Client's username and uuid from Client's init FilePointer then save it to UserInfo struct */
void client_getAuthenticalInfo( FILE* fp, UserInfo* infoPtr ){

	char buffer[60];
	char* token;

	while( fgets( buffer, 60, fp ) != NULL ){
		buffer[strcspn(buffer, "\n")] = 0;
		if( strstr( buffer, "username=" ) != NULL ){
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "=" )) != NULL ){
				strcpy( infoPtr->name, token );
			}
		}
		else if( strstr( buffer, "uuid=" ) != NULL ){
			token = strtok( buffer, "=" );
			if( (token = strtok( NULL, "=" )) != NULL ){
				strcpy( infoPtr->uuid_str, token );
			}
		}
		memset( buffer, 0, 60);
	}
}


/* send Client's init info from UserInfo struct to given Server's socket fd */
void client_sendAuthenticalInfo( int sockfd, UserInfo* infoPtr ){

	char message[100];

	memset( message, 0, 100 );
	sprintf( message, "@authentication\n%s\n%s", infoPtr->name, infoPtr->uuid_str );
	write( sockfd, message, 100 );
}

/*  */
void client_getPrivateMessage( char* given_message, char* username, char* private_message ){

	char* token;

	//retrieve username & private information
	token = strtok( given_message, "@" );
	if( (token = strtok( NULL, "@" )) != NULL ){
		strcpy( username, token );
	}
	if( (token = strtok( NULL, "@" )) != NULL ){
		strcpy( private_message ,token );
	}
}

/*  */
void client_savelogPrivateMessage( FILE* private_fd, char* private_username, char* private_message ){

	char buffer[500];

	memset( buffer, 0, 500 );
	sprintf( buffer, "\n%s : %s", private_username, private_message );
	fwrite( buffer, 1, strlen(buffer), private_fd );
}

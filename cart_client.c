////////////////////////////////////////////////////////////////////////////////
//
//  File          : cart_client.c
//  Description   : This is the client side of the CART communication protocol.
//
//   Author       : ????
//  Last Modified : ????
//

// Include Files
#include <stdio.h>

// Project Include Files
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <cart_network.h>
#include <cart_driver.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <cart_driver.h>
//
//  Global data
int client_socket = -1;
int                cart_network_shutdown = 0;   // Flag indicating shutdown
unsigned char     *cart_network_address = NULL; // Address of CART server
unsigned short     cart_network_port = 0;       // Port of CART serve
unsigned long      CartControllerLLevel = LOG_INFO_LEVEL; // Controller log level (global)
unsigned long      CartDriverLLevel = 0;     // Driver log level (global)
unsigned long      CartSimulatorLLevel = 0;  // Driver log level (global)

uint64_t value;					//this is the opcode that we send to the server
struct sockaddr_in caddr;		//this is our socket
char *ip = CART_DEFAULT_IP;			//this is our ip address
//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_cart_bus_request
// Description  : This the client operation that sends a request to the CART
//                server process.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

CartXferRegister client_cart_bus_request(CartXferRegister reg, void *buf){
	int count=1024;		//size of each frame
	caddr.sin_family=AF_INET;
	caddr.sin_port=htons(CART_DEFAULT_PORT);		

	registervariables cart_start;		//this will hold the data after we extract
	
	cart_start= extract_cart_opcode(reg);	//extracting the returned 64bit number for varification

	if(client_socket==-1){		//the frist step of setting up the server
		if(inet_aton(ip,&caddr.sin_addr)==0){
			return(-1); 
		}

		client_socket = socket(PF_INET,SOCK_STREAM,0);	//socketing with the server

		if(client_socket == -1){
			//printf("Error on socket creation [%s]\n", strerror(errno));
			return( -1 );
		}

		if (connect(client_socket,(const struct sockaddr *)&caddr,sizeof(caddr))==-1){	//we connect
 			//printf( "Error on socket connect [%s]\n",strerror(errno));
 			return( -1 );
 		}
	}

 	if(cart_start.ky1==CART_OP_POWOFF){		//when we have to power off the server, we disconnect

 		value = htonll64(reg);		//converting from host to network format
 		
 		if(write(client_socket,&value,sizeof(value))!=sizeof(value)){			//sending the reg here
	 		//printf("Error writing network data [%s]\n",strerror(errno));
	 		return(-1);
	 	}

	 	if(read(client_socket,&value,sizeof(value))!=sizeof(value)){		//grtting the opcode back
	 		return (-1);
	 	}

	 	value = ntohll64(value);	//converting back to the host format

 		close(client_socket);
 		client_socket=-1;

 		return value;		//returning the opcode back to the driver file
 	}

	if(cart_start.ky1==CART_OP_WRFRME){
		count =CART_FRAME_SIZE;	//size of each frame
		value = htonll64(reg);		//converting from host to network format

		if(write(client_socket,&value,sizeof(value))!=sizeof(value)){			//sending the reg here
	 		//printf("Error writing network data [%s]\n",strerror(errno));
	 		return(-1);
	 	}

	 	if(write(client_socket,buf,count)!=count){			//sending the buf to server

	 		return (-1);
	 	}

	 	if(read(client_socket,&value,sizeof(value))!=sizeof(value)){		//grtting the opcode back
	 		return (-1);
	 	}

	 	value = ntohll64(value);		//converting back to the host format

	 	return value;		//returning the opcode back to the driver file

	}

	if(cart_start.ky1==CART_OP_RDFRME){	//this means we want to read from the server

		//int check;
		//uint64_t new_count=htonll64(count);
		//printf("we are here line 141");
		value = htonll64(reg);	//reg to network format
		if(write(client_socket,&value,sizeof(value))!=sizeof(value)){			//sending the reg here
	 		//printf("Error writing network data [%s]\n",strerror(errno));
	 		return(-1);
	 	}

		if(read(client_socket,&value,sizeof(value))!=sizeof(value)){		//grtting the opcode back
	 		return (-1);
	 	}

	 	value = ntohll64(value);		//converting back to the host format

		if(read(client_socket,buf,CART_FRAME_SIZE)!=CART_FRAME_SIZE){		//reading from the server
	 		return (-1);
	 	}

	 	return value;		//returning the opcode back to the driver file

	}

	if(cart_start.ky1==CART_OP_LDCART || cart_start.ky1==CART_OP_BZERO || cart_start.ky1==CART_REG_MAXVAL||cart_start.ky1==CART_OP_INITMS){
		count =0;		//size of each frame
		value=0;		//initializing the value

		value = htonll64(reg);	//converting from host to network format
		if(write(client_socket,&value,sizeof(value))!=sizeof(value)){			//sending the opcode here
	 		//printf("Error writing network data [%s]\n",strerror(errno));
	 		return(-1);
	 	}

		if(read(client_socket,&value,sizeof(value))!=sizeof(value)){		//grtting the opcode back
	 		return (-1);
	 	}

	 	value = ntohll64(value);		//converting back to the host format

	 	return value;		//returning the opcode back to the driver file
	}

	return 0;
}

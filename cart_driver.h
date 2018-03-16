#ifndef CART_DRIVER_INCLUDED
#define CART_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Thu Sep 15 15:05:53 EDT 2016
//

// Include files
#include <stdint.h>

// Defines
#define CART_MAX_TOTAL_FILES 1024 // Maximum number of files ever
#define CART_MAX_PATH_LENGTH 128 // Maximum length of filename length

//
// Interface functions

typedef struct{
	uint8_t ky1;
	uint8_t ky2;
	int rt1;
	uint16_t ct1;
	uint16_t fm1;

}registervariables;

typedef struct linked_list{
	int cart_num;					//cart number of the byte that is stored
	int frame_num;					//frame number of the place where the byte is stored
	struct linked_list *next;				//pointin to the next linked list 
	//int frame_full;					//this will let us know till how much is the frame full ie 1024==completely full			
}datastructure_list;	

typedef struct{
	char f_name[CART_MAX_PATH_LENGTH];	//this is gonna be the file name
	int flag;				//this checks if the file is open or close
	int posi;			//this sets the file read/write position
	int len;				//this sets the total length of the file
	uint16_t f_h;				//this is the file handle
	datastructure_list *head;				//points to the first frame of the linked list
	datastructure_list *tail; 				//points to the last frame of the frame
	//int node				//this will keep track of how many nodes this file currently has so that we can realloc
}file_tracker;


int32_t cart_poweron(void);
	// Startup up the CART interface, initialize filesystem

int32_t cart_poweroff(void);
	// Shut down the CART interface, close all files

int16_t cart_open(char *path);
	// This function opens the file and returns a file handle

int16_t cart_close(int16_t fd);
	// This function closes the file

int32_t cart_read(int16_t fd, void *buf, int32_t count);
	// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t cart_write(int16_t fd, void *buf, int32_t count);
	// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t cart_seek(int16_t fd, uint32_t loc);
	// Seek to specific point in the file

uint64_t create_cartopcode(registervariables creating_opcode); 	//this function will create the cart_opcode

registervariables extract_cart_opcode(uint64_t resp);	//this will extract opcode

datastructure_list *insert_node(datastructure_list *head, datastructure_list *tail,int file_index);

datastructure_list *search_node(datastructure_list *head, int file_index);

#endif



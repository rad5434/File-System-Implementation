////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the CRUD storage system.
//
//  Author         : [Raj Desai]
//  Last Modified  : [10/4/16]
//

// Includes
#include <stdlib.h>

// Project Includes
#include <cart_driver.h>
#include <cart_controller.h>
#include <cmpsc311_log.h>
#include <string.h>
#include <cart_cache.h>
#include <cart_network.h>

file_tracker file_array[CART_MAX_TOTAL_FILES];		//this is the total number of files

int file_num=0; //this will keep track of which index in the file_array can we 

int cart_curr=0;	//this will tell me which cart to look in to currently for frames

int frame_next=0;	//this will tell me which frame to use next
////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweron
// Description  : Startup up the CART interface, initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweron(void) {
	//with power on we first intialize the whole or cache system before doing anything
	// First initialize and than load it and than zero it.
	// Use for loop to do that.

	//we first initialize our cache system

	set_cart_cache_size(DEFAULT_CART_FRAME_CACHE_SIZE);

	uint64_t cartopcode_value;	// The valuse that we get from the create cart op code function

	registervariables cart_start;

	cart_start.ky1= CART_OP_INITMS;	// setting individual structs to the desired values
	cart_start.ky2= 0;				// before making the opcode
	cart_start.rt1= 0;
	cart_start.ct1=0;
	cart_start.fm1=0;

	cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

	cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
	//get from the controller

	cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

	if(cart_start.rt1==-1){								//Checking for success

		return(-1);											//returning -1 back if there is a failure
	}


	for(int i=0;i<CART_MAX_CARTRIDGES;i++){

		registervariables cart_start;

		cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
		cart_start.ky2= 0;				// before making the opcode
		cart_start.rt1= 0;
		cart_start.ct1=i;
		cart_start.fm1=0;

		cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

		cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
		//get from the controller
		cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification


		if(cart_start.rt1==-1){								//Checking for success

			return(-1);											//returning -1 back if there is a failure
		}

		cart_start.ky1= CART_OP_BZERO;	// setting individual structs to the desired values
		cart_start.ky2= 0;				// before making the opcode
		cart_start.rt1= 0;
		cart_start.ct1=i;
		cart_start.fm1=0;

		cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode


		cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
		//get from the controller

		cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

		if(cart_start.rt1==-1){								//Checking for success

			return(-1);											//returning -1 back if there is a failure
		}
	}

	// Return successfully
		return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweroff
// Description  : Shut down the CART interface, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweroff(void) {

	//we also need to close the cache here

	close_cart_cache();
	
	datastructure_list *temp1;
	datastructure_list *temp2;

	for(int i=0;i<CART_MAX_CARTRIDGES;i++){

		logMessage(LOG_INFO_LEVEL, "The file frreed is [%d]th file ....",i);
		
		temp1=file_array[i].head;
		
		while(temp1!=file_array[i].tail){
			temp2=temp1;
			temp1=temp1->next;
			free(temp2);
		}

		if(temp1==file_array[i].tail){
			free(temp1);
		}
	}

	uint64_t cartopcode_value;	// The valuse that we get from the create cart op code function

	registervariables cart_start;


	cart_start.ky1= CART_OP_POWOFF;	// powering off the whole sys
	cart_start.ky2=0;				// before making the opcode
	cart_start.rt1=0;
	cart_start.ct1=0;
	cart_start.fm1=0;

	cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode


	cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
															//get from the controller

	cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

	if(cart_start.rt1==-1){								//Checking for success of the shutting down

		return(-1);											//returning -1 back if there is a failure
	}

	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t cart_open(char *path) {


	int exists=-1;			//exists checks if the file exists or not. 0 means it exists

	char temp[CART_MAX_PATH_LENGTH];
	
	strncpy(temp,path,128);			//so that we have an array and its pointer locally

	//now we will have to check if the file exists in the entire array of file that we declared globally
	
	int index =0;			//to keep track of the while loop

	while(exists!=0 && index<1024){		//0 means it exists

		exists = strncmp(temp,file_array[index].f_name,128);

		if(exists !=0){

			index++;
		}

	}

	if(exists==0){	// this means the file exists

		file_array[index].flag=0;				//0 denotes that the file is now open

		file_array[index].posi=0;				//its read write position is being set to zero

		return file_array[index].f_h; 			//we open the file and return the file handle
	}

	else if(exists!=0){				//we know that the file does not exists so we create one
		
		strncpy(file_array[file_num].f_name,path,128);		//setting the name equal to path

		file_array[file_num].f_h=file_num;			//we assign the file handle to be the file_num

		file_num++;

		file_array[index].flag=0;					//zero means that the file is now open

		file_array[index].len=0;					//len is set to zero since the file is empy as we just created it

		return file_array[file_num-1].f_h;


	}

	else{								//this means the file handle was bad

		return (-1);
	}

	
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t cart_close(int16_t fd) {

	if(file_array[fd].flag==0 && file_array[fd].f_h==fd){	//this means the file is open and file handle is valid

		file_array[fd].flag=1;		//this instruction closes the file

		return (0);				//operation succesful
	}

	else{

		return -1;		//this means the file handle was bad
	}
	// Return successfully
	return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t cart_read(int16_t fd, void *buf, int32_t count){

	registervariables cart_start;

	registervariables cart_value;	//the data structure that we send to the iobus

	uint64_t cartopcode_value;	// The values that we get from the create cart op code function once we call it

	char temp_array[1024];			//temp array will have all the bytes of a single frame

	datastructure_list *temp=NULL;		//this will point to the frame to read from

	int rd_frame=0;		//this is the frame we do read on

	int rd_cart=0; 		//cart to read from

	int index=0;

	int count_len=count; 			//so that we dont change the length of the original count

	int posi=file_array[fd].posi;

	int start_read=posi%1024;		//the posi inside the frame we want to start reading from

	int check=-2;		//this check if the frame is in the cache or not

	if(file_array[fd].f_h==fd && file_array[fd].flag==0){		//if the file is valid and open

		if(file_array[fd].posi%1024!=0 && count_len<1024){	//if the posi is not equal to zero, we then have to read the 
								//the whole frame first and then send back on the part of the frame the user asked for

			//first of all, we will have to search for the frame. search_node will do that for us
			temp = search_node(temp,fd);	//temp will point to the node that has the frame num and cart num

			check=check_cache(rd_cart,rd_frame);

			if(check==-1){

				cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
				cart_start.ky2= 0;				// before making the opcode
				cart_start.rt1= 0;
				cart_start.ct1=rd_cart;
				cart_start.fm1=0;

				cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

				cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																		//get from the controller
				cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification


				cart_value.ky1=CART_OP_RDFRME;		//preparing the call iobus from read
				cart_value.ky2=0;
				cart_value.rt1=0;
				cart_value.fm1=rd_frame;			//frame and cart to read from
				cart_value.ct1=rd_cart;

				cartopcode_value = create_cartopcode(cart_value);

				cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);	//temp_array has the whole array

				cart_value = extract_cart_opcode(cartopcode_value);	

				if(cart_value.rt1==-1){			//instruction unsuccesfull

						return(-1);
				}

			}

			else{

				memcpy(temp_array,get_cart_cache(rd_cart,rd_frame),1024);
			}


			//Now we need to read from the temp_array on the data that the user asked for

			if(count_len<(1024-(posi%1024))){	//if the total count is less thn the whole frame
			
				memcpy(buf,&temp_array[start_read],count);	//we copy the whole count at once
				
				file_array[fd].posi+=count;
				count_len-=count;
				index+=index;

				return(count);//we are done with the read operation
			}

			else{	//if the count is greater than the whole frame, we read till the end of the frame

				memcpy(buf,&temp_array[start_read],(1024-start_read));	//copy the frame accordingly
				
				//here we change count_len,index and file posi. along with that we also need to increment
				//the temp to next so that we know whats the next frame/cart
				temp=temp->next;

				rd_cart=temp->cart_num;		//this will give us the next frame and cart num
				rd_frame=temp->frame_num;

				count_len-=(1024-start_read);
				index+=1024-start_read;
				file_array[fd].posi+=1024-start_read;	//we also change the posi

			}

		}

		while(count_len>=1024 && file_array[fd].posi%1024==0){			

			if(count_len!=count){		//this means that we know the frame and cart number that we need right now as,
						//we came to this while loop from the above if statement
				check=check_cache(rd_cart,rd_frame);

				if(check==-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=rd_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

					cart_value.ky1=CART_OP_RDFRME;		// preparing to send data to the iobus
					cart_value.ky2=0;
					cart_value.rt1=0;
					cart_value.fm1=rd_frame;		//this is the frame	and cart we need to read from	
					cart_value.ct1=rd_cart;

					cartopcode_value = create_cartopcode(cart_value);

					cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);	//temp array has the entire frame we need

					cart_value = extract_cart_opcode(cartopcode_value);	

					if(cart_value.rt1==-1){			//instruction unsuccesfull
						return(-1);
					}

				}

				else{

					memcpy(temp_array,get_cart_cache(rd_cart,rd_frame),1024);
				}

				

				memcpy(buf+index,temp_array,1024);		//we copy the whole temp array and store it in the indexth
					//location in the buf.

				count_len-=1024;
				index+=1024;
				file_array[fd].posi+=1024;

				//now we check if the count_len is zero or not. if not, we find the location of the next node
				if(count_len!=0){

					temp=temp->next;

					rd_cart=temp->cart_num;		//this will give us the next frame and cart num
					rd_frame=temp->frame_num;
				}

				else{

					return count;//we are done with the read operation
				}

			}

			else{		//this means that the we directly came to this while loop and we dont know the pois
					//of the frame/cart that we want to use

				//so first of all we search for the frame/cart we want
				temp = search_node(temp,fd);	//temp will point to the node that has the frame num and cart num

				rd_cart=temp->cart_num;
				rd_frame=temp->frame_num;	//we have the frame and the cart num

				check=check_cache(rd_cart,rd_frame);

				//then we send the read command to the iobus

				if(check==-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=rd_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

					cart_value.ky1=CART_OP_RDFRME;		// preparing to send data to the iobus
					cart_value.ky2=0;
					cart_value.rt1=0;
					cart_value.fm1=rd_frame;		//this is the frame	and cart we need to read from	
					cart_value.ct1=rd_cart;

					cartopcode_value = create_cartopcode(cart_value);

					cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);	//temp array has the entire frame we need

					cart_value = extract_cart_opcode(cartopcode_value);	

					if(cart_value.rt1==-1){			//instruction unsuccesfull
						return(-1);
					}

				}

				else{

					memcpy(temp_array,get_cart_cache(rd_cart,rd_frame),1024);
				}

				memcpy(buf+index,temp_array,1024);		//we copy the whole temp array and store it in the indexth
					//location in the buf.

				count_len-=1024;
				index+=1024;
				file_array[fd].posi+=1024;

				if(count_len!=0){

					temp=temp->next;

					rd_cart=temp->cart_num;		//this will give us the next frame and cart num
					rd_frame=temp->frame_num;
				}

				else{

					return count;//we are done with the read operation
				}

			}


		}

		if(count_len <1024 && file_array[fd].posi%1024==0){

			if(count_len!=count){		//this means that we know the frame and cart number that we need right now as,
						//we came to this if statement from the above if statement or while loop

				check=check_cache(rd_cart,rd_frame);

				if(check==-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=rd_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

					cart_value.ky1=CART_OP_RDFRME;		// preparing to send data to the iobus
					cart_value.ky2=0;
					cart_value.rt1=0;
					cart_value.fm1=rd_frame;		//this is the frame	and cart we need to read from	
					cart_value.ct1=rd_cart;

					cartopcode_value = create_cartopcode(cart_value);

					cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);  //temp array has the entire frame we need

					cart_value = extract_cart_opcode(cartopcode_value);	

					if(cart_value.rt1==-1){			//instruction unsuccesfull
						return(-1);
					}

				}

				else{

					memcpy(temp_array,get_cart_cache(rd_cart,rd_frame),1024);
				}

				memcpy(buf+index,temp_array,count_len);		//we copy the whole temp array and store it in the indexth
					//location in the buf.

				count_len-=count_len;	//we read the last count_len bytes
				index+=count_len;
				file_array[fd].posi+=count_len;

				return count; //we are done with the read operation

			}

			else{		//this means that the we directly came to this if statement and we dont know the pois
					//of the frame/cart that we need to use

				//so first of all we search for the frame/cart we want

				temp = search_node(temp,fd);	//temp will point to the node that has the frame num and cart num

				rd_cart=temp->cart_num;
				rd_frame=temp->frame_num;	//we have the frame and the cart num

				//then we send the read command to the iobus

				check=check_cache(rd_cart,rd_frame);

				if(check==-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=rd_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

					cart_value.ky1=CART_OP_RDFRME;		// preparing to send data to the iobus
					cart_value.ky2=0;
					cart_value.rt1=0;
					cart_value.fm1=rd_frame;		//this is the frame	and cart we need to read from	
					cart_value.ct1=rd_cart;

					cartopcode_value = create_cartopcode(cart_value);

					cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);	//temp array has the entire frame we need

					cart_value = extract_cart_opcode(cartopcode_value);	

					if(cart_value.rt1==-1){			//instruction unsuccesfull
						return(-1);
					}
				}

				else{

					memcpy(temp_array,get_cart_cache(rd_cart,rd_frame),1024);
				}
				memcpy(buf+index,temp_array,count_len);		//we copy the whole temp array and store it in the indexth
					//location in the buf.

				count_len-=count_len;
				index+=count_len;
				file_array[fd].posi+=count_len;

				return count; //we are done with read 

			}

		}

	}

	else{
		
		return -1; //something is wrong

	}


	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure


int32_t cart_write(int16_t fd, void *buf, int32_t count) {

	registervariables cart_start;

	registervariables cart_value;

	uint64_t cartopcode_value;	// The values that we get from the create cart op code function once we call it

	int posi=file_array[fd].posi; //the posi of the current file

	int count_len=count; 			//so that we dont change the length of the original count

	char temp_array[1024];			//this is the temp array that we will pass to the iobus

	datastructure_list *temp=NULL;		//this temp will tell us the frame/cart that we need at the moment

	int rd_frame=0;				//the frame to read from

	int rd_cart=0;				//the cart to read from

	int wr_frame=0;				//the frame to write to

	int wr_cart=0;				//cart to write to

	int written=0;				//amt of bytes written to the frames. written=count being the max bytes

	int check=-2;			//this will tell us if the frame exists in the cache or not

//first of all we check if the file handle is valid and if the file is open

	if(file_array[fd].f_h==fd && file_array[fd].flag==0){	//checking above mentioned conditions
		//if all the above conditions hold true, we write at the posi where file.posi is pointing

			//step 1
			if(file_array[fd].posi%1024!=0){	//this means that we already have atleast one frame assigned to this file
										//and that we are supposed to write to somewhere in the middle of that file

				//before writing, we would need to read back the thing we wrote to the frame before. Search node function 
				// will help us to get the frame we want to read from

				temp=search_node(temp,fd);	//after this is called, we will know what frame we need to send to iobus

				rd_frame=temp->frame_num;		//we need to get data from this frame

				rd_cart =temp->cart_num;		//from this cart

				check=check_cache(rd_cart,rd_frame);

				if(check==-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=rd_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

					cart_value.ky1=CART_OP_RDFRME;		// preparing to read data from the frames
					cart_value.ky2=0;
					cart_value.rt1=0;
					cart_value.fm1=rd_frame;			//this has to be the frame where the posi is pointing	
					cart_value.ct1=rd_cart;				//inside this cart

					cartopcode_value = create_cartopcode(cart_value);

					cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);	//this temp array will grab all the data

					cart_value = extract_cart_opcode(cartopcode_value);	

					if(cart_value.rt1==-1){			//instruction unsuccesfull

						return(-1);
					}

				}
				else{

					memcpy(temp_array,get_cart_cache(rd_cart,rd_frame),1024);

				}

				if(count_len>1024-posi%1024){	//if the count size if greater than the free space availabe in the curr frame,
								//we write to the end of frame.

					memcpy(&temp_array[posi%1024],buf+written,1024-posi%1024);	//copy (1024-posi%1024) bytes from buf at index
														//to the temp_array at index (posi%1024)
					written+=(1024-posi%1024);
					file_array[fd].posi+=written;		//increasing the value of the file posi
					count_len=count_len-(1024-posi%1024);		//decreasing the amount of bytes we are supposed to write

				//we still need to write more data to the file. we will do that in step 2 or step 3
				}

				else{		//if count is smaller than or equal to one frame,

					memcpy(&temp_array[posi%1024],(buf+written),count);	//we copy the whole buffer

					file_array[fd].posi+=count;		//increasing the value of the file posi
					written+=count;					//we wrote all we had to
					count_len-=count_len;				//we dont have anything to write now
				}

				//now we need to send stuff to iobus so that it can store it

				wr_frame= rd_frame;	//cart and frames are the same as read
				wr_cart= rd_cart;	

				if(check!=-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=wr_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification


				}

				cart_value.ky1=CART_OP_WRFRME;
				cart_value.ky2=0;				//preparing to send the data over ot the iobus
				cart_value.rt1=0;
				cart_value.fm1=wr_frame;
				cart_value.ct1=wr_cart;

				cartopcode_value = create_cartopcode(cart_value);

				cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);

				cart_value = extract_cart_opcode(cartopcode_value);	

				if(cart_value.rt1==-1){			//instruction unsuccesfull

					return(-1);
				}

				//now we put this frame in the cache

				put_cart_cache(wr_cart, wr_frame, temp_array);


				if(file_array[fd].len<file_array[fd].posi){	//if the file len is less than the posi,

					file_array[fd].len=file_array[fd].posi;	//inc the file len

				}


			}

			while(count_len>=1024){	//if count_len is greater than 1024 and if the posi of the file is a the first byte of
						//the frame, we can directly write through ntire frame.

				if(file_array[fd].len==0){	//this means that this the first write to this file and we need 
									//to give it a frame to write on. Insert fun will help us with that

					file_array[fd].tail=insert_node(file_array[fd].head,file_array[fd].tail,fd);	
															//it will return 2 pointers to the file_array. head and tail

					wr_frame=file_array[fd].head->frame_num;	//cart and frame that we would be using
					wr_cart=file_array[fd].head->cart_num;

					//we already inc the cart and frame in the insert fun

					//now we know which frame to write data to, now lets get the pointer that can be sent along with
					//frame and cart num

					memcpy(temp_array,buf+(written),1024);	//we we copy the 1024 bytes to the temp array
											//here index may or may not be zero

					file_array[fd].posi+=1024;
					written+=1024;
					count_len-=1024;


				}

				else{			//if the len is not zero there is already some frames assigned to it,

					//we dont need to insert the node if the file posi is still less than the len. But we need to assign
					//as soon as the posi is more than the len

					if(file_array[fd].len>file_array[fd].posi+(1024-file_array[fd].posi%1024)){	
						//this means that the len of the file is greater than the end of the frame. hence we dont
						//need to assign it a new frame right now. So we just need the search node function

						temp=search_node(temp,fd);		//temp will gives us the frame and cart num we need	to write to 

						wr_frame = temp->frame_num;
						wr_cart = temp->cart_num;

						memcpy(temp_array,buf+(written),1024);	//we copy the 1024 bytes to the temp array
											//here written may or may not be zero depending on how much we wrote before

						file_array[fd].posi+=1024;
						written+=1024;
						count_len-=1024;

					}

					else{			//if the len is less if the above if condition, we will have to insert a frame

						file_array[fd].tail = insert_node(file_array[fd].head,file_array[fd].tail,fd);
								//here the tail has the next node we are looking for. we will also assign it a new
									//frame and cart number.

						wr_frame=file_array[fd].tail->frame_num;	//the cart and frame we would be sending
																					//to iobus
						wr_cart=file_array[fd].tail->cart_num;
						
						//we already inc the cart and frame in the insert fun

						memcpy(temp_array,buf+(written),1024);	//we copy the 1024 bytes to the temp array
											//here written may or may not be zero depending on how much we wrote before

						file_array[fd].posi+=1024;
						written+=1024;
						count_len-=1024;


					}


				}

				//before writing to the frame, we will need to know which frame to write on, search will help us with that

				if(check!=-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=wr_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

				}
				cart_value.ky1=CART_OP_WRFRME;
				cart_value.ky2=0;
				cart_value.rt1=0;
				cart_value.fm1=wr_frame;
				cart_value.ct1=wr_cart;

				cartopcode_value = create_cartopcode(cart_value);

				cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);

				cart_value = extract_cart_opcode(cartopcode_value);	

				if(cart_value.rt1==-1){			//instruction unsuccesfull

						return(-1);
				}

				//now we need to get this frame in to the cache

				put_cart_cache(wr_cart, wr_frame, temp_array);


				if(file_array[fd].len<file_array[fd].posi){	//if the file len is less than the posi,

						file_array[fd].len=file_array[fd].posi;	//inc the file len

					}


			}

			//step 3
			if(file_array[fd].posi%1024==0){			//for the conditions where the writing starts from the 
								//begining of the frame

				if(file_array[fd].posi==file_array[fd].len){ //if the file posi and len are same then
									//we know we have to assign a new frame to the next write we do

					file_array[fd].tail=insert_node(file_array[fd].head,file_array[fd].tail,fd);
								//here the tail has the next node we are looking for. we will also assign it a new
									//frame and cart number.

					wr_frame=file_array[fd].tail->frame_num;	//the cart and frame we would be sending
																					//to iobus
					wr_cart=file_array[fd].tail->cart_num;
					
					//we already inc the cart and frame in the insert fun

					//we are left with writing on the last part on the last frame

					memcpy(temp_array,buf+(written),count_len);	//we copy the 1024 bytes to the temp array
										//here written may or may not be zero depending on how much we wrote before

					file_array[fd].posi+=count_len;
					written+=count_len;
					count_len-=count_len;


				}

				else{	//we know that the len of the file is greater than the posi
					//this means that the frame is already there and there is a possibility that we are 
					//over writing it

					temp=search_node(temp,fd);	//after this is called, we will know what frame we need to send to iobus

					rd_frame=temp->frame_num;		//we need to get data from this frame

					rd_cart =temp->cart_num;		//from this cart

					check=check_cache(rd_cart,rd_frame);

					if(check==-1){

						cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
						cart_start.ky2= 0;				// before making the opcode
						cart_start.rt1= 0;
						cart_start.ct1=rd_cart;
						cart_start.fm1=0;

						cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

						cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																				//get from the controller
						cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

						cart_value.ky1=CART_OP_RDFRME;		// preparing to read data from the frames
						cart_value.ky2=0;
						cart_value.rt1=0;
						cart_value.fm1=rd_frame;			//this has to be the frame where the posi is pointing	
						cart_value.ct1=rd_cart;				//inside this cart

						cartopcode_value = create_cartopcode(cart_value);

						cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);	//this temp array will grab all the data

						cart_value = extract_cart_opcode(cartopcode_value);	
					}

					else{

						memcpy(temp_array,get_cart_cache(rd_cart,rd_frame),1024);

					}
					//after we get the data, we need to write in the beginning of the file

					memcpy(temp_array,buf+(written),count_len);	//we copy the 1024 bytes to the temp array
										//here written may or may not be zero depending on how much we wrote before

					file_array[fd].posi+=count_len;
					written+=count_len;
					count_len-=count_len;

					wr_frame=rd_frame;
					wr_cart=rd_cart;

				}

				if(check!=-1){

					cart_start.ky1= CART_OP_LDCART;	// setting individual structs to the desired values
					cart_start.ky2= 0;				// before making the opcode
					cart_start.rt1= 0;
					cart_start.ct1=wr_cart;
					cart_start.fm1=0;

					cartopcode_value= create_cartopcode(cart_start);		//sending the structure to get an opcode

					cartopcode_value = client_cart_bus_request(cartopcode_value,NULL);	//sending and then storing the 64bit number that we 
																			//get from the controller
					cart_start= extract_cart_opcode(cartopcode_value);	//extracting the returned 64bit number for varification

					
				}
			
				cart_value.ky1=CART_OP_WRFRME;
				cart_value.ky2=0;
				cart_value.rt1=0;
				cart_value.fm1=wr_frame;
				cart_value.ct1=wr_cart;

				cartopcode_value = create_cartopcode(cart_value);

				cartopcode_value = client_cart_bus_request(cartopcode_value,temp_array);

				cart_value = extract_cart_opcode(cartopcode_value);	

				if(cart_value.rt1==-1){			//instruction unsuccesfull
						return(-1);
				}

				//now we need to update the cache also with this frame

				put_cart_cache(wr_cart, wr_frame, temp_array);

				if(file_array[fd].len<file_array[fd].posi){	//if the file len is less than the posi,

						file_array[fd].len=file_array[fd].posi;	//inc the file len

				}

			}

			return count;
	}
	else{			//if the if cond is not valid, return neg 1

		return (-1);
	}

	return(0);
	
}



////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t cart_seek(int16_t fd, uint32_t loc) {

	if(loc<0 && loc>file_array[fd].len){				//if loc<0 and the loc is not greater than end of file
		return(-1);
	}

	else{

		file_array[fd].posi=loc;						//if not than assign loc to posi
		if(loc == 0 )
		// Return successfully
		return (0);
	}
	return (0);
}

uint64_t create_cartopcode(registervariables creating_opcode){		//defining the structure defined in header file

	uint64_t b_number=0;
	uint64_t b2_number=0;

	b_number=creating_opcode.ky1;
	b_number=b_number << 56;
	b2_number=b_number;

	b_number=creating_opcode.ky2;
	b_number=b_number << 48;
	b2_number=b2_number | b_number;

	b_number=creating_opcode.rt1;
	b_number=b_number << 47;
	b2_number=b2_number | b_number;

	b_number=creating_opcode.ct1;
	b_number=b_number << 31;
	b2_number=b2_number | b_number;
	
	b_number=creating_opcode.fm1;
	b_number=b_number << 15;
	b2_number=b2_number | b_number;

	return b2_number;

}

registervariables extract_cart_opcode(uint64_t resp){

	registervariables i_number;

	uint64_t temp1= resp, temp2= resp, temp3= resp, temp4= resp, temp5= resp;

	i_number.ky1 = temp1 >> 56;

	temp2 = temp2 << 8;
	i_number.ky2 = temp2 >> 56;

	temp3 = temp3 << 16;
	i_number.rt1 = temp3>>63;

	temp4 = temp4 << 17;
	i_number.ct1 = temp4>> 48;

	temp5 = temp5 << 33;
	i_number.fm1 = temp5>> 48;

	return i_number;
}

datastructure_list *insert_node(datastructure_list *head, datastructure_list *tail,int file_index){		//it will insert a node(frame) 
																			//at the end

	if(head==NULL && tail==NULL){		//here the head is from the file data structure and so is the tail

		//file_array[file_index].node=1;	//since this would be the first node of this file

		file_array[file_index].head = (datastructure_list *)malloc(sizeof(datastructure_list));

		file_array[file_index].head->cart_num=cart_curr;	//curr frame we are working in

		file_array[file_index].head->frame_num=frame_next; 	//next frame that we are gonna use

		tail=file_array[file_index].head;

	}

	else{								//here the head is from the file data structure whereas tail would be from frame

		//file_array[file_index].node++;		//we inc the 

		tail->next=(datastructure_list *)malloc(sizeof(datastructure_list));

		tail=tail->next;

		tail->cart_num=cart_curr;	//curr frame we are working in

		tail->frame_num=frame_next; 	//next frame that we are gonna use

	}
	
	frame_next++;		//increment the frame so that next file can use it


	if(frame_next>1023){

		frame_next=0;	//next frame to use
		cart_curr++;	//curr cart to use
	}

	return tail;

}

datastructure_list *search_node(datastructure_list *head, int file_index){	//search would only tell me which frame
																	//to read next from

	int index=0; //to know how many bytes we have passed through

	datastructure_list *temp;		//this will point to the frame we want to change

	if(file_array[file_index].posi<1024){	//checking if its pointing to the first frame of the file

		temp = file_array[file_index].head;
	}

	else{								//if not first than 

		temp = file_array[file_index].head;

		index+=1024;

		while(file_array[file_index].posi>index+(file_array[file_index].posi%1024)){

			index=index+1024;

			temp = temp->next;

		}
		
		temp = temp->next;
	}

	return temp;		//the frame and cart num that this temp poiter has is what we need to use
}

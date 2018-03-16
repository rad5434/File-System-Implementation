////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_cache.c
//  Description    : This is the implementation of the cache for the CART
//                   driver.
//
//  Author         : [** YOUR NAME **]
//  Last Modified  : [** YOUR DATE **]
//

// Includes

// Project includes

// Defines

//
// Functions
#include <stdlib.h>

// Project Includes
#include <cart_cache.h>
#include <cart_driver.h>
#include <cart_controller.h>
#include <cmpsc311_log.h>
#include <string.h>
#include <time.h>


cache_array *cac;
cache_array *unit_cache;
int total_frames=0;
char temp_array[1024];
char *temp_ptr;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : set_cart_cache_size
// Description  : Set the size of the cache (must be called before init)
//
// Inputs       : max_frames - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure

int set_cart_cache_size(uint32_t max_frames){

	total_frames=max_frames;
	int check = init_cart_cache();

	if(check==-1){

		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_cart_cache
// Description  : Initialize the cache and note maximum frames
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int init_cart_cache(void){	//in this function we allocate the part of the memory to our cache
	clock_t init_time=clock();
	int i=0;
	cac=(cache_array*)malloc(total_frames*sizeof(cache_array));	//this will give us the max num of caches we need

	while(i<total_frames){
		cac[i].frame=-2;
		cac[i].cart=-2;
		cac[i].flag=-2;	//-2 means the cache is now intialized
		cac[i].time=init_time;	//this is initialization time
		i++;		
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_cart_cache
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure

int close_cart_cache(void) {
	int i=0;
	while(i<total_frames){
		cac[i].flag=-100;	//-100 means the cache is now closed
		i++;		
	}
	free(cac);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_cart_cache
// Description  : Put an object into the frame cache
//
// Inputs       : cart - the cartridge number of the frame to cache
//                frm - the frame number of the frame to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure

int put_cart_cache(CartridgeIndex cart, CartridgeIndex frm, void *buf){ 
		//cart_driver.c sends us the data we have to put inside the cache, we simply put it in to the cache
	int i=0;
	int check=-2;//-1 denotes that we found the empty slot
	clock_t use_time=clock();	//we set the least time to be that of the first frame of cache

	while(i<total_frames && check!=-1){	//we insert the frame where we find the next empty slot

		
		if(cac[i].cart==cart && cac[i].frame==frm){	//if we do find a frame that is has a similar frame and cart number

			memcpy(cac[i].byte_data,buf,1024);	//copy the whole frame to the cache
			cac[i].time=clock();		//we change its time to current time as the last time we needed it was right now
			check=-1;	//we found an the frame so we can now stop the while loop
			return 0;
		}

		else if(cac[i].time<use_time && cac[i].flag!=-2){	//we keep track of the time just in case we dont find the frame we want
			
			use_time=cac[i].time;
			check=i;		//we set the that index to check so that we know what element to replace if we dont find an empty space
			i++;
		}

		else if(cac[i].flag==-2){	//if we find an unitialized frame in cache
			//we copy the buffer and put it in that unitialized frame
			memcpy(cac[i].byte_data,buf,1024);	//copy the whole frame to the cache
			cac[i].cart=cart;	//we assign the frame and cart number it
			cac[i].frame=frm;
			cac[i].time=clock();		//we change its time to current time as the last time we needed it was right now
			cac[i].flag=0;
			check=-1;	//we found an the frame so we can now stop the while loop
			return 0;
		}

		else{
			i++;	//increment i to check the next frame in the cache
		}
	}
		
		if(i==total_frames && check!=-1){	
			//this means that the cache is full and we dont have the frame we are looking for in the cache
				//hence we delete the frame that was first used and we store the new frame in its place
			
			//cac=(cache_array*)delete_cart_cache(cac[check].cart,cac[check].frame,check);

			memcpy(cac[check].byte_data,buf,1024);	//copy the whole buf to the cache
			cac[check].cart=cart;	//assigning the cart and frame number to it
			cac[check].frame=frm;
			cac[check].flag=0;	//this frame is now full
			cac[check].time=clock();
			check=-1;
			return 0;
		}
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_cart_cache
// Description  : Get an frame from the cache (and return it)
//
// Inputs       : cart - the cartridge number of the cartridge to find
//                frm - the  number of the frame to find
// Outputs      : pointer to cached frame or NULL if not found

void * get_cart_cache(CartridgeIndex cart, CartridgeIndex frm){	
			//here we just have to get data from the cache and send it back to the cart_driver.
			//if the data is not there, we take it from the disk
	int i=0;	//our counter
	int check=-2;	//-1 denotes that we found the frame

	while(i<total_frames && check!=-1){		
				//we first find the frame which we want to send back to the cart_driver.c.this has to 
				//come before we find the last frame in the cache or an uninitialized frame

		if(cac[i].cart==cart && cac[i].frame==frm && cac[i].flag!=-2){	//if we do find a frame that is has a similar frame and cart number

			memcpy(temp_array,cac[i].byte_data,1024);	//copy the whole frame from the cache to the temp_array that we are gonna send back
			cac[i].time=clock();		//we change its time to current time as the last time we needed it was right now
			check=-1;	//we found an the frame so we can now stop the while loop
			return temp_array;
		}

		else if(cac[i].flag==-2){	//this means we did not find the frame we wanted yet and we encountered a frame that was just intialized
			
			temp_ptr=NULL;	//we were unable to find that frame
			return temp_ptr;
		}
		else{

			i++;	//increment i to check the next frame in the cache
		}
	}

	if(i==total_frames && check==-2){	//this means the frame does not exists in the cache

		temp_ptr=NULL;	//we were unable to find that frame
		return temp_ptr;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : delete_cart_cache
// Description  : Remove a frame from the cache (and return it)
//
// Inputs       : cart - the cart number of the frame to remove from cache
//                blk - the frame number of the frame to remove from cache
// Outputs      : pointe buffer inserted into the object

/*void * delete_cart_cache(CartridgeIndex cart, CartridgeIndex blk,int index){
		//this functions merely deletes the frame from the cache. 

	cac[index].frame=-1;
	cac[index].cart=-1;
	cac[index].time=0;	//we have to set new time as well
	cac[index].flag=-1;	//this frame is now empty

	return cac[index].byte_data;
}*/

int check_cache(CartridgeIndex cart, CartridgeIndex blk){

	int i=0;

	for(i=0;i<total_frames;i++){

		if(cac[i].cart==cart && cac[i].frame==blk){

			return 0;
		}

		else if (cac[i].cart==-2 && cac[i].frame==-2){

			return -1;
		}
	}

	if(i==total_frames){

		return -1;
	}

	return 0;
}

//
// Unit test

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cartCacheUnitTest
// Description  : Run a UNIT test checking the cache implementation
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int cartCacheUnitTest(void){
	int init_cache_size=DEFAULT_CART_FRAME_CACHE_SIZE;
	set_cart_cache_size(init_cache_size);

	int fixed_iterations=DEFAULT_CART_FRAME_CACHE_SIZE;

	for(int i=0;i<fixed_iterations;i++){
		int cart=-1;
		int frame=-1;
		int check=-2;
		char letter[1024];

		int operation=(int)rand();

		if(operation%100<50){			//this is read to the cache 
			cart=((int) rand())%64;
			frame=((int) rand())%1024;
			check=check_cache(cart,frame);

			if(check==-1){
				logMessage(LOG_OUTPUT_LEVEL, "The frame was not found in the cache.");
			}

			else{

				memcpy(letter,get_cart_cache(cart,frame),1024);

				if(letter[0]!='a'){

					logMessage(LOG_OUTPUT_LEVEL, "Cache unit test was not completed.");
					return -1;
				
				}

				else{

					logMessage(LOG_OUTPUT_LEVEL, "The frame is present in the cache.");
				}
			}
		}

		else{		//this is write to the frame

			cart=((int) rand())%64;
			frame=((int) rand())%1024;
			check=check_cache(cart,frame);
			if(check==-1){	//if X is not marked in the cache
				check=put_cart_cache(cart,frame,"a");
				if(check==0){

					logMessage(LOG_OUTPUT_LEVEL, "Putting in cache was succesfull.");
				}

				else{

					logMessage(LOG_OUTPUT_LEVEL, "Failure in putting the frame to the cache.");
				}
			}
		}

	}

	close_cart_cache();
	// Return successfully
	logMessage(LOG_OUTPUT_LEVEL, "Cache unit test completed successfully.");
	return(0);
}

#ifndef ORDER_H
#define ORDER_H

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "customer.h"

//an order object (created by producer and not yet processed)
struct info_t{
     int customer_id; //for customer funds
     char *book_name; //for price of book
     float bookprice;
};
typedef struct info_t *orderInfoPtr;

//an orderbuffer struct, each consumer will have access to its own
//each one keeps track of its count, and conditional variables, so 
//each consumer "thinks" it has exclusive communication with the producer
//prevents deadlocks by keeping sharing to a minimum and locking to a minimum
struct orders_struct{
	pthread_t tid; //contains the thread id of the consumer that owns this buffer
	orderInfoPtr *buf;
	int size; //should equal MAXBUFSIZE
	int count; //shound not exceed MAXBUFSIZE
	int front;
	int rear;
	pthread_mutex_t mutex;
    pthread_cond_t dataAvailable;
    pthread_cond_t spaceAvailable; 
};
typedef struct orders_struct *orderBufferPtr;

//sale objected (created by consumer after order was processed)
//can be added to acceptedSales or rejectedSales sorted lists depending on customer funds
struct sale_struct{
	int customer_id;
	char *booktitle;
	float bookprice;
	float remaining_balance;
};
typedef struct sale_struct * sale_reportPtr;

//allocates a customer pointer (located in customer.h)
customerPtr createCustomer(char *name, char *add, char *zip, char *state, float bal);

//free memory when done
void killCustomer(customerPtr tempcust);

//producer creates orders with this function
orderInfoPtr init_newOrder(int, char*, float);

//hashmap links categories to buffers initialized by this function
void init_order_buf(orderBufferPtr ob, int buf_size);

//clean memory when done with buffer
void kill_order_buf(orderBufferPtr ob);

//cleans individual orders when we are done with it
void free_order(orderInfoPtr order);

//created by the consumer, added to the sortedlist after order has been processed
sale_reportPtr createNewSale(int c_id, char * b_title, float b_price, float rembal);

//comparator function used for sorted list
int compareSales(void* rep1, void* rep2);

//destroyer function used for sorted list
void destroySales(void* rep);

#endif
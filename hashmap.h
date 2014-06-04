#ifndef HASHMAP_H
#define HASHMAP_H

//credit for the hashmap backend is given to UTHash from: http://troydhanson.github.io/uthash/
#include "uthash.h"
#include "customer.h"
#include <pthread.h>
#include "order.h"


//holds the key and a LinkedList of Indexes, from index.c
//UT_hash_handle is what uthash uses to create our hashmap
struct bufferHash{
	char * category_key;
	orderBufferPtr buffer_value;
	UT_hash_handle hh;
};
typedef struct bufferHash * bufferHashPtr;

struct customerHash{
	int customer_key;
	customerPtr customer_info;
	UT_hash_handle hh;
};
typedef struct customerHash * customerHashPtr;

/*
 * Used to clear the hash and free all memory when caller is done with it
 */
void clearCustomerHash(customerHashPtr *);

// Frees allocated memory from the buffer hash
void clearBufferHash(bufferHashPtr *);

// Adds customer to the customer hash table
// Checks if customer is already in the hash
int addCustomer(int, customerPtr, customerHashPtr *);

//returns the customer info or null if customer doesnt exist
customerPtr getCustomer(int, customerHashPtr *);

//adds an initialized buffer with they key category
int addBuffer(char *, orderBufferPtr, bufferHashPtr *);

//returns a pointer to the buffer given a category key
orderBufferPtr getBuffer(char *, bufferHashPtr *);

//comparator function to sort the hash by customerid, used to print the final report
//in case customerid's were not given in order
int sort_customersByID(customerHashPtr a, customerHashPtr b);

void sortHash(customerHashPtr *hash_t, int(*compare)(customerHashPtr, customerHashPtr));

//debugging functions
void printCustomerTable(customerHashPtr *hash_t);

void printBufferTable(bufferHashPtr *hash_t);





#endif

#include "hashmap.h"

void clearCustomerHash(customerHashPtr *cust_hash){
	if(*cust_hash == NULL)
		return;
	else{
		customerHashPtr currentHash, tempHash;

		HASH_ITER(hh, *cust_hash, currentHash, tempHash){
			HASH_DEL(*cust_hash, currentHash);
			killCustomer(currentHash->customer_info);
			//free(currentHash);
		}
	}
}

void clearBufferHash(bufferHashPtr *buff_hash){
	if(*buff_hash == NULL)
		return;
	else{
		bufferHashPtr currentHash, tempHash;

		HASH_ITER(hh, *buff_hash, currentHash, tempHash){
			HASH_DEL(*buff_hash, currentHash);
			kill_order_buf(currentHash->buffer_value);
			free(currentHash->category_key);
			//free(currentHash);
		}
	}
}

// adds buffer to hash with bufferptr and category name
int addBuffer(char * category, orderBufferPtr order_buffer, bufferHashPtr *buffer_hash){
	bufferHashPtr keyExists;

	//check for uniqueness of key
	HASH_FIND_STR(*buffer_hash, category, keyExists);

	if(!keyExists){
		keyExists = (bufferHashPtr) malloc(sizeof(struct bufferHash));
		keyExists->category_key = category;
		keyExists->buffer_value = order_buffer;
		HASH_ADD_KEYPTR(hh, *buffer_hash, keyExists->category_key, strlen(keyExists->category_key), keyExists);
		return 0;
	} //does not add buffer if category exists
	else return 1;
}

// adds customer to the customer hash table
int addCustomer(int customerID, customerPtr newCustomer, customerHashPtr *customerHash){

	customerHashPtr keyExists;

	//check uniqueness of key
	HASH_FIND_INT(*customerHash, &customerID, keyExists);

	if(!keyExists){
		keyExists = (customerHashPtr) malloc(sizeof(struct customerHash));
		keyExists->customer_key = customerID;
		keyExists->customer_info = newCustomer;
		HASH_ADD_INT(*customerHash, customer_key, keyExists);
		return 0;
	}
	else return 1;
}

orderBufferPtr getBuffer(char *category, bufferHashPtr *hash_t){
	bufferHashPtr keyExists;

	HASH_FIND_STR(*hash_t, category, keyExists);

	if(keyExists){
		return keyExists->buffer_value;
	}
	else
		return NULL;
}

customerPtr getCustomer(int customerID, customerHashPtr *customer_hash){
	customerHashPtr keyExists;

	HASH_FIND_INT(*customer_hash, &customerID, keyExists);

	if(keyExists){
		return keyExists->customer_info;
	}
	else return NULL;

}

int sort_customersByID(customerHashPtr a, customerHashPtr b) {
	return a->customer_key < b->customer_key ? -1 : (a->customer_key > b->customer_key ? 1 : 0);
}

void sortHash(customerHashPtr *hash_t, int(*compare)(customerHashPtr, customerHashPtr)){
	HASH_SORT(*hash_t, compare);
}

/**
 * Debugging functions
 */

void printCustomerTable(customerHashPtr *hash_t){
	customerHashPtr printer;

	for(printer = *hash_t; printer!=NULL; printer=(customerHashPtr)(printer->hh.next)){
		printf("Customer ID: %d\n", printer->customer_key);
		printf("\tName: %s\n", printer->customer_info->name);
		printf("\tAddress: %s\n", printer->customer_info->address);
		printf("\tState: %s\n", printer->customer_info->state);
		printf("\tZip: %s\n", printer->customer_info->zip);
		printf("\tBalance: %.2f\n", printer->customer_info->balance);
	}
}

void printBufferTable(bufferHashPtr *hash_t){
	bufferHashPtr printer;
	char *category;
	orderBufferPtr buffer;
	
	int i;
	for(printer = *hash_t; printer!=NULL; printer=(bufferHashPtr)(printer->hh.next)){
		category = printer->category_key;
		buffer = printer->buffer_value;
		printf("Category: %s\n", category);
		for(i=0; (buffer->buf[i])!=NULL; i++){
			printf("\tCustomerID: %d\n", buffer->buf[i]->customer_id);
			printf("\tBookname: %s\n", buffer->buf[i]->book_name);
			printf("\tBookprice: %f\n", buffer->buf[i]->bookprice);
		}
	}	
}

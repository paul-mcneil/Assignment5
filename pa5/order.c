#include "order.h"

// creates a new customer object
customerPtr createCustomer(char *name, char *add, char *zip, char *state, float bal){
	customerPtr customerToAdd = (customerPtr) malloc(sizeof(struct CustomerStruct));

	customerToAdd->name = name;
	customerToAdd->address = add;
	customerToAdd->state = state;
	customerToAdd->zip = zip;
	customerToAdd->balance = bal;

	return customerToAdd;
}

void killCustomer(customerPtr tempcust){
	if(tempcust == NULL)
		return;
	else{
		free(tempcust->name);
		free(tempcust->address);
		free(tempcust->state);
		free(tempcust->zip);
		free(tempcust);
	}
}

// given customer id, book, book price
// return pointer to new order
orderInfoPtr init_newOrder(int customer_ID, char* book_title, float book_price){
	orderInfoPtr newOrder = (orderInfoPtr) malloc(sizeof(struct info_t));

	newOrder->customer_id = customer_ID;
	newOrder->book_name = book_title;
	newOrder->bookprice = book_price;

	return newOrder;
}

//initializes a buffer of orders
void init_order_buf(orderBufferPtr ob, int buf_size){
    ob->buf = calloc(buf_size, sizeof(struct info_t));
    ob->size = buf_size;
    ob->count = 0;
    ob->front = ob->rear = 0;
    pthread_mutex_init(&ob->mutex, 0);
    pthread_cond_init(&ob->dataAvailable, 0);
    pthread_cond_init(&ob->spaceAvailable, 0);
}

void kill_order_buf(orderBufferPtr ob){
	if(ob == NULL)
		return;
	else{
		pthread_mutex_destroy(&ob->mutex);
		pthread_cond_destroy(&ob->dataAvailable);
		pthread_cond_destroy(&ob->spaceAvailable);
		free(ob->buf);
		free(ob);
	}
}

void free_order(orderInfoPtr order){
	if(order == NULL)
		return;
	else{
		free(order->book_name);
		free(order);
	}
}

sale_reportPtr createNewSale(int c_id, char * b_title, float b_price, float rembal){
	sale_reportPtr report = (sale_reportPtr) malloc(sizeof(struct sale_struct));

	report->customer_id = c_id;
	report->booktitle = b_title;
	report->bookprice = b_price;
	report->remaining_balance = rembal;

	return report;
}

int compareSales(void* r1, void* r2){
	sale_reportPtr rep1 = (sale_reportPtr) r1;
	sale_reportPtr rep2 = (sale_reportPtr) r2;

	return (rep1->customer_id > rep2->customer_id) ? 1 : ((rep1->customer_id < rep2->customer_id) ? -1 : (rep1->remaining_balance > rep2->remaining_balance ? -1: (rep1->remaining_balance < rep2->remaining_balance ? 1 : 0)));
}

void destroySales(void* rep){
	sale_reportPtr temprep = (sale_reportPtr) rep;
	if(temprep == NULL)
		return;
	else{
		//booktitles are freed by the cleanup function
		free(rep);
	}
}

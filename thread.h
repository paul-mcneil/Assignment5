#ifndef THREAD_H
#define THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "hashmap.h"
#include "order.h"
#include "tokenizer.h"
#include "sorted-list.h"

#define MAXBUFSIZE 10
#define MAX_LINE_LEN 200 //change max line length if you think it can be longer

//sets up the environment so that the producer and consumers can process the orders
//initializes a database of customer info from database.txt
//initializes an empty buffer for each category from categories.txt
void setup(char *dbFile, char *categoriesFile);

// Writes a report to finalreport.txt
// Consists of a final summary for each customer
// List of successful orders, rejected orders and total remaining balance
void writeReport(const char *filename, SortedListPtr accepted, SortedListPtr rejected);

// free's allocated memory from buffer hash, customer hash and sorted lists
void cleanup();

// **** PRODUCER ****
// opens the orders file and adds sales to the appropriate category buffer
// shouts to consumer that a new order is available
// releases all threads waiting upon its exit
void *addNewOrder(void *args);

// **** CONSUMER(S) ****
// Continuously checks the category buffer for available orders
// If the producer is finished reading the orders file and there is no more orders in the buffer, exit
// Shouts to main whenever a consumer exits, main waits for all consumers to finish
void *processOrder(void *args);

// Print report lists for debugging
void SLPrint(SortedListPtr sl);

// Trims extra spaces, quotes, newlines etc..
void trimExtras(char *text);

#endif

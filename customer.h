#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//template for a customer item

struct CustomerStruct{
	char * name;
	char * address;
	char * state;
	char * zip;
	float balance;
};
typedef struct CustomerStruct * customerPtr;

#endif
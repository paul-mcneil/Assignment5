#include "thread.h"

/**
 * GLOBAL VARIABLES: Shared accross all threads
 *
 * lockAcceptedList: locks the global list of accepted sales while a thread is updating it
 *
 * lockRejectedList: locks the global list of rejected sales while a thread is updating it
 *
 * lockConsumerDB: locks the customer database when updating customers funds
 * 
 * numFinishedConsumers: allows each consumer to work individually
 * without hogging the resources, while still avoiding a race. Main thread
 * waits until all consumers are done before printing out a report of the sales
 *
 * producerFinished: tells the consumers when the producer has finished
 * reading the file so that when the buffer is empty they know they are finisihed
 *
 * acceptedSales: list of accepted sales sorted by customer id
 *
 * rejectedSales: list of accepted sales sorted by customer id
 *
 * customerHash_t: a global database of all the customers listed in database.txt
 * 
 * buffHash_t: a global hash of the categories as the key and the address to their buffer as the value
 */

pthread_mutex_t lockAcceptedList = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockRejectedList = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t lockConsumerDB = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t lockConsumerCount = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t consumerCountCond = PTHREAD_COND_INITIALIZER;
int numFinishedConsumers;
int numCategories;

pthread_mutex_t lockProducerFlag = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t producerEndCond = PTHREAD_COND_INITIALIZER;
int producerFinished;

SortedListPtr acceptedSales;
SortedListPtr rejectedSales;

customerHashPtr customerHash_t;
bufferHashPtr buffHash_t;

/**
 * End of global variables
 */

long getFileSize(FILE *fp){
    long fileSize;
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);

    return fileSize;
}

int main(int  argc, char **argv){ 
    if(argc != 4){
        printf("Illegal number of args.\n");
        printf("Exiting.\n");
        exit(1);
    }
    else{
        char *db_file = argv[1];
        char *order_file = argv[2];
        char *categ_file = argv[3];
        pthread_t producer_tid;
        const char *reportFileName = "finalreport.txt";

        setup(db_file, categ_file);

        //create producer to read file
        pthread_create(&producer_tid, NULL, addNewOrder, order_file);

        //create consumers, send them the location of the buffers
        bufferHashPtr traverse;
        for(traverse = buffHash_t; traverse!=NULL; traverse=(bufferHashPtr)(traverse->hh.next)){
            pthread_create(&traverse->buffer_value->tid, NULL, processOrder, traverse->buffer_value);
        }

        //Wait for all the consumers to process all their orders
        //Avoids race condition
        pthread_mutex_lock(&lockConsumerCount);
        while(numFinishedConsumers != numCategories){
            pthread_cond_wait(&consumerCountCond, &lockConsumerCount);
        }
        pthread_mutex_unlock(&lockConsumerCount);

        //once all consumers are done we can write the sale report
        writeReport(reportFileName, acceptedSales, rejectedSales);

        cleanup();      
    }

    return 0;
}

void setup(char *dbFile, char *categoriesFile){
    // parse files
    FILE *db_fp; // database.txt
    FILE *categories_fp; // categories.txt
    long fileSize;
    TokenizerT *tk;

    //set up global variables
    numFinishedConsumers = 0;
    producerFinished = 0;
    numCategories = 0;

    //setup global sales report lists
    acceptedSales = SLCreate(compareSales, destroySales);
    rejectedSales = SLCreate(compareSales, destroySales);

    //setup global hashes
    customerHash_t = NULL;
    buffHash_t = NULL;

    //setup customer database
    if((db_fp = fopen(dbFile, "r")) == NULL){
        perror("Error trying to open database file");
        printf("Program terminated.\n");
        cleanup();
        exit(1);
    }
    else{
        fileSize = getFileSize(db_fp);
        char buffer[fileSize];
        char *name, *address, *state, *zip;
        int customer_id;
        float customer_funds;
        customerPtr newCustomer;

        //must have customers in order to process the orders
        if(fileSize == 0){
            printf("Customer file given was empty, please input another file.\n");
            printf("Program Exiting\n");
            cleanup();
            exit(1);
        }

        while(fgets(buffer, fileSize, db_fp) != NULL){
            tk = TKCreate("|", buffer);

            while((name = TKGetNextToken(tk)) != NULL){
                customer_id = atoi(TKGetNextToken(tk));
                customer_funds = atof(TKGetNextToken(tk));
                address = TKGetNextToken(tk);
                state = TKGetNextToken(tk);
                zip = TKGetNextToken(tk);

                trimExtras(name);
                trimExtras(address);
                trimExtras(state);
                trimExtras(zip);

                newCustomer = (customerPtr) malloc(sizeof(struct CustomerStruct));
                newCustomer->name = name;
                newCustomer->address = address;
                newCustomer->state = state;
                newCustomer->zip = zip;
                newCustomer->balance = customer_funds;

                addCustomer(customer_id, newCustomer, &customerHash_t);
            }
        }
        fclose(db_fp); //customer db is created so safe to close customer file
    }

    //setup queues for each category
    if((categories_fp = fopen(categoriesFile, "r")) == NULL){
        perror("Error trying to open categories file");
        printf("Program terminated\n");
        cleanup();
        exit(1);
    }else{
        fileSize = getFileSize(categories_fp);
        char line[fileSize];
        char *category;
        orderBufferPtr ob_buff;
        int bufSize = MAXBUFSIZE;

        //if we are unable to create at least one consumer, we cant process orders
        if(fileSize == 0){
            printf("Categories file given was empty, please input another file.\n");
            printf("Program Exiting\n");
            cleanup();
            exit(1);
        }

        while((fgets(line, fileSize, categories_fp)) != NULL){
            category = malloc(strlen(line)+1);
            strcpy(category, line);

            trimExtras(category);

            //initialize a new buffer for each category and store it in a table such that:
            //KEY: category to VALUE: pointer to buffer
            ob_buff = (orderBufferPtr) malloc(sizeof(struct orders_struct));
            init_order_buf(ob_buff, bufSize);
            addBuffer(category, ob_buff, &buffHash_t);
            //increment the global variable numCategories
            numCategories++;
        }
        fclose(categories_fp); //category buffers initialized so safe to close category file
    }
}

void writeReport(const char *filename, SortedListPtr accepted, SortedListPtr rejected){
    FILE *ofp;
    if((ofp = fopen(filename, "w")) == NULL){
        perror("Error opening output file");
        cleanup();
        exit(1);
    } 
    else{
        sale_reportPtr temp_aSale, temp_rSale; //pointers to the accepted sales and rejected sales
        customerHashPtr printer;
        int cid;

        //since we aren't guaranteed a specific order of customers
        sortHash(&customerHash_t, sort_customersByID);

        for(printer = customerHash_t; printer!=NULL; printer=(customerHashPtr)(printer->hh.next)){
            cid = printer->customer_key;
            fprintf(ofp, "=== BEGIN CUSTOMER INFO ===\n");
            fprintf(ofp, "### BALANCE ###\n");
            fprintf(ofp, "Customer name: %s\n", printer->customer_info->name);
            fprintf(ofp, "Customer ID number: %d\n", cid);
            fprintf(ofp, "Remaining credit balance after all purchases (a dollar amount): %.2f\n", printer->customer_info->balance);
            fprintf(ofp, "### SUCCESSFUL ORDERS ###\n");

            while(accepted->head != NULL){
                temp_aSale = (sale_reportPtr) accepted->head->data;

                if(cid != temp_aSale->customer_id)
                    break;
                else{
                    fprintf(ofp, "\"%s\"|%.2f|%.2f\n", temp_aSale->booktitle, temp_aSale->bookprice, temp_aSale->remaining_balance);
                    accepted->head = accepted->head->next;
                }
            }

            fprintf(ofp, "### REJECTED ORDERS ###\n");

            while(rejected->head != NULL){
                temp_rSale = (sale_reportPtr) rejected->head->data;

                if(cid != temp_rSale->customer_id)
                    break;
                else{
                    fprintf(ofp, "\"%s\"|%.2f\n", temp_rSale->booktitle, temp_rSale->bookprice);
                    rejected->head = rejected->head->next;
                }
            }
            fprintf(ofp, "=== END CUSTOMER INFO ===\n");
            fprintf(ofp, "\n");
        }

        //report file done
        fclose(ofp);
    }
} 

//PRODUCER
void *addNewOrder(void *args){
    char *order_file = (char *) args;
    FILE *order_fp;

    //open order.txt file
    if((order_fp = fopen(order_file, "r")) == NULL){
        perror("Error trying to open orders file");
        printf("Program terminated\n");
        cleanup();
        exit(1);
    }
    else{
        char buffer[MAX_LINE_LEN];
        TokenizerT *tk;
        char *booktitle, *category;
        float bookprice;
        int customer_id, index;
        orderBufferPtr orderBuffer;
        orderInfoPtr oinf;

        //if file of orders is empty there is nothing to do
        if(getFileSize(order_fp) == 0){
            printf("Orders file given was empty, please input another file.\n");
            printf("Program Exiting\n");
            cleanup();
            exit(1);
        }

        //we don't want this thread to slow the other threads down
        //and we would also like to free resources after program is done
        pthread_detach(pthread_self()); 

        while(fgets(buffer, MAX_LINE_LEN, order_fp) != NULL){
            printf("Producer is adding a new sale.\n");
            tk = TKCreate("|", buffer);
            while((booktitle = TKGetNextToken(tk)) != NULL){
                bookprice = atof(TKGetNextToken(tk));
                customer_id = atoi(TKGetNextToken(tk));
                category = TKGetNextToken(tk);

                trimExtras(booktitle);
                trimExtras(category);

                //get the buffer of that category
                orderBuffer = getBuffer(category, &buffHash_t);

                //checks invalid category
                if(orderBuffer == NULL)
                    continue;

                //lock the buffer
                pthread_mutex_lock(&orderBuffer->mutex);

                //while the buffer is full call the consumer of that buffer
                while(orderBuffer->count == orderBuffer->size){
                    pthread_cond_signal(&orderBuffer->dataAvailable);
                    printf("Producer waiting for consumer\n");
                    pthread_cond_wait(&orderBuffer->spaceAvailable, &orderBuffer->mutex);
                }
 
                index = (orderBuffer->rear++)%(orderBuffer->size);
                //initialize new order and add to buffer
                oinf = init_newOrder(customer_id, booktitle, bookprice);
                orderBuffer->buf[index] = oinf;
                orderBuffer->count++;

                pthread_cond_signal(&orderBuffer->dataAvailable);
                pthread_mutex_unlock(&orderBuffer->mutex);
                //unlock mutex
            }
        }

    //at this point producer reached the end of the orders text file
    //so producer can close the order file and safely exit
    fclose(order_fp);

    //warn consumers that producer has finished reading order file
    pthread_mutex_lock(&lockProducerFlag);
    producerFinished = 1;
    pthread_mutex_unlock(&lockProducerFlag);
    }

    pthread_mutex_lock(&lockConsumerCount);
    int consCountCopy = numFinishedConsumers;
    pthread_mutex_unlock(&lockConsumerCount);
    bufferHashPtr temp;

    //release all threads waiting for the producer
    while(consCountCopy < numCategories){      
        for(temp = buffHash_t; temp!=NULL; temp=(bufferHashPtr)(temp->hh.next)){
            pthread_cond_signal(&temp->buffer_value->dataAvailable);
        }
        pthread_mutex_lock(&lockConsumerCount);
        consCountCopy = numFinishedConsumers;
        pthread_mutex_unlock(&lockConsumerCount);
    }

    printf("Producer exiting\n");
    pthread_exit(NULL);
}
// Used to check to see if the producer is finished reading database file
// returns 1 if the producer is finished and closed the file
// function used so that mutex isn't hogged throughout the entire iteration of the loop
int checkProducerFlag(){
    int flagcopy;
    pthread_mutex_lock(&lockProducerFlag);
    flagcopy = producerFinished;
    pthread_mutex_unlock(&lockProducerFlag);
    return flagcopy;
}

//CONSUMER(S)
void *processOrder(void *args){
    orderBufferPtr orders = (orderBufferPtr) args;
    orderInfoPtr *oinfbuf = orders->buf;
    
    orderInfoPtr item;
    int customer_id, index;
    char *booktitle; //bookname
    float bookprice;
    customerPtr c_info;
    sale_reportPtr report;

    //we don't want this thread to slow the other threads down
    //and we would also like to free resources after program is done
    pthread_detach(pthread_self()); 

    //while producer is not done reading the file
    while(!checkProducerFlag()){
        pthread_mutex_lock(&orders->mutex);
        //if buffer is empty call on producer
        while(orders->count == 0){
            pthread_cond_signal(&orders->spaceAvailable);
            printf("Consumer (%x) waiting for producer\n", (unsigned int) pthread_self());
            //if producer was done and the buffer is empty, we can leave
            if(checkProducerFlag()){
                pthread_mutex_unlock(&orders->mutex);
                pthread_mutex_lock(&lockConsumerCount);
                numFinishedConsumers++;
                pthread_cond_signal(&consumerCountCond);
                pthread_mutex_unlock(&lockConsumerCount);
                printf("Consumer (%x) exiting\n", (unsigned int) pthread_self());
                pthread_exit(NULL);
            }
            pthread_cond_wait(&orders->dataAvailable, &orders->mutex);
        }

        //process all items in this buffer
        while(orders->count > 0){
            printf("Consumer (%x) is processing a sale\n", (unsigned int) pthread_self());
            index = (orders->front++)%(orders->size);
            orders->count--;
            item = oinfbuf[index];
            customer_id = item->customer_id;
            booktitle = item->book_name;
            bookprice = item->bookprice;

            //update customer's funds
            pthread_mutex_lock(&lockConsumerDB);
            c_info = getCustomer(customer_id, &customerHash_t);

            //process order only if customer exists
            if(c_info != NULL && (c_info->balance - bookprice) >= 0){
                //deduct from his balance and add to acceptedOrders list
                c_info->balance -= bookprice;
                pthread_mutex_lock(&lockAcceptedList);
                report = createNewSale(customer_id, booktitle, bookprice, c_info->balance);
                SLInsert(acceptedSales, report);
                pthread_mutex_unlock(&lockAcceptedList);
            }
            else if(c_info != NULL && (c_info->balance - bookprice) < 0){
                //add to rejectedOrders
                pthread_mutex_lock(&lockRejectedList);
                report = createNewSale(customer_id, booktitle, bookprice, c_info->balance);
                SLInsert(rejectedSales, report);
                pthread_mutex_unlock(&lockRejectedList);
            }
            else if(c_info == NULL){
                printf("CustomerID %d was not found in the database\n", customer_id);
            }
            pthread_mutex_unlock(&lockConsumerDB);

            pthread_cond_signal(&orders->spaceAvailable);
            pthread_mutex_unlock(&orders->mutex);
        }
    }

    //incase flag was updated before the list was empty
    pthread_mutex_lock(&lockConsumerCount);
    numFinishedConsumers++;
    pthread_mutex_unlock(&lockConsumerCount);
    pthread_cond_signal(&consumerCountCond);
    
    printf("Consumer (%x) exiting\n", (unsigned int) pthread_self());
    pthread_exit(NULL);
}

/*
*Helper functions
*/
//Deletes the spaces, quotes, newlines etc..
void trimExtras(char *text){
    int length = strlen(text);
    
    char *tempchar = (char *) malloc(length+1);
    strcpy(tempchar, text);

    //trim the ends
    while(tempchar[length] == '\0' || tempchar[length] == '\n' || tempchar[length] == '\"' || tempchar[length] == ' '){
        length--;
    }
    tempchar[length+1] = '\0';

    //trim front
    int i;
    for(i=0; tempchar[i] == '\0' || tempchar[i] == '\n' || tempchar[i] == '\"' || tempchar[i] == ' '; i++);

    strcpy(text, &tempchar[i]);
    free(tempchar);
}

//free allocated memory upon exit
void cleanup(){
    clearCustomerHash(&customerHash_t);
    clearBufferHash(&buffHash_t);
    SLDestroy(acceptedSales);
    SLDestroy(rejectedSales);
}

//DEBUGGIN FUNCTIONS
void SLPrint(SortedListPtr sl){
    SortedListIteratorPtr iter = SLCreateIterator(sl);
    sale_reportPtr data;
    
    while((data = ((sale_reportPtr) SLNextItem(iter))) != NULL){
        printf("Customer ID: %d, Booktitle: %s, Remaining: %4.2f\n", data->customer_id, data->booktitle, data->remaining_balance);
    }
}


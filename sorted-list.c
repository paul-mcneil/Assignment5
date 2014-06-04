/*
*sorted-list.c
*
*Big-O analysis:
*
*Insert: O(n)
*Remove: O(n)
*Destroy List: O(n)
*All others: O(1)
*Generating a sorted list from a list of data: O(n^2)
*/

/**
 * ALTERTED IN ORDER TO ACCEPT DUPLICATES, SINCE ONE CUSTOMER IS ABLE TO MAKE MULTIPLE ORDERS
 */

#include "sorted-list.h"

SortedListPtr SLCreate(CompareFuncT cf, DestructFuncT df){
	if(cf == NULL || df == NULL){
		printf("Missing one or both of the functions required to create a sorted list.\n");
		return NULL;
	}
	SortedListPtr sl = (SortedListPtr) malloc(sizeof(struct SortedList));
	sl->head = NULL;
	sl->comparator = cf;
	sl->destroyer = df;

	return sl;
}

void SLDestroy(SortedListPtr list){
	if(list == NULL)
		return; // dont do anything, nothing to destroy
	else{
		NodePtr deleter;

		while(list->head != NULL){ //delete all dynamically allocated nodes
			deleter = list->head;
			list->head = list->head->next;
			list->destroyer(deleter->data);
			free(deleter);
		}

		//functions are not dynamically allocated, they are created in main, so do not need to be free'd
		free(list); //then free the structure
	}
}

//insert does not avoid duplicates, altered for threaded program
int SLInsert(SortedListPtr list, void *newObj){
	if(list == NULL){
		printf("List was not allocated properly\n");
		return 0;
	}
	else{
		//allocate a temporary node, which will be inserted into the list if its valid, free'd if its a duplicate
		//each parameter is explained in the header file
		NodePtr tempNode = (NodePtr) malloc(sizeof(struct Node));
		tempNode->data = newObj;
		tempNode->next = NULL;
		tempNode->numPointers = 0;
		tempNode->isValid = true;

		if(list->head == NULL){ //if this is the first item to be added to the list
			list->head = tempNode; //simply make it the head of the list
		}
		else if(list->head->next == NULL){ //there is only 1 item in the list
			if(list->comparator(list->head->data, newObj) < 0){ //new object is smaller (it becomes second on list)
				list->head->next = tempNode;
				return 1;
			}
			else{ //new object is larger, it becomes the head
				tempNode->next = list->head;
				list->head = tempNode;
				return 1;
			}
		}
		else{ //there are at least 2 items in the list
			if(list->comparator(list->head->data, newObj) > 0){ //if object is larger than head it becomes head
				tempNode->next = list->head;
				list->head = tempNode;
				return 1;
			}

			NodePtr traverse = list->head->next;
			NodePtr lagging = list->head;
			while(traverse != NULL){
				if(list->comparator(traverse->data, newObj) < 0){ //new object is smaller (it goes towards end of list)
					lagging = traverse;
					traverse = traverse->next;
				}
				else{ //new object is larger (add it to this spot)
					tempNode->next = traverse;
					lagging->next = tempNode;
					return 1;
				}
			}
			if(list->comparator(lagging->data, newObj) < 0){ //tests one more time, if smaller than last item, adds to the end of the list
				lagging->next = tempNode;
				return 1;
			}
		}
	}
	return 1; //makes compiler happy
}

//also acts as a sort of garbage collection, when looking for newObj, it looks for invalid objects with no pointers to free the node
int SLRemove(SortedListPtr list, void *newObj){
	//base cases, avoids segfault
	if(list == NULL){
		printf("List was not allocated properly\n");
		return 0;
	}
	else if(list->head == NULL){
		printf("List is empty\n");
		return 0;
	}
	
	//checks against head
	else if(list->comparator(list->head->data, newObj) == 0){
		if(list->head->numPointers == 0){
			NodePtr tempNode = list->head; //must save node to be freed
			list->head = list->head->next; //update head
			list->destroyer(tempNode->data);
			free(tempNode);
			return 1;
		}
		else{
			if(!list->head->isValid){ //if item was already removed but still has a pointer
				printf("Item was not found\n");
				return 0;
			}

			list->head->isValid = false;
			return 1;
		}
	}
	else
	{
		NodePtr lagging = list->head;
		NodePtr traverse = list->head->next;

		while(traverse != NULL)
		{	
			//garbage collection !!TEST GARBAGE COLLECTION MAKE SURE IT WORKS PROPERLY
			if(!(traverse->isValid) && (traverse->numPointers == 0)){
				lagging->next = traverse->next;
				list->destroyer(traverse->data);
				free(traverse);
				traverse = lagging->next;
			}

			if(list->comparator(traverse->data, newObj) == 0)
			{
				if(traverse->numPointers == 0){
					lagging->next = traverse->next;
					list->destroyer(traverse->data);
					free(traverse);
					return 1;
				}
				else{
					if(!traverse->isValid){ //if item was already removed but still has a pointer
						printf("Item was not found\n");
						return 0;
					}

					traverse->isValid = false;
					return 1;
				}	
			}

			lagging = traverse;
			traverse = traverse->next;
			
		}
	}
	//garbage collection for first item, done last so parsing isn't affected !!TEST THIS CASE AS WELL
	if(!(list->head->isValid) && (list->head->numPointers == 0)){
		NodePtr oldHead = list->head; //keep track of the old head to be free'd
		list->head = list->head->next;
		list->destroyer(oldHead->data);
		free(oldHead);
	}

	printf("Item was not found\n");
	return 0;
}

SortedListIteratorPtr SLCreateIterator(SortedListPtr list){
	if(list == NULL || list->head == NULL){ //list must be initialized and have at least one item
		printf("List was either not allocated, or is empty, cannot create a pointer to it\n");
		return NULL;
	}

	//create a space in memory for the the iterator make it point to head
	SortedListIteratorPtr iterator = (SortedListIteratorPtr) malloc(sizeof(struct SortedListIterator));
	iterator->currentNode = list->head;
	list->head->numPointers++; //increment numofpointers pointing to head

	return iterator;
}

void SLDestroyIterator(SortedListIteratorPtr iter){
	if(iter != NULL){
		if(iter->currentNode != NULL)
			iter->currentNode->numPointers--;
		free(iter); //only iter needs to be freed, not anything it points to
	}
}

void *SLNextItem(SortedListIteratorPtr iter){ //make sure all those cases are taken care of
	if(iter == NULL){
		printf("Iterator was not allocated properly\n");
		return NULL;
	}
	else if(iter->currentNode == NULL)
		return NULL; //check for null will terminate a loop in case it is used
	else{
		if(iter->currentNode->isValid){ //node was not removed while this pointer pointed to it
			void * dataReturned = malloc(sizeof(iter->currentNode->data));
			dataReturned = iter->currentNode->data; //save the data the iterator is pointing to
			iter->currentNode->numPointers--; //decrement curent node's # of pointers
			iter->currentNode = iter->currentNode->next; //update the iterator
			if(iter->currentNode != NULL) 
				iter->currentNode->numPointers++; //now that the iterator was incremented we can update the number of iterators pointing to the new node (assuming it is not outside of the list)
			return dataReturned;
		}
		else{ //if node was removed while this pointer pointed to it
			iter->currentNode->numPointers--;
			//remove function acts as a garbage collection and will take care of this node if there are no pointers pointing to it

			iter->currentNode = iter->currentNode->next;
			if(iter->currentNode != NULL) 
				iter->currentNode->numPointers++;

			return SLNextItem(iter); //does the same with the next token
		}
	}
}

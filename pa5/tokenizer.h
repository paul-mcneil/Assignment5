#ifndef DELIMTOKENIZER_H
#define DELIMTOKENIZER_H

/*
 * Header file for files that need to use the tokenizer.c functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TokenizerT_ {
	char * delims;
	char * text;
};

typedef struct TokenizerT_ TokenizerT;

TokenizerT *TKCreate(char *, char *);
void TKDestroy(TokenizerT *);
char *TKGetNextToken(TokenizerT *);

#endif
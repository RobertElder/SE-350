
#ifndef _UTILS_H_
#define _UTILS_H_

#include "uart.h"

void assert(int, unsigned char *);
 
unsigned int pow(unsigned int, unsigned int);

unsigned int get_random(void);

void printDigit(unsigned int);
void print_unsigned_integer(unsigned int);
void print_signed_integer(signed int);
unsigned int string_len(char * c);

int myatoi(const char *string);

#endif


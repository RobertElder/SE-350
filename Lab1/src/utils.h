
#ifndef _UTILS_H_
#define _UTILS_H_

#include "uart.h"

#define PI_CONSTANT 3.14159265

void assert(int, unsigned char *);
 
unsigned int pow(unsigned int, unsigned int);

unsigned int get_random(void);

void printDigit(unsigned int);
void print_printable_character(unsigned char);
void print_unsigned_integer(unsigned int);
void print_hex_byte(unsigned int);
void print_signed_integer(signed int);
unsigned int string_len(char * c);

int myatoi(const char *string);

double sine(double);
double cosine(double);

unsigned char * get_erase_display_sequence(void);



#endif


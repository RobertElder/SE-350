#include "utils.h"
#include "uart_polling.h"

void assert(int value, unsigned char * message){
	if(value == 0){
		//uart0_put_string("\nTHERE WAS AN ASSERTION FAILURE!!!\n");
		uart0_polling_put_string(message);
		uart0_polling_put_string("\nAbnormal program termination.\n");
		// I can't figure out any other way to get it to stop executing.
		while (1) {}
	}
}

unsigned int string_len(char * c){
	int i = 0;
	while(c[i]){i++;}

	return i;
}

unsigned int pow(unsigned int base, unsigned int exponent){
	// Big surprise we have to write our own pow function
	if(exponent == 0)
		return 1;
	else
		return base * pow(base,exponent - 1);
}

int RANDOM_NUMBER_GENERATOR_m_w = 7;    /* must not be zero */
int RANDOM_NUMBER_GENERATOR_m_z = 123;    /* must not be zero */


unsigned int get_random(){
	/* This is a shitty way to generate random numbers but it works for what we need */
    RANDOM_NUMBER_GENERATOR_m_z = 36969 * (RANDOM_NUMBER_GENERATOR_m_z & 65535) + (RANDOM_NUMBER_GENERATOR_m_z >> 16);
    RANDOM_NUMBER_GENERATOR_m_w = 18000 * (RANDOM_NUMBER_GENERATOR_m_w & 65535) + (RANDOM_NUMBER_GENERATOR_m_w >> 16);
    return (RANDOM_NUMBER_GENERATOR_m_z << 16) + RANDOM_NUMBER_GENERATOR_m_w;  /* 32-bit result */
}


void printDigit(unsigned int i){
	unsigned char digits[] = "0123456789";
	assert(i < 10, "This should not happen");
	//Put a null after the digit we want so that we only print that digit
	digits[i+1] = 0;
	uart0_polling_put_string(&(digits[i]));
}

void printHexDigit(unsigned int i){
	unsigned char digits[] = "0123456789ABCDEF";
	assert(i < 16, "This should not happen");
	//Put a null after the digit we want so that we only print that digit
	digits[i+1] = 0;
	uart0_polling_put_string(&(digits[i]));
}

void print_printable_character(unsigned char c){
	char b[3];
	if(
		c > 31 && c < 127 ||
		c == 20
		){
		b[0] = c;
		b[1] = 32;
		b[2] = 0;
	}else if(c == 0){
		b[0] = 92;
		b[1] = 48;
		b[2] = 0;
	}else if(c == 10){
		b[0] = 92;
		b[1] = 110;
		b[2] = 0;
	}else if(c == 13){
		b[0] = 92;
		b[1] = 114;
		b[2] = 0;
	}else{
		b[0] = 46;
		b[1] = 32;
		b[2] = 0;
	}
	uart0_polling_put_string(b);
}

void print_hex_byte(unsigned int i){

	int hasSeenADigitYet = 1;
	// Max int is 4,294,967,295
	int currentExponent = 1;

	if(i == 0){
		printHexDigit(i);
		printHexDigit(i);
		return;
	}

	while(currentExponent >= 0){
		// What value does a digit have for this exponent? 
		int currentDigitValue = pow(16,currentExponent); 
		//  What is the value for this digit?
		int currentDigit = i / currentDigitValue;
		//  We have accounted for this quantity in our representation, remove it
		i -= currentDigit * currentDigitValue;

		//  Don't print leading 0's but do print interior zeros
		if(hasSeenADigitYet || currentDigit){
			//  Always print 0's now
			hasSeenADigitYet = 1;
			//  Determines if P = NP
			printHexDigit(currentDigit);
		}
		// Next value 
		currentExponent--;
	}
}

void print_unsigned_integer(unsigned int i){

	int hasSeenADigitYet = 0;
	// Max int is 4,294,967,295
	int currentExponent = 9;

	if(i == 0){
		printDigit(i);
		return;
	}

	while(currentExponent >= 0){
		// What value does a digit have for this exponent? 
		int currentDigitValue = pow(10,currentExponent); 
		//  What is the value for this digit?
		int currentDigit = i / currentDigitValue;
		//  We have accounted for this quantity in our representation, remove it
		i -= currentDigit * currentDigitValue;

		//  Don't print leading 0's but do print interior zeros
		if(hasSeenADigitYet || currentDigit){
			//  Always print 0's now
			hasSeenADigitYet = 1;
			//  Determines if P = NP
			printDigit(currentDigit);
		}
		// Next value 
		currentExponent--;
	}
}

void print_signed_integer(signed int i){
	if(i < 0){
		uart0_put_string("-");
		print_unsigned_integer((unsigned int)(-i));
	}else{
		print_unsigned_integer((unsigned int)i);		
	}
}

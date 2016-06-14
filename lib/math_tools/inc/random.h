#ifndef RANDOM_H_
#define RANDOM_H_


#include <stdint.h>


/*! Generate 8 bit random number

    \return 8bit random number from 1 to 255 inclusive
*/
uint8_t xorshift8(void);


/*! Generate 16 bit random number

    \return 16bit random number from 1 to 65535 inclusive
*/
uint16_t xorshift16(void);


/*! Generate 32 bit random number

    \return 32bit random number from 1 to 4294967295 inclusive
*/
uint32_t xorshift32(void);


/*! Generate 64 bit random number

    \return 64bit random number from 1 to 18446744073709551615 inclusive
*/
uint64_t xorshift64(void);






#endif /* RANDOM_H_ */

/* eof */

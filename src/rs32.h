/* Global definitions for Reed-Solomon encoder/decoder
 * Phil Karn KA9Q, August 1997
 *
 * The parameters MM and KK specify the Reed-Solomon code parameters.
 *
 * Set MM to be the size of each code symbol in bits. The Reed-Solomon
 * block size will then be NN = 2**M - 1 symbols. Supported values are
 * defined in rs.c.
 *
 * Set KK to be the number of data symbols in each block, which must be
 * less than the block size. The code will then be able to correct up
 * to NN-KK erasures or (NN-KK)/2 errors, or combinations thereof with
 * each error counting as two erasures.
 */

#ifndef __RS32_H__
#define __RS32_H__

#define MM  8		/* RS code over GF(256) */
#define KK  223		/* KK = number of information symbols */

#define	NN ((1 << MM) - 1)

#if (MM <= 8)
typedef unsigned char dtype;
#else
typedef unsigned int dtype;
#endif

/* This defines the type used to store an element of the Galois Field
 * used by the code. Make sure this is something larger than a char if
 * if anything larger than GF(256) is used.
 *
 * Note: unsigned char will work up to GF(256) but int seems to run
 * faster on the Pentium.
 */
typedef int gf;

/* Initialization function */
void init_rs(void);

/* Reed-Solomon encoding
 * data[] is the input block, parity symbols are placed in bb[]
 * bb[] may lie past the end of the data, e.g., for (255,223):
 *	encode_rs(&data[0],&data[223]);
 */
int rs32e(dtype data[KK], dtype bb[NN-KK]);

/* Reed-Solomon erasures-and-errors decoding
 * The received block goes into data[], and a list of zero-origin
 * erasure positions, if any, goes in eras_pos[] with a count in no_eras.
 *
 * The decoder corrects the symbols in place, if possible and returns
 * the number of corrected symbols. If the codeword is illegal or
 * uncorrectible, the data array is unchanged and -1 is returned
 */
int rs32d(dtype data[NN], int eras_pos[], int no_eras);
int rsd32(dtype data[NN], int eras_pos[NN-KK], int no_eras);
#ifdef ASM_I386
int rse32(dtype data[KK], dtype bb[NN-KK]);
#endif

#endif

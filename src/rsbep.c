/***************************************************************************
          rsbep.c  -  Reed-Solomon & Burst Error Protection
                         -------------------
    begin                : Mon Jul  30 20:27:09 MEST 2001
    copyright            : (C) 2000 by Guido Fiala
    email                : gfiala@s.netic.de
 ***************************************************************************/

/***************************************************************************
    Slightly modified by ttsiodras at removethis gmail dot com

    It doesn't write the 3 parameters of rsbep as a single line before the 
    data, as this makes the stream fragile (if this information is lost, 
    decoding fails)

    It uses a default value of 16*255=4080 for parameter D, and it can thus 
    tolerate 4080*16=65280 consecutive bytes to be lost anywhere in the 
    stream, and still recover...
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#ifdef ASM_I386
  // the hardcoded i386-asm version only for RS(255,223)  
#include "rs32.h"
#else
  // plain & slow C version (can also be used for anything different from RS(255,223))
#include "rs.h"
#define rse32 encode_rs
#define rsd32 eras_dec_rs
#endif

#define ERR_BURST_LEN (255*16)	// default number of blocks: 4080
#define RS_BSIZE 255		// default encoded block size
#define RS_DSIZE 223		// default input block size
#define RSBEP_VERSION "0.0.9"
#define RSBEP_HDR "rsbep"

union long_byte_union {
    long long i;
    char b[8];
};

//this function is used to copy src to dst while separating consecutive bytes
//equally distributed over the entire array, it tries to utilize several
//cpu-pipes and as high as possible memory efficiency without using asm.
//(but not several CPU's...anyway -it's only 15% of CPU-time, most is RS)

// 2010-06-13: Re-written by ttsiod, the old version had memory access errors
inline void distribute(dtype * src, dtype * dst, int rows, int cols,
		       int reverse)
{
    int totalBytes = rows * cols;
    int boundary = totalBytes;
    int offset = 0;

    if (!reverse) {
	while (totalBytes--) {
	    dst[offset] = *src++;
	    offset += 4080;
	    if (offset >= boundary)
		offset -= (boundary - 1);
	}
    } else {
	while (totalBytes--) {
	    *dst++ = src[offset];
	    offset += 4080;
	    if (offset >= boundary)
		offset -= (boundary - 1);
	}
    }
}

//decode buffer and write to stdout
inline void decode_and_write(char *ec_buf, int rs_bsize, int bep_size,
			     int rs_dsize, long *sum_of_failures,
			     long *uncorrectable_failures, char *argv,
			     int quiet, int verbose)
{
    register int i;
    int eras_pos[32];		//may once hold positions of erasures...
    for (i = 0; i < bep_size; i++) {
	int failure = rsd32((unsigned char *)(ec_buf + i * rs_bsize), eras_pos, 0);
	if (failure > 0) {
	    *sum_of_failures += failure;
	    if (!quiet && verbose)
		fprintf(stderr,
			"%s: errors corrected: %ld uncorrectable blocks: %ld\r",
			argv, *sum_of_failures, *uncorrectable_failures);
	} else if (failure == -1) {
	    (void)*uncorrectable_failures++;
	    if (!quiet && verbose)
		fprintf(stderr,
			"%s: errors corrected: %ld uncorrectable blocks: %ld\r",
			argv, *sum_of_failures, *uncorrectable_failures);
	}
	if (rs_dsize != fwrite((ec_buf + i * rs_bsize), sizeof(dtype), rs_dsize, stdout)) {	//write decoded data
	    //perror("serious problem: ");
	    exit(-1);
	}
    }
}

// main-program: reads commandline args and does everything else...

int main(int argc, char *argv[])
{
    dtype *ec_buf = NULL;	// pointer to encoding/decoding buffer
    dtype *tr_buf = NULL;	// pointer to transmit-buffer (fread/fwrite)
    long verbose = 0, decode = 0, quiet = 0, temp = 0, override = 0;
    long rs_bsize = 0, rs_dsize = 0, bep_size = 0;
    long i = 0;
    long sum_of_failures = 0;
    long uncorrectable_failures = 0;
    extern char *optarg;

    assert(sizeof(dtype) == 1);	//anything works only with bytes

    while ((i = getopt(argc, argv, "dvqB:D:R:")) != EOF) {
	switch (i) {
	case 'd':
	    decode = 1;
	    break;
	case 'v':		/* Be quite verbose */
	    verbose = 1;
	    break;
	case 'q':		/* Be strictly quiet */
	    quiet = 1;
	    break;
	case 'B':		/* take rs_bsize */
	    temp = atoi(optarg);
	    if ((temp <= 255) && (temp >= 32)) {
		rs_bsize = temp;
		override = 1;
	    } else {
		fprintf(stderr,
			"%s: illegal RS-block size (32..255), bailing.\n",
			argv[0]);
		exit(-1);
	    }
	    break;
	case 'D':		/* take rs_dsize */
	    temp = atoi(optarg);
	    if ((temp <= rs_bsize - 2) && (temp >= rs_bsize - 32)) {
		rs_dsize = temp;
		override = 1;
	    } else {
		fprintf(stderr,
			"%s: illegal RS-data size (RS-block-2..-32), bailing.\n",
			argv[0]);
		exit(-1);
	    }
	    break;
	case 'R':		/* take bep_size */
	    temp = atoi(optarg);
	    if ((temp > rs_bsize) && (temp <= 65535))	//just a sane limit
	    {
		bep_size = temp;
		override = 1;
	    } else {
		fprintf(stderr,
			"%s: illegal error burst len (has to be >=RS-block size, <65535), bailing.\n",
			argv[0]);
		exit(-1);
	    }
	    break;
	default:
	    printf("rsbep version %s\n", RSBEP_VERSION);
	    printf("usage: %s [-d] [-v] [-q] [-B x -D y -R z ]\nd=decode,"
		   "v=verbose, q=quiet\nOverride-Settings with:\n"
		   "x=Reed-Solomon Blocksize, "
		   "y=Reed-Solomon-Datasize, z=ERR_BURST_LEN\n"
		   "(defaults to 255,223,765)\n", argv[0]);
	    exit(1);
	}
    }
#ifdef ASM_I386
    init_rs();			//Initialisiere RS(255)-GF(256)
#else
    //c-Version initialises itself...
#endif
    if (decode) {
	if (!override) {
	    rs_bsize = RS_BSIZE;
	    rs_dsize = RS_DSIZE;
	    bep_size = ERR_BURST_LEN;
	}
	ec_buf = malloc(rs_bsize * bep_size * sizeof(dtype));
	tr_buf = malloc(rs_bsize * bep_size * sizeof(dtype));
	if (ec_buf == NULL || tr_buf == NULL) {
	    fprintf(stderr, "%s: out of memory, bailing.\n", argv[0]);
	    exit(-1);
	}
	//go on:
	while (!feof(stdin)) {
	    long got = fread(tr_buf, 1, bep_size * rs_bsize, stdin);	//read the encoded data
	    if (!got)
		break;
	    if (got < (bep_size * rs_bsize)) {
		bzero((tr_buf + got), (bep_size * rs_bsize) - got);	//zero remaining data - should never happen!
	    }

	    distribute(tr_buf, ec_buf, bep_size, rs_bsize, 1);
	    decode_and_write((char *)ec_buf, rs_bsize, bep_size, rs_dsize,
			     &sum_of_failures, &uncorrectable_failures,
			     argv[0], quiet, verbose);
	}
	if ((sum_of_failures > 0 || uncorrectable_failures > 0 || verbose)
	    && !quiet) {
	    fprintf(stderr, "\n%s: number of corrected failures   : %ld\n",
		    argv[0], sum_of_failures);
	    fprintf(stderr, "%s: number of uncorrectable blocks : %ld\n",
		    argv[0], uncorrectable_failures);
	}
    } else {			//encode
	if (!override) {
	    rs_bsize = RS_BSIZE;
	    rs_dsize = RS_DSIZE;
	    bep_size = ERR_BURST_LEN;
	}
	//get the buffers we need (here we know how large):
	ec_buf = malloc(rs_bsize * bep_size * sizeof(dtype));
	tr_buf = malloc(rs_bsize * bep_size * sizeof(dtype));
	if (ec_buf == NULL || tr_buf == NULL) {
	    fprintf(stderr, "%s: out of memory, bailing.\n", argv[0]);
	    exit(-1);
	}
	//no go on with data
	while (!feof(stdin)) {
	    //now encode and write the block:
	    for (i = 0; i < bep_size; i++) {
		long got =
		    fread((ec_buf + i * rs_bsize), 1, rs_dsize, stdin);
		if (got < rs_dsize) {
		    bzero((ec_buf + i * rs_bsize + got), rs_dsize - got);	//zero remaining data
		}
		//encode rs_dsize bytes of data, append Parity in row
		rse32((ec_buf + i * rs_bsize),
		      (ec_buf + i * rs_bsize + rs_dsize));
	    }
	    distribute(ec_buf, tr_buf, bep_size, rs_bsize, 0);
	    if (bep_size * rs_bsize != fwrite(tr_buf, sizeof(dtype), bep_size * rs_bsize, stdout)) {	//write the encoded data
		perror("serious problem: ");
		if (ec_buf)
		    free(ec_buf);
		if (tr_buf)
		    free(tr_buf);
		exit(-1);
	    }
	}
    }
    //clean up:
    if (ec_buf)
	free(ec_buf);
    if (tr_buf)
	free(tr_buf);
    return 0;
}

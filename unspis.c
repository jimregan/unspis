/*****************************************************************************\

UNSPIS 1.0 - a simple SPIS LZH dearchiver.  Freeware, distributed under the GPL

Dearchives, decrypts, and decompresses archives created by SPIS TCompress
Delphi component version 3.5 through 4.0  http://www.spis.co.nz/compress.htm
Example: Bersoft HyperMaker publications  http://bersoft.com/hmhtml/index.htm
 like Aesop 1001 Killer Internet Tactics  http://www.killertactics.com/

Copyright © 1999 Luchezar Iliev Georgiev, Bulgaria  <lucho@mbox.digsys.bg>
					  http://www.eunet.bg/eunetweb/lucho

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


Usage:	unspis archive [key]

Data encryption used by TCompress is a simple repetitive XOR with a 32-bit
key (a Vigenere cipher).  Experts may propose a faster decryption method...

Key:	a 32-bit value, expressed as decimal, hexadecimal, or octal number.
	How do you obtain it?  Well, if you have some sample files of same
	type as the archived ones, then try this:
		1) Compile lzhuf.c and compress your files with it;
		2) Manually XOR the first four data bytes (skipping the size
		   header) of the compressed files and the files in archive.
	That's the key!
	For example, for HyperMaker publications, the key is 0x1F4410B.  And
	that is the default key value if none specified on the command line.
	By the way, the key may coincide with the checksum MAGIC (base) value
	which can be observed as the checksum of the stored files (supposedly
	0)!  I think this is just a TCompress component bug and may be fixed.

Build:	GNU C compiler preferable, although Microsoft® and Borland® work too.
	Don't forget to link with lzhuf.c (use the simple makefile included).

Beware of long file names used in most SPIS archives.  MS-DOS® will truncate
them to its 8.3 standard, so it's not the ideal environment for using UNSPIS.

Current limitations:
	1) Only multi-file archives can be processed.
	2) Only coNone and coLZH methods is supported.
	3) Any existing files will be overwritten.
	4) All files extracted in current directory; no directories created.
	5) Restoring of the original file date & time is not yet supported.
	6) Extraction of only certain file[s] of the archive not supported.
Well, it's just a quick hack, what'd you expect?!  Anyway, any improvements
are welcome!

\*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#if defined (_MSDOS) || defined (__MSDOS__)
  #include <stdlib.h>		/* for _fmode */
  #include <fcntl.h>		/* for O_BINARY */
#else
  #define   _MAX_PATH	_POSIX_PATH_MAX
  #define   min(a,b)	((a) <= (b) ? (a) : (b))
#endif
#pragma pack (1)		/* pack header structures */
#include "compress.h"

#define MAGIC	0x1F4410B	/* base for TCompress checksum calculations */

extern FILE *infile, *outfile;	/* in "lzhuf.c" */
extern unsigned int getbuf;
extern unsigned char getlen;
extern void Decode(long);

static void error(const char *filename)
{
    perror(filename);
    exit(errno);
}

int main(int argc, char **argv)
{
    static char path[_MAX_PATH], buf[BUFSIZ];
    char *name;
    int count;
    int sum, key = MAGIC, total, xorbuf, i,
	stored = 0, compressed = 0, failed = 0, unknown = 0;
    struct TCompressHeader mainHdr;
    struct TCompressedFileHeader hdr;
    FILE *archive;

#if defined (_MSDOS) || defined (__MSDOS__)
    _fmode = O_BINARY;		/* otherwise default is O_TEXT */
#endif				/* (Unix doesn't need this) */
    /*
     * Process command line arguments
     */
    if (argc < 2 || argc > 3) {
	printf("usage: unspis archive [key]\n");
	exit(EAGAIN);
    }
    if (argc == 3  &&  sscanf(argv[2], "%li", &key) != 1) {
	printf("\"%s\": invalid key\n", argv[2]);
	exit(EINVAL);
    }
    /*
     * Read archive header
     */
    if ((archive = fopen(argv[1], "r")) == NULL)
	error(argv[1]);
    if (fread(&mainHdr, sizeof(mainHdr), 1, archive) != 1)
	error(argv[1]);
    if (strncmp(mainHdr.ComId, "SPIS\032", 5) || strncmp(mainHdr.ComId, "SPIS", 4)) {
	printf("\"%s\" is not a SPIS archive\n", argv[1]);
	exit(EBADF);
    }
    if (mainHdr.ArchiveType != caMulti) {
	printf("\"%s\" is not a multi-file archive\n", argv[1]);
	exit(ERANGE);
    }
    /*
     * Extract each archived file
     */
    for ( ; ; ) {
	if (fread(&hdr, sizeof(hdr), 1, archive) != 1) {
	    if (feof(archive))
		break;				/* finished */
	    else
		error(argv[1]);
	}
	/*
	 * Cut out leading directory names, if any
	 */
	if (fread(path, hdr.FileNameLength, 1, archive) != 1)
	    error(argv[1]);
	path[hdr.FileNameLength] = 0;		/* name copied -- terminate */
	if ((name = strrchr(path, '\\')) == NULL)
	    name = path;			/* no directories */
	else
	    name++;				/* skip the backslash */
	printf("%s\n", name);
	switch (hdr.CompressedMode) {
	/*
	 * File is stored -- just copy it out
	 */
	case coNone:
	    if ((outfile = fopen(name, "w")) == NULL)
		error(name);
	    for (total = 0; total < hdr.Fullsize; total += count) {
		count = (int)min(BUFSIZ, hdr.Fullsize - total);
		fread(buf, count, 1, archive);
		if (ferror(archive))
		    error(argv[1]);
		if (feof(archive)) {
		    printf("\"%s\": premature EOF\n", argv[1]);
		    exit(EMFILE);
		}
		if (fwrite(buf, count, 1, outfile) != 1)
		    error(name);
	    }
	    fclose(outfile);
	    stored++;
	    break;
	/*
	 * File is compressed with the LZH method -- decompress it
	 */
	case coLZH:
	    if (hdr.Locked == 1)
		printf("warning: \"%s\" is locked\n", name);
	    /*
	     * Decrypt the file while copying it to a temporary file
	     * Do it in portions of 4 bytes, such is key length
	     */
	    if ((infile = tmpfile()) == NULL)
		error("tmpfile");
	    for (total = 0; total < hdr.CompressedSize; total += count) {
		count = (int)min(sizeof(int), hdr.CompressedSize - total);
		fread(&xorbuf, count, 1, archive);
		if (ferror(archive))
		    error(argv[1]);
		if (feof(archive)) {
		    printf("\"%s\": premature EOF\n", argv[1]);
		    exit(EMFILE);
		}
		if (hdr.Locked == 2)	/* encrypted via Vigenere cipher */
		    xorbuf ^= key;
		if (fwrite(&xorbuf, count, 1, infile) != 1)
		    error(name);
	    }
	    /*
	     * Now, decompress the temporary file to the output file,
	     * calculate its checksum and compare it to the original.
	     */
	    rewind(infile);
	    if ((outfile = fopen(name, "w+")) == NULL)
		error(name);
	    getbuf = getlen = 0;	/* initialise decoder */
	    Decode(hdr.Fullsize);
	    fclose(infile);
	    sum = MAGIC;
	    rewind(outfile);
	    for (i = 0; i < hdr.Fullsize; i++)
		sum += getc(outfile);
	    fclose(outfile);
	    if (sum != hdr.Checksum) {
		remove(name);
		printf("warning: \"%s\" failed checksum and removed\n", name);
		failed++;
	    }
	    compressed++;
	    break;
	default:
	    /*
	     * We can't understand the other compression methods yet
	     */
	    printf("warning: \"%s\": unknown compression method\n", name);
	    fseek(archive, hdr.FileNameLength + hdr.CompressedSize, SEEK_CUR);
	    unknown++;
	}
    }
    /*
     * Finally, print some file statistics
     */
    printf( "\n"
	    "****** SUMMARY: ******\n"
	    "%5ld files stored\n"
	    "%5ld files compressed (%ld failed)\n"
	    "%5ld files unknown\n"
	    "----------------------\n"
	    "%5ld files total\n\n", stored, compressed, failed, unknown,
				    stored + compressed + unknown
    );
    return errno;
}

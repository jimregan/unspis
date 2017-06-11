unspis: unspis.c lzhuf.c compress.h
	$(CC) -O2 -DSPIS_LZH unspis.c lzhuf.c

/*
 * These constants are used in the archive header of each archive to
 * note what kind of archive it is.
 */
enum TCompressArchiveType {
    caSingle,		/* single item archive (no file headers) */
    caMulti		/* multi-file archive (file header for each) */
};

enum TCompressionMethod {
    coNone,		/* no data compression -- just store */
    coRLE,		/* compress with RLE */
    coLZH,		/* compress with LZH1 */
    coCustom,		/* compress with Custom method */
    coLZH5		/* compress with LZH5 */
};			/* (30-50% faster, 10-20% better than LZH1) */

/*
 * Every compressed archive (whether single-item or multi-file) starts with
 * a copy of this header.
 *
 * If it is a multi-file archive, this header is immediately followed by
 * the first file header, followed in turn by the compressed copy of that
 * file.  Note that the Locked flag is unimportant for multi-file archives.
 */
struct TCompressHeader {
    char ComId[5];	/* Always SPIS + chr(26) */
    char ComMethodId[3];/* RLE, LZH, NON, CUS/user-defined */
    long Fullsize;	/* excl. header, not used for caMulti*/
    char ArchiveType;	/* caSingle, caMulti */
    long Checksum;	/* checksum value, not used for caMulti */
    long Locked;	/* 1 if file has been locked (V3.05 or earlier), */
};			/* 2 if encrypted (V3.5 or later), otherwise 0 */

/*
 * In a multi-file archive, the archive header is followed by 0 or more
 * repeats of this header, each followed by its filename and then its
 * compressed file data.
 *
 * The filename is max 255 characters long, and is always stored with no
 * drive letter (although a path may be stored if desired).
 *
 * Note that compressedMode can vary from file to file in a single archive.
 */
struct TCompressedFileHeader {
    short FileNameLength;/* filename itself immediately follows this header */
    long Datetime;	/* original file date and time (in MS-DOS format) */
    short Attributes;	/* original file attributes (again MS-DOS --L.G.) */
    long Fullsize;	/* original file size */
    long CompressedSize;/* size of compressed copy of the file */
    char CompressedMode;/* how compressed */
    long Checksum;	/* checksum value of original file, 0 if coNone (?!) */
    long Locked;	/* 1 if file has been locked (V3.05 or earlier), */
};			/* 2 if encrypted (V3.5 or later), otherwise 0 */

/******************************************************************************

Key Property
============
Applies to
TCompress component.

Declaration  Changed in V3.5
property Key: Longint; Default = 0, meaning no protection

Description
-----------
This property allows compressed items or files to be "locked" in such a
way that your program will not be able to decompressed them unless the
key is set to the correct value first.

During compression, set this key to the desired value -- as it is a
Longint, very high values can be used if need be. Note that in the case
of a multi-file archive, it is possible to have some files protected
with one key, some with a different key, and some not protected at all.

During expansion, if the key is not the correct value, an EInvalidKey
exception will be generated.

Notes:
------
1) V3.5 change: Earlier "bicycle lock" protection has been replaced by
file encryption (unless a file is stored with a CompressionMethod of
coNone). V3.5 and later will still unlock files protected with earlier
versions.

2) Be careful when experimenting with Key, particularly if you are using
DoCompress, CompressString, compressing blobs or using CDBMemo/CDBImage.
If Key is non-zero, protection will be applied in all these cases so
make sure that is your intention!

Author: Peter Hyde

******************************************************************************/

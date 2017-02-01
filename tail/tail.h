/*
 *  tail.h
 *  Berlin
 *
 *  Created by Nikki Chumakov on 28/01/2017.
 *
 *  This is free and unencumbered software released into the public domain.
 *
 *  Anyone is free to copy, modify, publish, use, compile, sell, or
 *  distribute this software, either in source code form or as a compiled
 *  binary, for any purpose, commercial or non-commercial, and by any
 *  means.
 */

#ifndef tail_h
#define tail_h

#include <stdlib.h>      /* size_t  */
#include <sys/types.h>   /* ssize_t */

/* return codes from tail_xxxx functions */
#define TAIL_OK     0
#define TAIL_FAIL   1
#define TAIL_NOTSUP 2 /* not supported */


/**
 Copy last lines from src into dst using mmap(2)

 @param file file name (used for error reporting only)
 @param src source file descriptor
 @param dst destination file descriptor
 @param lines number of lines to copy
 @param segment_size buffer size
 @param fsize file size
 @return -1 on error, see errno for description
 */
int tail_mmap (const char* file, int src, int dst, size_t lines,
               size_t segment_size, off_t fsize);

/**
 Copy last lines from src into dst using read(2) and seek(2)
 
 @param file file name (used for error reporting only)
 @param src source file descriptor
 @param dst destination file descriptor
 @param lines number of lines to copy
 @param segment_size buffer size
 @param fsize file size
 @return -1 on error, see errno for description
 */
int tail_read (const char* file, int src, int dst, size_t lines,
               size_t segment_size, off_t fsize);

/**
 Copy last lines from src into dst using read(2)
 
 @param file file name (used for error reporting only)
 @param src source file descriptor
 @param dst destination file descriptor
 @param lines number of lines to copy
 @param segment_size buffer size
 @param fsize file size
 @return -1 on error, see errno for description
 */
int tail_pipe (const char* file, int src, int dst, size_t lines,
               size_t segment_size, off_t fsize);

/* returned by window_destroy_fn on error */
#define WIN_FAILED ((void*) -1)

/* args: buff, fd, offset, length */
typedef void* (* window_create_fn) (void*, int, off_t, size_t);

/* args: window, length */
typedef int  (* window_destroy_fn) (void*, size_t);

int tail_window (const char* file, int src, int dst, size_t lines,
                 size_t segment_size, off_t fsize,
                 window_create_fn wcreate, window_destroy_fn wdestroy);

/**
 Checks if the character is end-of-line symbol
 
 @param ch character
 @return 1 - if the ch is EOL, 0 - otherwise
 */
int is_eol (char ch);

/*
 Writes full buffer or returns failure. eliminates partial writes, e.g. on
 no space on FS
 
 @param fd file descriptor
 @param buf pointer to output buffer of bytes
 @param nbyte number of the bytes to write
 @return nuber of bytes written or -1 on error
 */
ssize_t write_all (int fd, const void *buf, size_t nbyte);

#endif /* tail_h */

/* vi: ts=8 et sts=2 sw=2 ai si cin :
 */

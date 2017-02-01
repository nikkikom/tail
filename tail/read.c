/*
 *  read.c
 *  Berlin Project
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

/*
 *  tail_mmap reads files backward using tail_window function and 
 *  read(2) + lseek(2) for low-level read.
 */

#include <unistd.h> /* lseek, read, sysconf  */
#include <stdlib.h> /* malloc, free          */
#include <errno.h>  /* errno                 */
#include <stdio.h>  /* perror                */

#include "tail.h"

static void* win_read_create (void* addr, int fd, off_t offset, size_t length)
{
  ssize_t rc;
  void* buf = addr;
  
  if (NULL == buf && NULL == (buf = malloc (length)))
  {
    errno = ENOMEM;
    return WIN_FAILED;
  }
  
  if (-1 == lseek (fd, offset, offset>=0 ? SEEK_SET : SEEK_END))
  {
    if (NULL == addr) free (buf);
    return WIN_FAILED;
  }
  
  if ((rc = read (fd, buf, length)) == -1)
  {
    /* we read less than expected. it may happen if file is trancating 
     * during the read
     */
    int saved_errno = errno;
    if (NULL == addr) free (buf);
    errno = saved_errno;
    return WIN_FAILED;
  }
  
  return buf;
}

static int win_read_destroy (void* addr, size_t length)
{
  if (NULL == addr)
  {
    errno = EINVAL;
    return -1;
  }

  free (addr);
  return 0;
}

int tail_read (const char *file, int src, int dst, size_t lines,
               size_t segment_size, off_t fsize)
{
  const long page_size = sysconf(_SC_PAGESIZE);

  /* arg checks */
  if (! (
            file != NULL
         && src >= 0
         && dst >= 0
         && lines > 0
         && fsize > 0
  ))
  {
    errno = EINVAL;
    perror ("tail_read");
    return TAIL_FAIL;
  }
  
  /* will use 4 pages by default (wild guess) */
  if (0 == segment_size) segment_size = 4 * page_size;
  
  /* segment size should not be greater than file size */
  if (segment_size > fsize) segment_size = fsize;
  
  return tail_window (file, src, dst, lines, segment_size, fsize,
                      &win_read_create, &win_read_destroy);
}

/* vi: ts=8 et sts=2 sw=2 ai si cin :
 */

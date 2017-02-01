/*
 *  mmap.c
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
 *  tail_mmap reads files backward using tail_window function and mmap(2) 
 *  for low-level read.
 */


#include <errno.h>    /* errno   */
#include "tail.h"

#if HAVE_MMAP

#include <unistd.h>   /* sysconf */
#include <sys/mman.h> /* mmap    */
#include <stdio.h>    /* perror  */

static void* win_mmap_create (void* addr, int fd, off_t offset, size_t length)
{
  void* ret = mmap (addr, length, PROT_READ,
                    MAP_PRIVATE | (NULL != addr ? MAP_FIXED : 0), fd, offset);
  return MAP_FAILED == ret ? WIN_FAILED : ret;
}

static int win_mmap_destroy (void* addr, size_t length)
{
  return munmap (addr, length);
}

int tail_mmap (const char *file, int src, int dst, size_t lines,
               size_t segment_size, off_t fsize)
{
  const long page_size = sysconf(_SC_PAGESIZE);
  
  /* arg checks */
  if (! (   file != NULL
         && src >= 0
         && dst >= 0
         && lines > 0
         && fsize > 0
  ))
  {
    errno = EINVAL;
    perror ("tail_mmap");
    return TAIL_FAIL;
  }
  
  /* will use 4 pages by default (wild guess) */
  if (0 == segment_size) segment_size = 4 * page_size;

  /* segment size should not be greater than file size */
  if (segment_size > fsize) segment_size = fsize;
  
  /* may need to round up segment size to
   * page size because of mmap(2) constraints 
   */
  if (segment_size % page_size)
    segment_size += page_size - (segment_size % page_size);
  
  return tail_window (file, src, dst, lines, segment_size, fsize,
                      &win_mmap_create, &win_mmap_destroy);
}

#else /* HAVE_MMAP */

int tail_mmap (const char *file, int src, int dst, size_t lines,
               size_t segment_size, off_t fsize)
{
  errno = ENOTSUP;
  return TAIL_FAIL;
}

#endif /* HAVE_MMAP */

/* vi: ts=8 et sts=2 sw=2 ai si cin :
 */

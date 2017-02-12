/*
 *  pipe.c
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
 *  tail_pipe reads all the file into the buffers connected into a circular
 *  ring structure "bring". This ring grows as needed. Every new block is
 *  scanned for new-line characters and start-line positions are recorded 
 *  into a ring buffer "lring". Its size is fixed to the number of the lines
 *  we need to send to output. Blocks from "bring" is freed as soon as line
 *  pointers from "lring" does not point to them anymore. The other strategy
 *  could be to reuse them from future blocks.
 */

#include <errno.h>   /* errno           */
#include <unistd.h>  /* read, sysconf   */
#include <stdlib.h>  /* alloc, realloc  */
#include <string.h>  /* memset, memmove */
#include <stdio.h>   /* perror          */
#include <assert.h>  /* assert          */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "tail.h"

/* ring buffer for the file segments */
struct blocks_ring
{
  size_t block_size;
  size_t last_block_size;
  
  size_t capacity;
  size_t size;
  char **tail;
  char **base;
};

/* ring bugger for the pointers to the lines start positions */
struct lines_ring
{
  size_t      capacity;
  size_t      size;     /* occupied cells                          */
  size_t      tail;     /* index of first free cell                */
  const char *base[];   /* array of pointers to the start of lines */
};

/* formulas to deal with ring buffers */
#define RING_SIZE(R)     (R.size)
#define RING_CAPACITY(R) (R.capacity)
#define RING_BASE(R)     (R.base)
#define RING_END(R)      (R.base + R.capacity)
#define RING_TAIL(R)     (R.tail)
#define RING_HEAD(R)     (R.base + (R.tail + R.size - R.base) % R.capacity)

int tail_pipe (const char *file, int src, int dst, size_t lines,
               size_t segment_size, off_t fsize)
{
  int ret = TAIL_FAIL;
  int saved_errno = 0;
  
  /* first file position is always start of line */
  int at_line_start = 1;
  
  /* will use it for calculation default segment size */
  const long page_size = sysconf(_SC_PAGESIZE);

  /* ring buffers */
  struct blocks_ring bring;
  struct lines_ring *lring;
  
  size_t lhead;
  
  /* checks input arguments */
  if (! (
            file != NULL
         && src >= 0
         && dst >= 0
         && lines > 0
         && fsize >= 0
  ))
  {
    errno = EINVAL;
    perror ("tail_pipe");
    return TAIL_FAIL;
  }
  
  /* will use 4 pages by default (wild guess) */
  if (0 == segment_size) segment_size = 4 * page_size;
  
  /* segment size should not be greater than file size */
  if (fsize > 0 && segment_size > fsize) segment_size = fsize;
  
  /* initialize blocks */
  memset (&bring, 0, sizeof bring);
  bring.block_size = segment_size;
  
  /* initialize lines */
  lring = (struct lines_ring*) malloc (sizeof *lring +
                                       lines * sizeof (const char*));
  if (NULL == lring)
  {
    /* saved_errno = */ errno = ENOMEM;
    perror ("tail_pipe");
    return TAIL_FAIL;
  }
  
  memset (lring, 0, sizeof *lring + lines * sizeof (const char*));
  lring->capacity = lines;
  
  for (;;) /* file read loop */
  {
    ssize_t rlen;
    char const* first_line;
    char* current_block;
    
    /* check if need more space in blocks ring */
    if (bring.size >= bring.capacity)
    {
      size_t new_capacity = bring.capacity ? bring.capacity * 2 : 4;
      
      /* extend block ptrs storage */
      char **b = (char**) realloc (bring.base, new_capacity * sizeof *b);
      
      if (NULL == b)
      {
        saved_errno = errno = ENOMEM;
        perror ("tail_pipe");
        goto free_bring;
      }
      
      /* accomodate pointers to new mem region */
      bring.tail += b - bring.base;
      bring.base = b;
      
      /* move tailing blocks to the new end */
      if (bring.tail + bring.size > RING_END (bring))
      {
        memmove (bring.tail + new_capacity - bring.capacity, bring.tail,
                sizeof (char*) * (RING_END (bring) - bring.tail));
        bring.tail += new_capacity - bring.capacity;
      }

      bring.capacity = new_capacity;
    }
    
    /* check point */
    assert (bring.size < bring.capacity);
    
    /* read next buffer */
    if ((current_block = *RING_HEAD(bring) = (char*) malloc (segment_size))
                                                     == NULL)
    {
      saved_errno = errno = ENOMEM;
      perror ("tail_pipe");
      goto free_bring;
    }
    
    ++bring.size;
    
    if ((rlen = read (src, current_block, segment_size)) == -1)
    {
      saved_errno = errno;
      perror (file);
      goto free_bring;
    }
    
    /* no more data: nothing to do */
    if (! rlen) {
      /* undo last block */
      free (current_block);
      --bring.size;
      break;
    }
    
    /* only the very last block may be less than block_size
     * if we fail here it may mean that file was changed (trancated)
     * during the read and it is impossible to show last lines (correctly)
     */
    if (! (rlen <= bring.block_size) &&
        ! (rlen == bring.block_size ||
           bring.last_block_size == bring.block_size))
    {
      saved_errno = errno = EIO;
      perror (file);
      goto free_bring;
    }
    
    bring.last_block_size = rlen;
    
    /* scan current block for new lines */
    for (const char* s = current_block;
                     s < current_block + rlen; at_line_start = is_eol (*s++))
      if (at_line_start)
      {
        /* track this line position */
        lring->base[lring->tail++] = s;
        lring->tail %= lring->capacity;
      }

    if (NULL != (first_line = lring->base[lring->tail]))
      while (first_line < *bring.tail ||
             first_line >= *bring.tail + bring.block_size)
      {
        free (*bring.tail++);
        if (bring.tail >= RING_END (bring)) bring.tail -= bring.capacity;
        --bring.size;
        assert (bring.size > 0);
      }
  } /* file read loop */
  
  /* find first stored line ptr. for-loop stops at first not-null pointer or
   * when lhead will become lring->tail second time 
   */
  for (lhead = lring->tail; NULL != lring->base[lhead] &&
       lring->tail != (lhead = (lhead+1) % lring->capacity);)
    ;
  
  /* write the lines */
  if (NULL != lring->base[lhead])
  {
    const char* start;
    char **blocks_ptr = bring.tail;
    size_t blocks_num = bring.size;
    assert (blocks_num > 0);
    
    for (start = lring->base[lhead]; blocks_num > 0; --blocks_num)
    {
      size_t blen = bring.block_size;
      assert (start >= *blocks_ptr);
      if (blocks_num == 1) blen = bring.last_block_size;
      assert (start < *blocks_ptr + blen);
      
      // FIXME: check errors
      write_all (dst, start, blen - (start - *blocks_ptr));
    
      if (++blocks_ptr >= RING_END (bring))
        blocks_ptr = bring.base;
      
      start = *blocks_ptr;
    }
  }

free_bring:
  for (; bring.size > 0; --bring.size)
  {
    free (*bring.tail);
    if (++bring.tail >= RING_END (bring)) bring.tail = bring.base;
  }
  ret = TAIL_OK;

  free (bring.base);

  /* free lring */
  free (lring);
  
  errno = saved_errno;
  return ret;
}

/* vi: ts=8 et sts=2 sw=2 ai si cin :
 */


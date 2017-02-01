/*
 *  window.c
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
 *  tail_window reads the file backword in segments until it finds the 
 *  specified number of lines. It read the segments with the help of
 *  window_create ans window_destroy functions.
 */

#include <errno.h>   /* errno          */
#include <stdio.h>   /* perror         */
#include <assert.h>  /* assert         */

#include "tail.h"

int tail_window (const char *file, int src, int dst, size_t lines,
                 size_t segment_size, off_t fsize,
                 window_create_fn wcreate, window_destroy_fn wdestroy)
{
  char *window = NULL;
  off_t segment;
  unsigned num_of_segments;
  size_t line = 0;
  size_t len = 0;
  char *s = 0;
  
  /* arg checks */
  if (! (
            file != NULL
         && src >= 0
         && dst >= 0
         && lines > 0
         && segment_size > 0
         && fsize > 0
  ))
  {
    errno = EINVAL;
    perror (file);
    return TAIL_FAIL;
  }

  // actually it's number of segments minus 1
  num_of_segments = (unsigned) ((fsize-1) / segment_size);
  
  /* go backwords through the file, looking for <lines> lines */
  for (segment = num_of_segments; segment >= 0 && line < lines; --segment)
  {
    /* the length of current segment is 
     * segment_size except for last one 
     */
    len = (num_of_segments == segment)
        ? fsize - segment * segment_size
        : segment_size;
    
    /* map the file to the sliding window */
    s = (*wcreate) (window, src, segment * segment_size, segment_size);
    if (s == WIN_FAILED)
    {
      if (NULL != window)
      {
        int saved = errno;
        (void) (*wdestroy) (window, segment_size);
        errno = saved;
      }
      perror (file);
      return TAIL_FAIL;
    }
    
    /* the address of the window may be changed */
    window = s;
    
    /* ignore <NL> if file ends with it 
     * (the most simple way to deal with that is just to 
     * increase line by one)
     */
    if (segment == num_of_segments && is_eol (window[len-1]))
      ++lines;
    
    
    /* looking for <lines> new lines in window */
    for (s = window + len - 1; s >= window; --s)
    {
      if (is_eol (*s) && ++line >= lines) break;
    }
    
    if (line >= lines)
      break;
  }
  
  ++s;
  
  assert (NULL != window);
  assert (NULL != s);
  assert (s >= window);
  assert (s - window <= len);

  /* output the current segment */
  if (segment >= 0 && s - window < len &&
      write_all (dst, s, window + len - s) == -1)
  {
    int saved = errno;
    (void) (*wdestroy) (window, segment_size);
    errno = saved;
    perror ("write");
    return TAIL_FAIL;
  }
  
  /* output all segments till eof */
  for (++segment; segment <= num_of_segments; ++segment)
  {
    char *win;

    len = (num_of_segments == segment)
        ? fsize - segment * segment_size
        : segment_size;

    win = (*wcreate) (window, src, segment * segment_size, segment_size);
    if (win == WIN_FAILED)
    {
      if (NULL != window)
      {
        int saved = errno;
        (void) (*wdestroy) (window, segment_size);
        errno = saved;
      }
      perror (file);
      return TAIL_FAIL;
    }
    
    window = win;
    
    if (write_all (dst, window, len) == -1)
    {
      int saved = errno;
      (void) (*wdestroy) (window, segment_size);
      errno = saved;
      perror ("write");
      return TAIL_FAIL;
    }
  }
  
  if (wdestroy (window, segment_size) == -1)
  {
    /* should we really return the error here? */
    perror ("munmap");
    return TAIL_FAIL;
  }

  return TAIL_OK;
}

/* vi: ts=8 et sts=2 sw=2 ai si cin :
 */

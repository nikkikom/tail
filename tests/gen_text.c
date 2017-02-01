/*
 *  gen_text.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main (int argc, char* argv[])
{
  int rep = 10, len = 72;
  char c = '0';
  char NL = '\n';
  ssize_t unusd, unused = 0;
  int no_end_nl = 0;

  srand ((unsigned) time (0));
  
  for (int i=1; i<argc; ++i)
  {
    const char* token = strtok (argv[i], ":");
    if (! token) continue;
    
    if (strcmp (token, "new-line") == 0)
    {
      token = strtok (NULL, ":");
      if (token) NL = *token;
    }
    else if (strcmp (token, "no-end-nl") == 0)
    {
      no_end_nl = 1;
    }
    else if (strcmp (token, "string") == 0)
    {
      token = strtok (NULL, ":");
      if (token)
      {
        rep = atoi (token);
        token = strtok (NULL, ":");

        if (token)
        {
          c = *token;
          token = strtok (NULL, ":");
          
          if (token)
            len = atoi (token);
        }
      }
      
      char* buf = (char*) malloc (len+1);
      memset (buf, c, len);
      buf[len] = NL;
      
      while (rep--)
      {
        if (no_end_nl && !rep) --len;
        unused = write (1, buf, len+1);
      }
      free (buf);
    }
    else if (strcmp (token, "random1") == 0)
    {
      token = strtok (NULL, ":");
      if (token)
      {
        rep = atoi (token);
        token = strtok (NULL, ":");
        
        if (token) len = atoi (token);
      }
      
      char* buf = (char*) malloc (len+1);
      for (int k=0; k<len; ++k)
        buf[k] = '!' + (rand() % 93);
      buf[len] = NL;
      
      while (rep--) {
        if (no_end_nl && !rep) --len;
        unused = write (1, buf, len+1);
      }
      free (buf);
    }
    else if (strcmp (token, "random") == 0)
    {
      token = strtok (NULL, ":");
      if (token)
      {
        rep = atoi (token);
        token = strtok (NULL, ":");
        
        if (token) len = atoi (token);
      }
      
      char* buf = (char*) malloc (len+1);
      buf[len] = NL;

      while (rep--)
      {
        for (int k=0; k<len; ++k)
          buf[k] = '!' + (rand() % 93);
      
        if (no_end_nl && !rep) --len;
        unused = write (1, buf, len+1);
      }
      free (buf);
    }
  }
  /* making gcc and clang happy */
  unusd = unused;
  unused = unusd;
  
  return 0;
}

/* vi: ts=8 et sts=2 sw=2 ai si cin :
 */

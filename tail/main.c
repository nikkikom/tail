/*
 *  main.c
 *  Tail
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

#include <sys/types.h>
#include <sys/stat.h>

#include <getopt.h>
#include <unistd.h> /* write, getopt  */
#include <stdio.h>  /* printf, perror */
#include <fcntl.h>  /* open           */
#include <errno.h>  /* errno          */
#include <ctype.h>  /* isprint        */
#include <limits.h> /* UINT_MAX       */
#include <errno.h>  /* errno          */
#include <string.h> /* strcmp         */

#include "tail.h"

static const char* file     = "-";
char               eol_char = '\n';
static int         no_mmap  = 0;
static int         no_pipe  = 0;
static int         no_read  = 0;
static unsigned    lines    = 5;

static const char esc_chars[11] = "0abfnrtv\\'\"";
static const char esc_subs[11]  = "\0\a\b\f\n\r\t\v\\'\"";

static void usage (int argc, char* argv[])
{
  printf ("Usage: tail [-e EOL] [-m pipe|read|mmap] [-n <L>] file\n");
  printf ("options:\n");
  printf (" -n L        output no more than L last lines (default 5)\n");
  printf (" -e CHAR     end-of-line char (default '\\n')\n");
  printf (" -m METHOD   use specified method to process the file\n");
  printf ("    pipe     read file from the begining to find trailing lines\n");
  printf ("    read     read file from end, using read and seek\n");
  printf ("    mmap     read file from end, using mmap(2)\n");
  printf (" file        file name or '-' for stdin\n");
  printf ("\n");
}

int is_eol (char ch)
{
  return ch == eol_char;
}

/* helper, used in tail_xxxx */
ssize_t write_all (int fd, const void *buf, size_t nbyte)
{
  size_t bytes_left;
  const char* cbuf = (const char*) buf;
  
  /* check args */
  if (fd < 0 && NULL == cbuf)
  {
    errno = EINVAL;
    return -1;
  }
  
  for (bytes_left = nbyte; bytes_left > 0;)
  {
    ssize_t transferred;
    if ((transferred = write (fd, cbuf, bytes_left)) < 0)
      return -1;
    
    /* partial write */
    bytes_left -= transferred;
    cbuf += transferred;
  }
  
  return nbyte;
}

static int parse_args (int argc, char *argv[])
{
  unsigned long tmp;
  char *endptr;
  int c;
  
  while ((c = getopt (argc, argv, "n:e:m:")) != -1)
    switch (c)
    {
      case 'n':
        tmp = strtoul (optarg, &endptr, 10);
        if (!endptr || *endptr != '\0' || tmp >= UINT_MAX)
        {
          fprintf (stderr, "Bad number for '-n' argument: '%s'\n", optarg);
          return 0;
        }
        lines = (unsigned) tmp;
        break;
        
      case 'e':
        if (optarg[0] == '\0' ||
            !(optarg[0] == '\\' && optarg[1] != '\0') ||
            (optarg[1] != '\0' && optarg[2] != '\0'))
        {
          fprintf (stderr, "Bad char for '-e' argument: '%s'\n", optarg);
          return 0;
        }
        
        if (optarg[0] == '\\' && optarg[1] != '\0')
        {
          int i;
          for (i = 0; i < sizeof (esc_chars); ++i)
            if (optarg[1] == esc_chars[i])
            {
              eol_char = esc_subs[i];
              break;
            }
          
          if (i >= sizeof (esc_chars))
          {
            fprintf (stderr, "Bad esc-char for '-e' argument: '%s'\n", optarg);
            return 0;
          }
        }
        else
        {
          eol_char = optarg[0];
        }
        break;
        
      case 'm':
        if      (! strcmp (optarg, "mmap")) no_pipe = no_read = 1;
        else if (! strcmp (optarg, "read")) no_pipe = no_mmap = 1;
        else if (! strcmp (optarg, "pipe")) no_mmap = no_read = 1;
        else {
          fprintf (stderr, "Unknown read method '%s', use mmap, read or pipe\n",
                   optarg);
          return 0;
        }
        
        break;
        
      case '?':
        if (optopt == 'n')
          fprintf (stderr, "Option -%c requires an argument\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        
        return 0;
        
      default:
        abort ();
    }
  
  if (optind < argc)
    file = argv[optind++];

  if (optind < argc)
  {
    fprintf (stderr, "Too many non-option arguments:");
    do fprintf (stderr, " %s", argv[optind]); while (++optind < argc);
    fputc ('\n', stderr);
    return 0;
  }
  
  return 1;
}

int main(int argc, char *argv[])
{
  struct stat st;
  int fd = 0; /* set to stdin by default */
  int rc = 0;
  
  if (!parse_args (argc, argv))
  {
    usage (argc, argv);
    return 1;
  }
  
  if (file[0] != '-' || file[1] != '\0')
    fd = open (file, O_RDONLY);
  else
    file = "<stdin>";

  
  if (fd < 0)
  {
    perror (file);
    return 1;
  }
  
  /* check the file size */
  if (fstat (fd, &st) < 0)
  {
    perror ("fstat");
    return -1;
  }
  
  if (S_ISFIFO(st.st_mode) || ! st.st_size)
  {
    /* the file does not support seeks (because it is probably a pipe)
     * tail_pipe - is the only supported method then.
     */
    no_mmap = no_read = 1;
  }
  
  if (no_mmap && no_read && no_pipe)
  {
    fprintf (stderr,
        "warning: no available read methods, switching to fallback (pipe)\n");
    no_pipe = 0;
  }
  
  rc = TAIL_NOTSUP;
  
  if ((no_mmap ||
        (rc = tail_mmap (file, fd, 1, lines, 0, st.st_size)) == TAIL_NOTSUP) &&
      (no_read ||
        (rc = tail_read (file, fd, 1, lines, 0, st.st_size)) == TAIL_NOTSUP) &&
      (no_pipe ||
        (rc = tail_pipe (file, fd, 1, lines, 0, st.st_size)) == TAIL_NOTSUP)
  )
  {
    fprintf (stderr, "error: %s - no available read methods\n", file);
  }
  
  return rc;
}

/* vi: ts=8 et sts=2 sw=2 ai si cin :
 */

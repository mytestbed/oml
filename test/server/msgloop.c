#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

enum TestType {
  TT_SERVER,
  TT_PROXY
};

/* ------------------------------>   |Seqno ||Length| */
const char header_prototype [] = "OML0123ABCD0123ABCD";
#define HEADER_LENGTH (sizeof (header_prototype) - 1)

int
hex_to_int (char a)
{
  int digit = a - '0';
  int letter = toupper (a) - 'A' + 0xA;

  if (letter >= 0xA)
    return letter;
  else if (digit >= 0)
    return digit;
  else
    fprintf (stderr, "'%c' (%d) is not a hex digit\n", a, a);

  return 0;
}

int
read_ascii_hex_int (char *buf, int nybbles)
{
  int i = 0;
  uint32_t value = 0;
  for (i = 0; i < nybbles; i++) {
    value <<= 4;
    value |= hex_to_int (buf[i]);
  }

  return value;
}

int
read_message (char **line, size_t *size, size_t *length_out, int *seqno_out)
{
  int length = 0;
  int seqno = 0; // Message sequence number
  int count = 0;
  int result = 0;
  char header[HEADER_LENGTH+1] = {0};
  char *buf;

  bzero (*line, *size);


  do {
    result = read (0, header + count, HEADER_LENGTH - count);
    if (result == -1) {
      fprintf (stderr, "reading header:  %s\n", strerror (errno));
      return -1;
    } else if (result == 0) {
      fprintf (stderr, "Read no bytes! Pipe closed?\n");
      return 0;
    }

    count += result;
  } while (count < HEADER_LENGTH);
  count = 0;

  if (result < HEADER_LENGTH) {
    fprintf (stderr, "Couldn't read the whole header length at once!\n");
    fprintf (stderr, "%d: %s\n", result, header);
    return -1;
  }

  if (strncmp ("OML", header, 3) != 0) {
    fprintf (stderr, "Packet format error (not OML)\n");
    return -1;
  }

  seqno = read_ascii_hex_int  (&header[3], 8);
  length = read_ascii_hex_int (&header[11], 8);

  if (*size < length) {
    char *new = realloc (*line, length);
    if (new == NULL) {
      fprintf (stderr, "realloc() error! %s\n", strerror (errno));
      return -1;
    }
    *line = new;
    *size = length;
  }

  char *p = *line;
  do {
    result = read (0, p + count, length - count);
    //    fprintf (stderr, "%s\n", p);
    if (result == -1) {
      fprintf (stderr, "Error reading message payload: %s\n", strerror (errno));
      return -1;
    } else if (result == 0) {
      fprintf (stderr, "Read no bytes! Pipe closed?\n");
      return 0;
    }

    count += result;
  } while (count < length);

  *length_out = length;
  *seqno_out = seqno;
  return 1;
}

int
main (int argc, char **argv)
{
  int test = TT_PROXY;
  size_t length = 1024;
  char *line = NULL;
  int result = 0;
  int n = 0;

  setlinebuf (stdout);

  if (argc > 1) {
    if (strcmp (argv[0], "--proxy") == 0) {
      test = TT_PROXY;
    } else if (strcmp (argv[0], "--server") == 0) {
      test == TT_SERVER;
    }
  }

  line = malloc (length);
  if (line == NULL) {
    fprintf (stderr, "malloc():  %s\n", strerror (errno));
    exit (1);
  }

  do {
    int seqno = 0;
    size_t msg_length = 0;
    result = read_message (&line, &length, &msg_length, &seqno);
    n++;
    printf ("%d, %d bytes\n", seqno, msg_length);
    if (result == -1) {
      fprintf (stderr, "read_message() failed\n");
      exit (1);
    } else if (result == 0) {
      break;
    }

    printf ("%04d:--->'%s'\n", seqno, line);
  } while (result == 1);

  printf ("Exiting\n");
  fclose (stdin);
  fclose (stdout);
  fclose (stderr);
  return 0;
}

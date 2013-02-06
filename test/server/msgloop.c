/*
 * Copyright 2010-2013 National ICT Australia (NICTA), Australia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "ocomm/o_log.h"
#include "proxy_client.h"

/* From proxy_server/oml2-proxy-server.c */
void proxy_message_loop (const char *client_id, Client *client, void *buf, size_t size);

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

  bzero (*line, *size);

  do {
    result = read (0, header + count, HEADER_LENGTH - count);
    if (result == -1) {
      fprintf (stderr, "reading header:  %s\n", strerror (errno));
      return -1;
    } else if (result == 0) {
      fprintf (stderr, "\n# msgloop: no more data, assuming closed pipe\n");
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
  Client *client = NULL;

  setlinebuf (stdout);
  o_set_log_file ("log.txt");
  o_set_log_level (O_LOG_DEBUG);

  if (argc > 1) {
    if (strcmp (argv[0], "--proxy") == 0) {
      test = TT_PROXY;
    } else if (strcmp (argv[0], "--server") == 0) {
      test = TT_SERVER;
    }
  }

  line = malloc (length);
  if (line == NULL) {
    fprintf (stderr, "malloc():  %s\n", strerror (errno));
    exit (1);
  }

  client = client_new (NULL, 4096, "dummy.bin", 0, "dummy.com");


  fprintf (stderr, "# msgloop: receiving test data...");
  do {
    int seqno = 0;
    size_t msg_length = 0;
    result = read_message (&line, &length, &msg_length, &seqno);
    n++;
    if (result == -1) {
      fprintf (stderr, "read_message() failed\n");
      exit (1);
    } else if (result == 0) {
      break;
    }

    switch (test) {
    case TT_PROXY:
      proxy_message_loop ("client", client, line, msg_length);
      break;
    case TT_SERVER:
    default:
      break;
    }

    fprintf (stderr, " %d", n);
  } while (result == 1);

  fprintf (stderr, "# msgloop: sending interpreted headers...\n");

  struct header *header = client->headers;
  while (header) {
    printf ("H>%s>%s\n", tag_to_string (header->tag),
            header->value);
    header = header->next;
  }

  fprintf (stderr, "# msgloop: sending %d interpreted messages...\n", (int)client->messages->length);
  while (client->messages->length > 0) {
    struct msg_queue_node *head = msg_queue_head (client->messages);
    struct cbuffer_cursor *cursor = &head->cursor;
    size_t length = head->msg->length;
    printf ("T>");
    while (length > 0) {
      char *buf = cbuf_cursor_pointer (cursor);
      size_t page_remaining = cbuf_cursor_page_remaining (cursor);
      size_t to_write = length < page_remaining ? length : page_remaining;
      fwrite (buf, 1, to_write, stdout);
      cbuf_consume_cursor (cursor, to_write);
      length -= to_write;
    }
    msg_queue_remove (client->messages);
  }
  fflush (stdout);
  fclose (stdout);
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

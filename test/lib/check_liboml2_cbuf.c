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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <check.h>

#include "cbuf.h"
#include "check_util.h"

#if 0
START_TEST (test_mbuf_create)
{
  void* buf = malloc (4096);
  if (buf == NULL)
    fail ("malloc(3) failed, so mbuf_create() might fail too\n");

  free (buf);

  MBuffer* mbuf = mbuf_create ();

  fail_if (mbuf == NULL);
  fail_if (mbuf->base == NULL);
  fail_if (mbuf->rdptr != mbuf->base);
  fail_if (mbuf->wrptr != mbuf->base);
  fail_if (mbuf->fill != 0);
  fail_unless (mbuf->length > 0);
  fail_if (mbuf->wr_remaining != mbuf->length);
  fail_if ((int)mbuf->rd_remaining != (mbuf->wrptr - mbuf->rdptr));
  unsigned int i;
  for (i = 0; i < mbuf->length; i++)
    fail_if (mbuf->base[i] != 0);
}
END_TEST


START_TEST (test_mbuf_buffer)
{
  MBuffer* mbuf = mbuf_create ();
  fail_if (mbuf_buffer (mbuf) != mbuf->base);
}
END_TEST

START_TEST (test_mbuf_length)
{
  MBuffer* mbuf = mbuf_create ();
  fail_if (mbuf_length (mbuf) != mbuf->length);
}
END_TEST

START_TEST (test_mbuf_resize)
{
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  size_t length_1 = mbuf_length (mbuf);

  result = mbuf_resize (mbuf, length_1 / 2);

  fail_if (result == -1);
  fail_if (mbuf->base == NULL);
  fail_if (mbuf_length (mbuf) != length_1); // Don't resize if new length is less

  size_t length_2 = length_1 + 1;

  result = mbuf_resize (mbuf, length_2);

  fail_if (result == -1);
  fail_if (mbuf->base == NULL);
  fail_if (mbuf_length (mbuf) != length_2);

  size_t length_3 = 2 * length_1;

  result = mbuf_resize (mbuf, length_3);

  fail_if (result == -1);
  fail_if (mbuf->base == NULL);
  fail_if (mbuf_length (mbuf) != length_3);
}
END_TEST

START_TEST (test_mbuf_resize_contents)
{
  uint8_t s[8192];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  size_t length_1 = mbuf_length (mbuf);

  memset (s, 0, sizeof (s));
  unsigned int i = 0;
  for (i = 0; i < length_1; i++)
    {
      s[i] = 'A' + (i % 16);
    }

  memcpy (mbuf->base, s, length_1);
  result = mbuf_resize (mbuf, length_1 / 2);

}
END_TEST

static const char* test_strings [] =
  {
    "",
    "a",
    "ab",
    "abc",
    "abcd",
    "abcde",
    "abcdef",
    "0123456789ABCDEF",
    "wxyz",
    "xyz",
    "yz",
    "z",
    "",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
    "0123456789ABCDEF",
  };

START_TEST (test_mbuf_write)
{
  char s[8192];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  size_t n = 0;
  size_t length = mbuf_length (mbuf);
  size_t len = strlen ((char*)test_strings[0]);
  memset (s, 0, sizeof (s));

  while (n + len < length && i < LENGTH (test_strings))
    {
      strcat (s, test_strings[i]);
      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], len);
      n += len;

      fail_if (result == -1);
      fail_if (mbuf->fill != n);
      fail_if (mbuf->wrptr - mbuf->base != (int)n);
      fail_if (mbuf->rdptr != mbuf->base);
      fail_if (mbuf->wr_remaining != length - n);
      fail_if (mbuf->rd_remaining != n);
      fail_if (mbuf->wrptr < mbuf->rdptr);
      fail_if (strncmp ((char*)mbuf->base, s, n) != 0,
               "Expected:\n%s\nActual:\n%s\n", s, mbuf->base);

      i++;
      if (i < LENGTH (test_strings))
        len = strlen (test_strings[i]);
    }

  /* Now do it again and try to overrun the buffer */
  i = 0;
  int done = 0;
  len = strlen (test_strings[i]);
  while (i < LENGTH (test_strings) && !done)
    {
      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], len);

      if (result != -1)
        {
          strcat (s, test_strings[i]);
          n += len;

          i++;
          if (i < LENGTH (test_strings))
            len = strlen (test_strings[i]);
        }
      else
        {
          done = 1;
        }
    }

  // FIXME:  semantics changed:  mbuf_write() now resizes buffer as needed.
  //  fail_unless (done == 1); // Fail if we didn't overflow the buf

  fail_unless (strncmp ((char*)mbuf->base, s, n) == 0,
               "Expected:\n%s\nActual:\n%s\n", s, mbuf->base);

  // Check that we didn't write any bytes the last mbuf_write()
  //  for (i = n; i < length; i++)
  //    fail_unless (mbuf->base[i] == 0);

  fail_if (mbuf->fill != n);
  fail_if (mbuf->wrptr - mbuf->base != (int)n);
  fail_if (mbuf->rdptr != mbuf->base);
  //fail_if (mbuf->wr_remaining != length - n);
  fail_if (mbuf->rd_remaining != n);
  fail_if (mbuf->wrptr < mbuf->rdptr);
}
END_TEST

START_TEST (test_mbuf_write_null)
{
  char s[] = "abc";
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  memset (s, 0, sizeof (s));

  result = mbuf_write (NULL, (uint8_t*)s, 1);
  fail_if (result != -1);

  result = mbuf_write (mbuf, NULL, 1);
  fail_if (result != -1);
  fail_if (mbuf->wrptr != mbuf->base);
  fail_if (mbuf->fill != 0);
  fail_if (mbuf->wr_remaining != mbuf->length);
}
END_TEST

START_TEST (test_mbuf_read)
{
  char s[8192];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  size_t n = 0;
  size_t rdstart = 0;
  size_t length = mbuf_length (mbuf);
  memset (s, 0, sizeof (s));

  result = mbuf_read (mbuf, (uint8_t*)s, 100);

  fail_if (result != -1, "Trying to read from an empty buffer did not fail!\n");

  for (i = 0; i < LENGTH (s); i++)
    s[i] = 'A' + (i % 16);

  // Fill the buffer
  result = mbuf_write (mbuf, (uint8_t*)s, length);
  fail_if (result == -1, "Trying to write test data into the buffer\n");
  memset (s, 0, LENGTH(s));

  result = mbuf_read (mbuf, (uint8_t*)s, 0);
  fail_if (result == -1, "Trying to read 0 bytes\n");
  fail_if (s[0] != 0);
  fail_if (mbuf->rdptr != mbuf->base);
  fail_if (mbuf->rd_remaining != mbuf->fill);

  size_t old_remaining = mbuf->rd_remaining;
  size_t remaining = mbuf->rd_remaining;
  while (remaining)
    {
      memset (s, 0, LENGTH (s));
      int rdlen = remaining * (((double)rand())/RAND_MAX) + 1;

      result = mbuf_read (mbuf, (uint8_t*)s, rdlen);
      fail_if (result == -1, "Unabled to read %d bytes from mbuf\n", rdlen);

      n += rdlen;
      fail_if (strncmp (s, (char*)&mbuf->base[rdstart], rdlen) != 0);
      rdstart += rdlen;
      fail_if (remaining > old_remaining);
      old_remaining = remaining;
      remaining = mbuf->rd_remaining;
      fail_if (mbuf->rdptr > mbuf->wrptr);
    }

  fail_if (n != length);
  fail_if (mbuf->rd_remaining != 0);
  fail_if (mbuf->rdptr - mbuf->base != (int)mbuf->length);
  fail_if (mbuf->wrptr != mbuf->rdptr);
  fail_if (mbuf->msgptr != mbuf->base);
  fail_if (mbuf->wr_remaining != 0);
}
END_TEST

START_TEST (test_mbuf_read_null)
{
  char s[8192];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  size_t length = mbuf_length (mbuf);
  memset (s, 0, sizeof (s));

  for (i = 0; i < LENGTH (s); i++)
    s[i] = 'A' + (i % 16);

  // Fill the buffer
  result = mbuf_write (mbuf, (uint8_t*)s, length);
  fail_if (result == -1, "Trying to write test data into the buffer\n");

  result = mbuf_read (mbuf, NULL, 1);
  fail_if (result != -1);
  fail_if (mbuf->rdptr != mbuf->base);
  fail_if (mbuf->rd_remaining != mbuf->fill);

  result = mbuf_read (NULL, (uint8_t*)s, 1);
  fail_if (result != -1);
}
END_TEST

START_TEST (test_mbuf_begin_read)
{
  char s[8192];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  memset (s, 0, sizeof (s));

  for (i = 0; i < LENGTH (test_strings); i++)
    {
      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], strlen (test_strings[i]));
      fail_if (result == -1);
    }

  for (i = 0; i < LENGTH (test_strings); i++)
    {
      memset (s, 0, LENGTH (s));
      result = mbuf_read (mbuf, (uint8_t*)s, strlen (test_strings[i]));
      fail_if (result == -1);

      result = mbuf_begin_read (mbuf);
      fail_if (result == -1);
      fail_if (mbuf->msgptr != mbuf->rdptr);
      fail_if (strncmp (s, test_strings[i], strlen (test_strings[i])) != 0);
    }
}
END_TEST

START_TEST (test_mbuf_begin_write)
{
  char s[8192];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  memset (s, 0, sizeof (s));

  for (i = 0; i < LENGTH (test_strings); i++)
    {
      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], strlen (test_strings[i]));
      fail_if (result == -1);

      result = mbuf_begin_write (mbuf);
      fail_if (result == -1);
      fail_if (mbuf->msgptr != mbuf->wrptr);
    }
}
END_TEST

START_TEST (test_mbuf_reset_read)
{
  char s[4096];
  char t[4096];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  size_t n = 0;
  memset (s, 0, sizeof (s));
  memset (t, 0, sizeof (t));

  for (i = 0; i < LENGTH (test_strings[i]); i++)
    {
      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], strlen (test_strings[i]));
      fail_if (result == -1);
    }

  result = mbuf_begin_read (mbuf);
  fail_if (result == -1);
  fail_if (mbuf->msgptr != mbuf->base);
  fail_if (mbuf->rdptr != mbuf->base);

  for (i = 0; i < LENGTH (test_strings[i]); i++)
    {
      uint8_t* old_msgptr = mbuf->msgptr;
      memset (s, 0, LENGTH(s));
      strcat (t, test_strings[i]);
      n += strlen (test_strings[i]);

      result = mbuf_read (mbuf, (uint8_t*)s, n);
      fail_if (result == -1);

      fail_if (mbuf->rdptr - mbuf->msgptr != (int)n,
               "\nRead(%d) of string %s (%d) gives rdptr-start = %d (rdptr=%d, start=%d)\n",
               i,
               test_strings[i], strlen(test_strings[i]),
               mbuf->rdptr - mbuf->msgptr,
               mbuf->rdptr - mbuf->base,
               mbuf->msgptr - mbuf->base);
      fail_if (strncmp (s, t, n) != 0);

      result = mbuf_reset_read (mbuf);
      fail_if (result == -1);
      fail_if (mbuf->msgptr != old_msgptr);
      fail_if (mbuf->rdptr != mbuf->msgptr);
    }
}
END_TEST

START_TEST (test_mbuf_reset_write)
{
  char s[4096];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  memset (s, 0, sizeof (s));

  for (i = 0; i < LENGTH(test_strings); i++)
    {
      uint8_t* old_msgptr = mbuf->msgptr;
      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], strlen (test_strings[i]));

      fail_if (result == -1);
      fail_if (mbuf->wrptr < mbuf->msgptr);
      fail_if (mbuf->wrptr < mbuf->rdptr);
      fail_if (strncmp ((char*)mbuf->base, test_strings[i], strlen (test_strings[i])) != 0);

      result = mbuf_reset_write (mbuf);

      fail_if (result == -1);
      fail_if (mbuf->msgptr != old_msgptr);
      fail_if (mbuf->wrptr != mbuf->msgptr);
      fail_if (mbuf->wrptr != mbuf->base);
    }
}
END_TEST

START_TEST (test_mbuf_consume_message)
{
  char s[4096];
  char t[4096];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  memset (s, 0, sizeof (s));
  memset (t, 0, sizeof (t));

  for (i = 0; i < LENGTH (test_strings); i++)
    {
      memset (s, 0, LENGTH (s));
      uint8_t* old_msgptr = mbuf->msgptr;
      size_t msglen = strlen (test_strings[i]);

      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], msglen);
      fail_if (result == -1);
      fail_if (mbuf->wrptr < mbuf->msgptr);
      fail_if (mbuf->wrptr < mbuf->rdptr);
      fail_if (mbuf->wrptr - mbuf->rdptr != (int)msglen);

      result = mbuf_read (mbuf, (uint8_t*)s, msglen);
      fail_if (result == -1);
      fail_if (mbuf->rdptr != mbuf->wrptr);
      fail_if (mbuf->msgptr != old_msgptr);
      fail_if (strcmp (s, test_strings[i]) != 0);

      result = mbuf_reset_read (mbuf);
      fail_if (result == -1);
      fail_if (mbuf->msgptr != old_msgptr);
      fail_if (mbuf->rdptr != mbuf->msgptr);

      result = mbuf_read (mbuf, (uint8_t*)s, msglen);
      fail_if (result == -1);
      fail_if (mbuf->rdptr != mbuf->wrptr);
      fail_if (mbuf->msgptr != old_msgptr);
      fail_if (strcmp (s, test_strings[i]) != 0);

      result = mbuf_consume_message (mbuf);
      fail_if (result == -1);
      fail_if (mbuf->msgptr - old_msgptr != (int)msglen);
      fail_if (mbuf->wrptr != mbuf->rdptr);
      fail_if (mbuf->msgptr != mbuf->rdptr);
    }
}
END_TEST

START_TEST (test_mbuf_repack)
{
  char s[4096];
  char t[4096];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  size_t n = 0;
  size_t m = 0;
  memset (s, 0, sizeof (s));
  memset (t, 0, sizeof (t));

  size_t n_test_strings = LENGTH (test_strings);
  size_t consumed = n_test_strings / 2;

  // FIXME: this is a weak test.

  for (i = 0; i < n_test_strings; i++)
    {
      if (i < consumed)
        n += strlen (test_strings[i]);
      else
        {
          m += strlen (test_strings[i]);
          strcat (s, test_strings[i]);
        }

      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], strlen (test_strings[i]));
      fail_if (result == -1);
    }

  result = mbuf_begin_read (mbuf);
  fail_if (result == -1);

  for (i = 0; i < consumed; i++)
    {
      result = mbuf_read (mbuf, (uint8_t*)t, strlen (test_strings[i]));
      fail_if (result == -1);
    }

  fail_unless (mbuf->rdptr - mbuf->base == (int)n);
  fail_unless (mbuf->msgptr == mbuf->base);

  result = mbuf_repack (mbuf);
  fail_if (mbuf->rdptr != mbuf->base);
  fail_if (mbuf->msgptr != mbuf->base);
  fail_if (mbuf->wrptr != mbuf->base + mbuf->fill);
  fail_if (mbuf->fill != mbuf->rd_remaining);
  fail_if (mbuf->fill != m);
  fail_if (strncmp ((char*)mbuf->base, s, mbuf->fill) != 0);
}
END_TEST

START_TEST (test_mbuf_repack_message)
{
  char s[4096];
  char t[4096];
  MBuffer* mbuf = mbuf_create ();
  int result = 0;
  unsigned int i = 0;
  size_t n = 0;
  size_t m = 0;
  memset (s, 0, sizeof (s));
  memset (t, 0, sizeof (t));

  size_t n_test_strings = LENGTH (test_strings);
  size_t consumed = n_test_strings / 2;

  // FIXME: this is a weak test.

  for (i = 0; i < n_test_strings; i++)
    {
      if (i < consumed - 1) // consumed - 1 is intentional
        n += strlen (test_strings[i]);
      else
        {
          m += strlen (test_strings[i]);
          strcat (s, test_strings[i]);
        }

      result = mbuf_write (mbuf, (uint8_t*)test_strings[i], strlen (test_strings[i]));
      fail_if (result == -1);
    }

  result = mbuf_begin_read (mbuf);
  fail_if (result == -1);

  for (i = 0; i < consumed - 1; i++)  // consumed - 1 is intentional
    {
      result = mbuf_read (mbuf, (uint8_t*)t, strlen (test_strings[i]));
      fail_if (result == -1);
    }

  // Set the msgptr equal to the current rdptr
  result = mbuf_consume_message (mbuf);
  fail_if (result == -1);
  fail_unless (mbuf->rdptr - mbuf->base == (int)n);
  fail_unless (mbuf->msgptr == mbuf->rdptr);

  // Make the rdptr different to the msgptr by reading one more string
  result = mbuf_read (mbuf, (uint8_t*)t, strlen (test_strings[i]));
  fail_if (result == -1);
  fail_unless (mbuf->msgptr < mbuf->rdptr);

  size_t old_rd_offset = mbuf->rdptr - mbuf->msgptr;
  size_t old_rd_remaining = mbuf->rd_remaining;

  result = mbuf_repack_message (mbuf);
  fail_if (mbuf->msgptr != mbuf->base);
  fail_if (mbuf->rdptr != mbuf->msgptr + old_rd_offset);
  fail_if (mbuf->wrptr != mbuf->base + mbuf->fill);
  fail_if (mbuf->rd_remaining != old_rd_remaining);
  fail_if (mbuf->fill != m);
  fail_if (strncmp ((char*)mbuf->base, s, mbuf->fill) != 0);
}
END_TEST

#endif

START_TEST (test_cbuf_create)
{
  CBuffer *cbuf = cbuf_create (-1);
  char * test_string = "abcdefg";
  size_t len = strlen (test_string);
  int i = 0;

  for (i = 0; i < 1000; i++) {
    cbuf_write (cbuf, "abcdefg", len);
  }


}
END_TEST

Suite*
cbuf_suite (void)
{
  Suite* s = suite_create ("Cbuf");

  /* Cbuf test cases */
  TCase* tc_cbuf = tcase_create ("Cbuf");

  /* Add tests to "Mbuf" */
  tcase_add_test (tc_cbuf, test_cbuf_create);

  suite_add_tcase (s, tc_cbuf);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

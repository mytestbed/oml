/*
 * Copyright 2007-2008 National ICT Australia (NICTA), Australia
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
/*!\file mt_queue.c
  \brief testing the thread-safe queue
*/

#include <popt.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <assert.h>

#include "ocomm/mt_queue.h"

// Slow producer
slow_producer(
  MTQueue* q
) {
  int i;

  for (i = 0; i < 5; i++) {
    char* s = malloc(16);
    sprintf(s, "token_%i", i);
    printf("sp: Adding '%s'\n", s);
    mt_queue_add(q, s);
    sleep(1);
  }
}

fast_consumer(
  MTQueue* q,
  int samples
) {
  int i;

  for (i = 0; i < samples; i++) {
    void* res;
    res = mt_queue_remove(q);
    printf("fc: Removing '%s'\n", res);
  }
}

void
test1()

{
  MTQueue* q;
  pthread_t  thread;

  puts("-- TEST 1 --");
  q = mt_queue_new("Q", 3);
  pthread_create(&thread, NULL, slow_producer, (void*)q);
  fast_consumer(q, 5);
}

// Slow producer
fast_producer(
  MTQueue* q
) {
  int i;

  for (i = 0; i < 5; i++) {
    char* s = malloc(16);
    sprintf(s, "token_%i", i);
    printf("fp: Starting to add '%s'\n", s);
    mt_queue_add(q, s);
    printf("fp: Done adding '%s'\n", s);
  }
}

slow_consumer(
  MTQueue* q,
  int samples
) {
  int i;

  for (i = 0; i < samples; i++) {
    void* res;
    res = mt_queue_remove(q);
    printf("sc: Removing '%s'\n", res);
    sleep(1);
  }
}

void
test2()

{
  MTQueue* q;
  pthread_t  thread;

  puts("-- TEST 2 --\n");
  q = mt_queue_new("Q", 3);
  pthread_create(&thread, NULL, fast_producer, (void*)q);
  slow_consumer(q, 5);
}

int
main(
  int argc,
  const char *argv[]
) {
  test1();
  test2();
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/

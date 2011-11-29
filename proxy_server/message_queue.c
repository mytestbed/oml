/*
 * Copyright 2010-2011 National ICT Australia (NICTA), Australia
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
#include <assert.h>
#include <message.h>
#include <cbuf.h>
#include <mem.h>
#include "message_queue.h"

struct msg_queue*
msg_queue_create (void)
{
  struct msg_queue *q = xmalloc (sizeof (struct msg_queue));
  if (q == NULL)
    return NULL;

  q->tail = NULL;

  return q;
}

void
msg_queue_destroy (struct msg_queue *queue)
{
  while (queue->length > 0)
    msg_queue_remove (queue);
  xfree (queue);
}

/**
 * Create a new node at the end of the queue and return a pointer to it.
 * This operation is O(1).
 *
 */
struct msg_queue_node*
msg_queue_add (struct msg_queue *queue)
{
  if (queue == NULL)
    return NULL;

  struct msg_queue_node *node = xmalloc (sizeof (struct msg_queue_node));
  if (node == NULL)
    return NULL;

  node->next = NULL;
  if (queue->tail == NULL) {
    queue->tail = node;
    queue->tail->next = node;
  } else {
    node->next = queue->tail->next;
    queue->tail->next = node;
    queue->tail = node;
  }

  queue->length++;
  return node;
}

/**
 * Return a pointer to the head of the queue (next node to be
 * processed).  This operation is O(1).
 *
 */
struct msg_queue_node*
msg_queue_head (struct msg_queue *queue)
{
  if (queue == NULL || queue->tail == NULL)
    return NULL;
  return queue->tail->next;
}

/**
 * Remove the node at the head of the queue.  This operation is O(1).
 */
void
msg_queue_remove (struct msg_queue *queue)
{
  if (queue == NULL || queue->tail == NULL)
    return;

  struct msg_queue_node *head = queue->tail->next;

  assert (head != NULL);

  /* Unlink the head */
  queue->length--;
  if (queue->length == 0)
    queue->tail = NULL;
  else
    queue->tail->next = head->next;

  xfree (head);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

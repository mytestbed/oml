/*
 * Copyright 2010-2016 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file message_queue.c
 * \brief Implement a FIFO message queue containing CBuffer objects.
 */
#include <assert.h>

#include "mem.h"
#include "cbuf.h"
#include "message.h"
#include "message_queue.h"

struct msg_queue*
msg_queue_create (void)
{
  struct msg_queue *q = oml_malloc (sizeof (struct msg_queue));
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
  oml_free (queue);
}

/** Create a new node at the end of the queue and return a pointer to it.
 * This operation is O(1).
 */
struct msg_queue_node*
msg_queue_add (struct msg_queue *queue)
{
  if (queue == NULL)
    return NULL;

  struct msg_queue_node *node = oml_malloc (sizeof (struct msg_queue_node));
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

/** Return a pointer to the head of the queue (next node to be processed).
 *
 * This operation is O(1).
 */
struct msg_queue_node*
msg_queue_head (struct msg_queue *queue)
{
  if (queue == NULL || queue->tail == NULL)
    return NULL;
  return queue->tail->next;
}

/** Remove the node at the head of the queue.  This operation is O(1).
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

  oml_free (head);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

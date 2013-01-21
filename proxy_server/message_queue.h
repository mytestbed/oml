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
#ifndef MESSAGE_QUEUE_H__
#define MESSAGE_QUEUE_H__

#include <message.h>
#include <cbuf.h>

struct msg_queue_node {
  struct oml_message *msg;
  struct cbuffer_cursor cursor;
  struct msg_queue_node *next;
};

struct msg_queue {
  size_t length; /* Number of nodes */
  struct msg_queue_node *tail;
};


struct msg_queue* msg_queue_create (void);
void msg_queue_destroy (struct msg_queue *queue);
struct msg_queue_node* msg_queue_add (struct msg_queue *queue);
struct msg_queue_node* msg_queue_head (struct msg_queue *queue);
void msg_queue_remove (struct msg_queue *queue);

#endif /* MESSAGE_QUEUE_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

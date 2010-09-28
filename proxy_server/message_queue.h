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

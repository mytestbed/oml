/*
 * Copyright 2007-2015 National ICT Australia (NICTA), Australia
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
/** \file client_handler.h
 * \brief Interface for the ClientHandler.
 */
#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_

#include <time.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>
#include <oml2/oml_writer.h>
#include <mbuf.h>

#include "database.h"

#define MAX_PROTOCOL_VERSION OML_PROTOCOL_VERSION
#define MIN_PROTOCOL_VERSION 1

/** Enum representing the current state of a ClientHandler */
typedef enum {
  CS_HEADER = 0,     /**< processing header info */
  CS_DATA,           /**< receiving data */
  CS_PROTOCOL_ERROR, /**< a protocol error occurred */
} CState;

/** Enum representing the current OMSP mode of a ClientHandler */
typedef enum {
  CM_UNSPEC_DATA = 0,  /**< data format unknown yet*/
  CM_TEXT_DATA,        /**< data in text format */
  CM_BINARY_DATA,      /**< data in binary format */
} CMode;

#define DEF_NUM_VALUES  30
#define MAX_STRING_SIZE 64

/** Structure to hold the state of each connected client and associated socket, event and database */
typedef struct ClientHandler {
  char name[MAX_STRING_SIZE];       /**< Name used for debugging */

  Database*   database;             /**< Handler to the database object for this client */
  DbTable**   tables;               /**< Array of the tables for that client, indexed by MS id */
  int*        seqno_offsets;        /**< Array of the current sequence numbers for each MS, indexed by MS id \bug: not currently used */
  OmlValue**  values_vectors;       /**< Array of arrays of OmlValue to hold a new sample for each MS, indexed by MS id */
  int*        values_vector_counts; /**< Array of sizes of each vector in values_vectors, indexed by MS id */
  int         table_count;          /**< Size of the tables, seqno_offsets and values_vectors arrays */
  int         sender_id;            /**< ID of this sender in the database*/
  char*       sender_name;          /**< Name of the sender as sent in headers */
  char*       app_name;             /**< Name of the application as sent in headers */

  CState      state;                /**< Current state of the ClientHandler, \see CState */
  CState      content;              /**< Type of expected content, \see CState \bug should not be a CState*/
  Socket*     socket;               /**< OSocket from which the data is coming */
  SockEvtSource *event;             /**< Event handler for the OComm EventLoop */
  MBuffer* mbuf;                    /**< Mbuffer for incoming data */

  time_t      time_offset;          /**< value to add to remote ts to sync time across all connections */
} ClientHandler;

ClientHandler* client_handler_new (Socket* new_sock);
void client_handler_free (ClientHandler* self);

#endif /*CLIENT_HANDLER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

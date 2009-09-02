/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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
#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_

#include "database.h"

typedef enum _cstate {
  C_HEADER,       // processing header info
  C_BINARY_DATA,  // data is of binary format
  C_TEXT_DATA,    // data in binary format
  C_PROTOCOL_ERROR,// a protocol error occurred --> kick the client
} CState;

#define DEF_NUM_VALUES  30
#define MAX_STRING_SIZE 64

typedef struct _clientHandler {
  //! Name used for debugging
  char name[MAX_STRING_SIZE];

  Database*   database;
  DbTable**   tables;
  int         table_size;    // size of tables array

//  char        app_name[MAX_STRING_SIZE];
  int         sender_id;

  CState      state;
  CState      content;
  Socket*     socket;
  OmlMBuffer  mbuf;

  OmlValue*   values;
  int         value_count;

  OmlValue    table_name;

  long        time_offset;  // value to add to remote ts to
                            // sync time across all connections


} ClientHandler;


#endif /*CLIENT_HANDLER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
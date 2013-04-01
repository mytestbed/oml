/*
 * Copyright 2011-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file buffered_writer.c
 * \brief Public interfaces of the BufferedWriter.
 */

#ifndef OML_BUFFERED_WRITER_H_
#define OML_BUFFERED_WRITER_H_

#include "mbuf.h"
#include "oml2/oml_out_stream.h"

typedef void* BufferedWriterHdl;

BufferedWriterHdl bw_create(oml_outs_write_f writeFunc, OmlOutStream* writeFuncHdl, long queueCapacity, long chunkSize);

void bw_close(BufferedWriterHdl instance);

int bw_push(BufferedWriterHdl instance, uint8_t* chunk, size_t size);
int bw_push_meta(BufferedWriterHdl instance, uint8_t* chunk, size_t size);

MBuffer* bw_get_write_buf(BufferedWriterHdl instance, int exclusive);

void bw_unlock_buf(BufferedWriterHdl instance);

#endif // OML_BUFFERED_WRITER_H_

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

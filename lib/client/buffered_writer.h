
#ifndef OML_BUFFERED_WRITER_H_
#define OML_BUFFERED_WRITER_H_

#include <mbuf.h>
#include <oml2/oml_out_stream.h>

/*! Called to write a chunk to the lower level writer
 *
 * Return number of sent bytes on success, -1 otherwise
 */
/* typedef long (*bw_write_f)( */
/*   void* hdl, */
/*   void* buffer, */
/*   long  length */
/* ); */

typedef void* BufferedWriterHdl;

BufferedWriterHdl
bw_create(
  oml_outs_write_f  writeFunc,        //! Function to drain buffer - can block
  OmlOutStream*     writeFuncHdl,     //! Opaque handler to above function
  long        queueCapacity,    //! size of queue before dropping stuff
  long        chunkSize         //! the size of buffer space allocated at a time, set to 0 for default
);

/**
 * \brief Destroy or free all allocated resources
 */
void
bw_close(
  BufferedWriterHdl instance  //! Instance handle
);

/**
 * \brief Add a chunk to the end of the queue.
 * \param instance BufferedWriter handle
 * \param chunk Pointer to chunk to add
 * \param chunkSize size of chunk
 * \return 1 if success, 0 otherwise
 */
int
bw_push(
  BufferedWriterHdl instance,
  uint8_t*  chunk,
  size_t size
);

MBuffer*
bw_get_buf_locked(
  BufferedWriterHdl instance
);


/**
 * \brief Return the +mbuf+ at the writer end of the queue.
 *
 * If +exclusive+ is set to true (1), this function is also
 * trying to obtain a writer lock on the buffer. In this case,
 * do NOT forget to unlock the buffer with +bw_unlock_buf+.
 *
 * \return MBuffr instance if success, NULL otherwise
 */
MBuffer*
bw_get_write_buf(
  BufferedWriterHdl instance,
  int exclusive
);

/**
 * \brief Unlock a buffered writer.
 */
void
bw_unlock_buf(BufferedWriterHdl instance);


/**
 * \brief buffer the beginning of a measurement row in this instance
 * \return 1 if successful, 0 otherwise
 */
int
bw_row_start_text(
  BufferedWriterHdl instance,
  OmlMStream* ms,
  double now
);


/**
 * \brief buffer the results in this instance
 * \return 1 if successful, 0 otherwise
 */
int
bw_row_values_text(
  BufferedWriterHdl instance,
  OmlValue*  values,            /*! array of values  */
  int        value_count        /*! array size */
);


#endif // OML_BUFFERED_WRITER_H_

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/

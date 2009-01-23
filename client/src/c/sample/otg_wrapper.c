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
#include <oml/omlc.h>

#if 0

// sender_port(
// long pkt_seqno, long pkt_size, float gen_timestamp,
// long tx_timestamp, int seq_no, long rec_ts, int length)
static char* n1[7] = {"pkt_seqno", "pkt_size", "gen_timestamp", "tx_timestamp", "seq_no", "rec_ts", "length"};
static OmlValueT t1[7] = {OML_LONG_VALUE, OML_LONG_VALUE, OML_FLOAT_VALUE, OML_LONG_VALUE, OML_INT_VALUE, OML_LONG_VALUE, OML_INT_VALUE};
static OmlMPDef d1 = {1, 7, n1, t1};

static OmlMPDef* mp_def[1] = {d1};

int
initialize_oml(int* argcPtr, const char** argv, oml_log_fn oml_log)

{
  omlc_init(argcPtr, argv, oml_log, 1, mp_def);
}

void
oml_senderport(
  long pkt_seqno,
  long pkt_size,
  float gen_timestamp,
  long tx_timestamp,
  int seq_no,
  long rec_ts,
  int length
) {
	OmlMPoint* mp = omlc_mp_start(/* index */1);

	for (; mp; mp = mp->next) {
		OQueue* q;
		q = mp->queues[0];
		if (q) oqueue_add_long(q, pkt_seqno);
		q = mp->queues[1];
		if (q) oqueue_add_long(q, pkt_size);
		q = mp->queues[2];
		if (q) oqueue_add_float(q, gen_timestamp);
		// ....
		q = mp->queues[6];
		if (q) oqueue_add_int(q, length);

		omlc_mp_process(mp);
	}
	omlc_mp_end(1);
}


int
main(int argc, const char** argv)

{
	initialize_oml(&argc, argv, NULL);

	oml_senderport(1, 200, 99.11, 888, 2, 777, 500);
}

#endif

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/

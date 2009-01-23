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
#include <oml2/omlc.h>
//#include <oml2/oml_filter.h>

// oml_lin(string label, int seq_no)
static char* n1[2] = {"label", "seq_no"};
static OmlValueT t1[2] = {OML_STRING_VALUE, OML_LONG_VALUE};
static OmlMPDef d1 = {"lin", 1, 2, n1, t1};

// oml_sin(string label, float angle, float value)
static char* n2[3] = {"label", "angle", "value"};
static OmlValueT t2[3] = {OML_STRING_VALUE, OML_DOUBLE_VALUE, OML_DOUBLE_VALUE};
static OmlMPDef d2 = {"sin", 2, 3, n2, t2};

static OmlMPDef* mp_def[2] = {&d1, &d2};

int
initialize_oml(int* argcPtr, const char** argv, oml_log_fn oml_log)

{
  return omlc_init("generator", argcPtr, argv, oml_log, 2, mp_def);
}

void
oml_lin(
  char* label,
  int seq_no
) {
  OmlMStream* ms;
  if ((ms = omlc_mp_start(0))) {
    for (; ms; ms = ms->next) {
      OmlFilter* f;
      OmlValueU value;

      if ((f = ms->filters[0])) {
        value.stringPtrValue = label;
        f->sample(f, OML_STRING_PTR_VALUE, &value, 0);
      }
      if ((f = ms->filters[1])) {
        value.longValue = (long)seq_no;
        f->sample(f, OML_LONG_VALUE, &value, 1);
      }
      omlc_ms_process(ms);
    }
    omlc_mp_end(0);
  }
}

void
oml_sin(
  char* label,
  float phase,
  float value
) {
  OmlMStream* ms;
  if ((ms = omlc_mp_start(1))) {
    for (; ms; ms = ms->next) {
      OmlFilter* f;
      OmlValueU v;

      if ((f = ms->filters[0])) {
        v.stringPtrValue = label;
        f->sample(f, OML_STRING_PTR_VALUE, &v, 0);
      }
      if ((f = ms->filters[1])) {
        v.doubleValue = (double)phase;
        f->sample(f, OML_DOUBLE_VALUE, &v, 1);
      }
      if ((f = ms->filters[2])) {
        v.doubleValue = (double)value;
        f->sample(f, OML_DOUBLE_VALUE, &v, 2);
      }
      omlc_ms_process(ms);
    }
    omlc_mp_end(1);
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/

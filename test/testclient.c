/*
 * Copyright 2010-2015 National ICT Australia (NICTA), Australia
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

/*
 * Test client to generate test OML output based on a spec
 * received on stdin.
 *
 * Current implementation of the input parser can understand a single
 * MP definition that looks like this:
 *
 *  mp <name> { <eltname> : <type>, <eltname> : <type>, ... } <input_fn>;
 *
 *  The 'mp' token is literal, as are the '{', ':', ',', '}', and ';'
 *  tokens (left brace, colon, comma, right brace, semi-colon).  The
 *  <name> and <eltname> stand for the name of the MP and the name of
 *  the fields of the MP, respectively.  They can be any identifier
 *  containing letters, numbers, and underscores.  The <type>
 *  identifiers can be one of:
 *
 *    integer
 *    int32
 *    double
 *    string
 *
 *  'integer' and 'int32' are synonyms.
 *
 *  The <input_fn> specifies the input function for the MP samples,
 *  and can be one of "linear", "sine", or "gaussian".  Currently this
 *  field is ignored.  All inputs to the MP will be linear counting
 *  sequences.  (String values are given a string representation of
 *  the output of the linear function as their values.)
 *
 *  For instance, the following string, excluding the double quote
 *  marks, is a valid MP specifier:
 *
 *  "mp weather { temperature : double, humidity : double, day : integer } linear;"
 *
 *  Currently the client always generates 10000 samples.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "oml2/omlc.h"
#include "oml_value.h"

#define MAX_WORD 256
#define MAX_SAMPLES 10000

typedef enum mp_input
  {
    MP_INPUT_NONE,
    MP_INPUT_LINEAR,
    MP_INPUT_SINE,
    MP_INPUT_GAUSSIAN
  } mp_input_t;

struct mp
{
  char *name;
  OmlMPDef *def;
  OmlMP *mp;
  int length;
  enum mp_input input;
  struct mp *next;
};

typedef enum perr
  {
    PERR_NONE,
    PERR_NO_OPERATOR,
    PERR_WRONG_OPERATOR,
    PERR_WORD_TOO_LONG,
    PERR_EOF,
    PERR_BAD_MP_DELIMITER,
    PERR_SYSTEM
  } perr_t;

perr_t perr = 0;

void
perr_print (const char *s, const char *p)
{
  char *fmt;

  switch (perr)
    {
    case PERR_WORD_TOO_LONG:
      fmt = "%s:  word is too long\n";
      break;
    case PERR_EOF:
      fmt = "%s:  end of file encountered\n";
      break;
    case PERR_NO_OPERATOR:
      fmt = "%s:  unknown operator encountered\n";
      break;
    case PERR_WRONG_OPERATOR:
      fmt = "%s:  incorrect operator encountered\n";
      break;
    case PERR_SYSTEM:
      fprintf (stderr, "%s:  %s\n", s, strerror (errno));
      return;
    default:
      fmt = "%s:  unknown error occurred\n";
      break;
    }

  fprintf (stderr, fmt, s);
  if (p != NULL)
    {
      fprintf (stderr, "Input token: '%s'\n", p);
    }
  return;
}

const char*
input_to_s (enum mp_input input)
{
  switch (input)
    {
    case MP_INPUT_LINEAR: return "none";
    case MP_INPUT_SINE: return "sine";
    case MP_INPUT_GAUSSIAN: return "gaussian";
    case MP_INPUT_NONE: return "none";
    default: return "BAD_INPUT_TYPE";
    }
}

mp_input_t
input_from_s (const char *s)
{
  if (strncmp (s, "linear", MAX_WORD) == 0) return MP_INPUT_LINEAR;
  if (strncmp (s, "sine", MAX_WORD) == 0) return MP_INPUT_SINE;
  if (strncmp (s, "gaussian", MAX_WORD) == 0) return MP_INPUT_GAUSSIAN;
  return MP_INPUT_NONE;
}

void
set_value (OmlMPDef *def, OmlValueU *v, int value)
{
  char str[MAX_WORD];
  switch (def->param_types)
    {
    case OML_LONG_VALUE:    omlc_set_long(*v,   value); break;
    case OML_INT32_VALUE:   omlc_set_int32(*v,  value); break;
    case OML_UINT32_VALUE:  omlc_set_uint32(*v, value); break;
    case OML_INT64_VALUE:   omlc_set_int64(*v,  value); break;
    case OML_UINT64_VALUE:  omlc_set_uint64(*v, value); break;
    case OML_DOUBLE_VALUE:  omlc_set_double(*v, value); break;
    case OML_STRING_VALUE:
                            snprintf (str, MAX_WORD, "%d", value);
                            omlc_set_string_copy(*v, str, strlen(str));
                            break;
    case OML_BLOB_VALUE:
                            snprintf (str, MAX_WORD, "%d", value);
                            omlc_set_blob(*v, str, strlen(str));
                            break;
    default: omlc_set_long(*v, -1); break;
    }
}

/* skip white space in the input, according to isspace() */
int
skip_white (FILE *fp)
{
  int c = -1;
  do
    {
      c = fgetc (fp);
    }
  while (c != EOF && isspace (c));

  if (c == EOF)
    {
      perr = PERR_SYSTEM;
      return -1;
    }
  else
    {
      ungetc (c, fp);
      return 0;
    }
}

int
is_token_terminator (int c)
{
  const char *terminators = "{},:;";
  return isspace (c) || (strchr (terminators, c) != NULL);
}

char*
read_word (FILE *fp)
{
  int c = -1;
  static char buf[MAX_WORD];
  char *p = buf;

  do
    {
      c = fgetc (fp);

      if (c != EOF && !is_token_terminator (c))
        *p++ = c;
    }
  while (c != EOF && !is_token_terminator (c) && (p - buf) < MAX_WORD - 1);

  if (is_token_terminator (c))
    {
      *p = '\0';
      if (!isspace (c))
        ungetc (c, fp);
      return buf;
    }
  else
    {
      if (c == EOF)
        {
          if (feof (fp))
            perr = PERR_EOF;
          else
            perr = PERR_SYSTEM;
          return NULL;
        }
      else
        {
          perr = PERR_WORD_TOO_LONG;
          return NULL;
        }
    }
}


/* { eltname1 : type1, eltname2 : type2, ... } */
OmlMPDef *
read_mp_def (FILE *fp, int *mp_length)
{
  if (skip_white (fp)) return NULL;

  int c = fgetc (fp);

  if (c == EOF)
    {
      perr = feof (fp) ? PERR_EOF : PERR_SYSTEM;
      return NULL;
    }

  if (c != '{')
    {
      char s[2] = { c, '\0' };
      perr = PERR_BAD_MP_DELIMITER;
      perr_print ("read_mp_def", s);
      return NULL;
    }

  int done = 0;
  int elt_count = 0;
  perr = PERR_NONE;

  struct mpelt
  {
    OmlMPDef def;
    struct mpelt *next;
  } *head = NULL, *elt, *new;


  /* eltname : type, */
  do
    {
      int c;
      char *p;
      char name[MAX_WORD];
      char type[MAX_WORD];
      if (skip_white (fp)) break;
      p = read_word (fp);
      if (p == NULL) break;
      if (*p == '\0')
        {
          /* } or , */
          c = fgetc (fp);
          if (c == '}')
            {
              done = 1;
              continue;
            }
          else if (c == ',') continue;
          else
            {
              char s[2] = { c, '\0' };
              perr = PERR_BAD_MP_DELIMITER;
              perr_print ("read_mp_def: got a strange token", s);
              break;
            }
        }
      strncpy (name, p, MAX_WORD);

      if (skip_white (fp)) break;

      /* : */
      c = fgetc (fp);
      if (c != ':')
        {
          char s[2] = { c, '\0' };
          perr = PERR_BAD_MP_DELIMITER;
          perr_print ("read_mp_def: expecting ':', but got something else", s);
          break;
        }

      if (skip_white (fp)) break;
      p = read_word (fp);
      if (p == NULL) break;
      if (*p == '\0')
        {
          char s[2] = { c, '\0' };
          perr = PERR_BAD_MP_DELIMITER;
          perr_print ("read_mp_def: expecting element type, but got something else", s);
          break;
        }
      strncpy (type, p, MAX_WORD);

      new = (struct mpelt*) malloc (sizeof (struct mpelt));
      if (new == NULL) { perr = PERR_SYSTEM; break; }
      p = (char*) malloc (strlen (name) + 1);
      strncpy (p, name, strlen (name));
      new->def.name = p;
      new->def.param_types = oml_type_from_s (type);
      new->next = head;
      head = new;
      elt_count++;
    }
  while (!done);

  if (perr != PERR_NONE)
    return NULL;

  OmlMPDef *def = (OmlMPDef*) malloc ((elt_count+1) * sizeof (OmlMPDef));

  int i;
  for (elt = head, i = elt_count-1; elt != NULL; elt = elt->next, i--)
    {
      def[i].name = elt->def.name;
      def[i].param_types = elt->def.param_types;
    }

  def[elt_count].name = NULL;
  def[elt_count].param_types = 0;

  *mp_length = elt_count;
  return def;
}

/* mp <name> <mpdef> <inputfn> */
struct mp*
read_mp (FILE *fp)
{
  char *p = NULL;
  char *op = NULL;
  char name[MAX_WORD];
  OmlMPDef *def = NULL;
  int mp_length = 0;
  mp_input_t mp_input = MP_INPUT_NONE;

  if (skip_white (fp))
    {
      perr_print ("read_mp", NULL);
      return NULL;
    }

  op = read_word (fp);

  if (op == NULL)
    {
      perr = PERR_NO_OPERATOR;
      perr_print ("read_mp", NULL);
      return NULL;
    }

  if (strncmp (op, "mp", MAX_WORD) != 0)
    {
      perr = PERR_WRONG_OPERATOR;
      perr_print ("read_mp", op);
      return NULL;
    }

  if (skip_white (fp))
    {
      perr_print ("read_mp: looking for mp name", NULL);
      return NULL;
    }

  p = read_word (fp);
  if (p == NULL)
    {
      perr_print ("read_mp: name not found", NULL);
      return NULL;
    }
  strncpy (name, p, MAX_WORD);

  def = read_mp_def (fp, &mp_length);

  if (def == NULL)
    {
      perr_print ("read_mp: bad MP definition", NULL);
      return NULL;
    }

  if (skip_white (fp))
    {
      perr_print ("read_mp: input type not found", NULL);
      return NULL;
    }

  p = read_word (fp);

  if (p == NULL)
    {
      perr_print ("read_mp: input function not found", NULL);
      return NULL;
    }

  mp_input = input_from_s (p);

  if (mp_input == MP_INPUT_NONE)
    {
      perr_print ("read_mp", p);
      return NULL;
    }

  struct mp *mp = (struct mp*) malloc (sizeof(struct mp));
  if (mp == NULL) return NULL;

  mp->name = (char*) malloc (strlen (name) + 1);
  strncpy (mp->name, name, strlen (name) + 1);

  mp->def = def;
  mp->length = mp_length;
  mp->input = mp_input;

  return mp;
}

int
main (int argc, const char **argv)
{
  int result;
  struct mp *mp = read_mp (stdin);

  if (mp == NULL) exit (EXIT_FAILURE);

  printf ("MP : %s\n", mp->name);
  printf ("LEN : %d\n", mp->length);
  int i, j;
  for (i = 0; i < mp->length; i++) {
      printf ("-> %s : %s\n", mp->def[i].name, oml_type_to_s (mp->def[i].param_types));
  }

  result = omlc_init ("testclient", &argc, argv, NULL);

  if (result == -1)
    {
      fprintf (stderr, "Failed to initialize OML2 library\n");
      exit (EXIT_FAILURE);
    }

  mp->mp = omlc_add_mp (mp->name, mp->def);

  if (mp->mp == NULL)
    {
      fprintf (stderr, "Failed to add OML2 MP %s\n", mp->name);
      exit (EXIT_FAILURE);
    }

  omlc_start ();

  OmlValueU *v = (OmlValueU*) malloc (mp->length * sizeof (OmlValueU));
  omlc_zero_array(v, mp->length);

  for (i = 0; i < MAX_SAMPLES; i++) {
    for (j = 0; j < mp->length; j++) {
      set_value (&mp->def[j], &v[j], i);
    }
    omlc_inject (mp->mp, v);
  }

  /* Reset storage if needed */
  for (j = 0; j < mp->length; j++) {
    if (mp->def[j].param_types == OML_STRING_VALUE)
      omlc_reset_string(v[j]);
    else if (mp->def[j].param_types == OML_BLOB_VALUE)
      omlc_reset_blob(v[j]);
  }
  free(v);

  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/

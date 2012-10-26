#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <popt.h>

#include "oml2/omlc.h"

static int longblob = 0;
static int fixed_size = 0;
static int samples = -1;
static int hex = 0;
static int quiet = 0;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "long", 'l', POPT_ARG_NONE, &longblob, 0, "Generate long blobs (> 64KiB)", NULL},
  { "fixed", 'f', POPT_ARG_INT, &fixed_size, 0, "Used fixed size blobs", "SIZE" },
  { "samples", 'n', POPT_ARG_INT, &samples, 0, "Number of samples to generate. Default=forever", "SAMPLES"},
  { "hex", 'h', POPT_ARG_NONE, &hex, 0, "Generate HEX file output instead of binary", NULL},
  { "quiet", 'q', POPT_ARG_NONE, &quiet, 0, "If set, don't print", NULL},
  { NULL, 0, 0, NULL, 0, NULL, NULL }
};

#define MAX_BLOB (1024 * 1024)

void*
randgen (size_t *n)
{
  long length;
  int i;
  uint8_t *data = NULL;
  // 10000 bytes lee-way on short blobs
  size_t max_size = longblob ? MAX_BLOB : USHRT_MAX - 10000;

  if (fixed_size > 0)
    length = fixed_size;
  else
    while ((size_t)(length = random()) > max_size);

  data = malloc (length);

  if (data == NULL) {
    perror("randgen()");
    return NULL;
  }

  for (i = 0; i < length; i++)
    data[i] = (uint8_t)(random() & 0xFF);

  *n = length;
  return data;
}

int
write_blob_bin (int fd, void *blob, size_t n)
{
  return write (fd, blob, n);
}

int
write_blob_hex (int fd, void *blob, size_t n)
{
  char hex [] = "0123456789ABCDEF";
  unsigned int i = 0;
  uint8_t *hexblob = malloc (2 * n);
  int result = 0;
  for (i = 0; i < n; i++) {
    uint8_t *nybbles = (uint8_t*)hexblob + 2 * i;
    uint8_t *data = (uint8_t*)blob + i;
    nybbles[0] = hex[(*data >> 4) & 0xF];
    nybbles[1] = hex[*data & 0xF];
  }
  result = write (fd, hexblob, 2 * n);
  free (hexblob);
  return result;
}

void
blob_to_file (int index, void *blob, size_t n)
{
  char s[64];
  int fd, result;

  snprintf (s, sizeof(s), "g%d.%s", index, hex ? "hex" : "bin");
  fd = open (s, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
  if (fd == -1) {
    fprintf (stderr, "Could not open file %s: %s\n", s, strerror (errno));
    exit (1);
  }

  result = (hex ? write_blob_hex : write_blob_bin) (fd, blob, n);
  if (result == -1) {
    fprintf (stderr, "Error writing blob (%lu bytes) to file: %s\n", n, strerror (errno));
    close (fd);
    exit (1);
  }
  close (fd);
}

static OmlMPDef mpdef [] = {
  { "label", OML_STRING_VALUE },
  { "seq", OML_UINT32_VALUE },
  { "blob", OML_BLOB_VALUE },
  { NULL, (OmlValueT)0 }
};

OmlMP *mp;

void
run (void)
{
  int i = 0;
  void *blob = NULL;
  size_t blob_length = 0;
  char s[64];
  OmlValueU v[3];
  omlc_zero_array(v, 3);

  fprintf (stderr, "Writing blobs:");
  for (i = 0; samples != 0; i++, samples--) {
    snprintf (s, sizeof(s)-1, "sample-%04d\n",i);
    blob = randgen (&blob_length);
    blob_to_file (i + 1, blob, blob_length);
    fprintf (stderr, " %04d (%ld B)", i, blob_length);
    omlc_set_string (v[0], s);
    omlc_set_int32 (v[1], i);
    omlc_set_blob (v[2], blob, blob_length);
    omlc_inject (mp, v);
  }
  omlc_reset_string(v[0]);
  omlc_reset_blob(v[2]);
  fprintf (stderr, ".\n");
}


int
main (int argc, const char **argv)
{
  int c;

  omlc_init ("blobgen", &argc, argv, NULL);
  mp = omlc_add_mp ("blobmp", mpdef);
  omlc_start ();

  poptContext optcon = poptGetContext (NULL, argc, argv, options, 0);
  while ((c = poptGetNextOpt(optcon)) >= 0);

  run();
  omlc_close ();
  return 0;
}

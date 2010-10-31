#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <popt.h>

#include <oml2/omlc.h>

static int longblob = 0;
static int samples = -1;
static int quiet = 0;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "long", 'l', POPT_ARG_NONE, &longblob, 0, "Generate long blobs (> 64KiB)"},
  { "samples", 'n', POPT_ARG_INT, &samples, 0, "Number of samples to generate. Default=forever"},
  { "quiet", 'q', POPT_ARG_NONE, &quiet, 0, "If set, don't print"},
  { NULL, 0, 0, NULL, 0 }
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

  while ((length = random()) > max_size);

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

void
blob_to_file (int index, void *blob, size_t n)
{
  char s[64];
  int fd, result;

  snprintf (s, sizeof(s), "g%d.bin", index);
  fd = open (s, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
  if (fd == -1) {
    fprintf (stderr, "Could not open file %s: %s\n", s, strerror (errno));
    exit (1);
  }

  result = write (fd, blob, n);
  if (result == -1) {
    fprintf (stderr, "Error writing blob (%u bytes) to file: %s\n", n, strerror (errno));
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

  for (i = 0; samples != 0; i++, samples--) {
    OmlValueU v[3];
    bzero (v, sizeof (v));
    snprintf (s, sizeof(s)-1, "sample-%04d\n",i);
    blob = randgen (&blob_length);
    blob_to_file (i + 1, blob, blob_length);
    fprintf (stderr, "%04d: %ld\n", i, blob_length);
    omlc_set_string (v[0], s);
    omlc_set_int32 (v[1], i);
    omlc_set_blob (v[2], blob, blob_length);
    omlc_inject (mp, v);
  }
}


int
main (int argc, const char **argv)
{
  char c;

  omlc_init ("blobgen", &argc, argv, NULL);
  mp = omlc_add_mp ("blobmp", mpdef);
  omlc_start ();

  poptContext optcon = poptGetContext (NULL, argc, argv, options, 0);
  while ((c = poptGetNextOpt(optcon)) >= 0);

  run();
  omlc_close ();
  sleep (1);
  return 0;
}

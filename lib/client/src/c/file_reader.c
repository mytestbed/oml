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
#include <pcap.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "client.h"
#include "version.h"

/**
 * \fn
 * \brief
 * \param
 * \return
 */
OmlMPDef* split_line(char* line)
{
  int nb_def = 0;
  OmlMPDef* self = NULL;
  char delims[] = " ";
  char delims2[] = ":";
  char* result = NULL;
  char* save_line = NULL;
  strcpy(save_line, line);
  result = strtok(line, delims);
  while( result != NULL ) {
    nb_def++;
    result = strtok( NULL, delims );
  }
  result = strtok(save_line, delims);
  self = (OmlMPDef*)malloc((nb_def-2)*sizeof(OmlMPDef));

}
/**
 * \fn
 * \brief
 * \param
 * \return
 */
int main(int argc,char **argv)
{
//  const char* name = argv[0];
//  const char* p = name + strlen(argv[0]);
//  while (! (p == name || *p == '/')) p--;
//    if (*p == '/') p++;
//  name = p;

//  omlc_init(name, &argc, argv, o_log);
  FILE *fp1;
  int nbValue = 4;
  char **value;
  char *p;
  char oneword[256];
  char *c;
  char str[] = "16.902486       1       1       1498    10.0.1.1        10.0.5.5        0:e:c:7f:8:e now # is the time for all # good men to come to the # aid of their country";
  char delims[] = " ";
  char delims2[] = ":";
  char *result = NULL;
  char *result2 = NULL;
  char* saveptr1, saveptr2;

  fp1 = fopen("./oml_output", "r");
  if (fp1 == NULL)
  {
    // printf("File failed to open\n");
    // 		// exit (EXIT_FAILURE);
  }
  char* tmp;
  c =  fgets(oneword, 256, fp1);
  //result = strtok(c, delims);
  char* subtoken=NULL;
  printf("ttt %s \n ", c);
  while (  c != NULL ) {

    strcpy(tmp, c);
    result = strtok_r(c, delims, &saveptr1);
    printf("rrr %s \n ", c);
    while (  result != NULL ) {
      printf("sddffs %s \n ", result);

      if (strcmp( "schema:",result) == 0){
        result = strtok_r(NULL, delims, &saveptr1);
	result = strtok_r(NULL, delims, &saveptr1);
	result = strtok_r(NULL, delims, &saveptr1);
	printf(" %s \n ", result);
        //strcpy(result2, result);
        subtoken = strtok_r(result, delims2, &saveptr2);

	while(  subtoken != NULL) {
	  printf("subtoken %s \n ", subtoken);
          subtoken = strtok_r(NULL, delims2, &saveptr2);

	}
	result = strtok_r(tmp, delims, &saveptr1);
	//strcpy(result2, result);
	printf("result after token1 %s \n ", result);
	result = strtok_r(NULL, delims, &saveptr1);
	printf("result after token2 %s \n ", result);
	result = strtok_r(NULL, delims, &saveptr1);
	printf("result after token3 %s \n ", result);
	result = strtok_r(NULL, delims, &saveptr1);
	printf("result after token4 %s \n ", result);
	result = strtok_r(NULL, delims, &saveptr1);
	printf("result after token5 %s \n ", result);
	subtoken = strtok_r(result, delims2, &saveptr2);

	while(  subtoken != NULL) {
		printf("sdfsdf %s \n ", subtoken);
		subtoken = strtok_r(NULL, delims2, &saveptr2);


	}

      }
      result = strtok_r(NULL, delims, &saveptr1);

 //     printf("sdds %s \n ", result);
    }

    c =  fgets(oneword, 256, fp1);

  }

    //else{
      //result = strtok( NULL, delims );
    //}

  printf( "GO OUT\n" );




  fclose(fp1);
  return 0;

}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/

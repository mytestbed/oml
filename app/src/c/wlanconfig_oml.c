/*
 *  C Implementation: app_sigar
 *
 * Description:  An application based on the libsigar that communicates with 
 *               the oml library
 *
 *
 * Author: Guillaume Jourjon <guillaume.jourjon@nicta.com.au>, (C) 2008
 *
 * Copyright (c) 2007-2009 National ICT Australia (NICTA)
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
*/

#include <errno.h>
#include <string.h>
#include "oml2/omlc.h"
#include "app_sigar.h"
#include "oml2/oml_filter.h"
#include "client.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pcap.h>
#include <malloc.h>
#include <errno.h>
#include "log.h"


static OmlMPDef oml_def[] = { 
{"macAddress", OML_STRING_PTR_VALUE},
{"RSSI", OML_LONG_VALUE},
{"DBM", OML_LONG_VALUE},
{NULL, (OmlValueT)0},
};
static OmlMP* oml_mp = NULL;



int
main(
        int argc,
        const char *argv[]
) {
    
   int i = 0;
   int stop = 1;
    OmlValueU v[3];
    pid_t pid;
    int mypipe[2];
    char command[256]; 
    char command2[256] = "256";
    char addressToCheck[256] = "000";
    //memset(command2, 0, 256*sizeof(char));
    int* argc_;
    const char** argv_;
    char mem_used[64];
    omlc_init(argv[ 0 ], &argc, argv, o_log);

    argc_ =&argc;
    argv_ = argv;

   
      
      oml_mp = omlc_add_mp("wifi_info", oml_def);
    
    
     
    
    
 
        
        
     omlc_start();

    //command2 = "cmd";
   while(1){
       i = 0;
       stop = 1;
       if (pipe (mypipe))
         {
           fprintf (stderr, "Pipe failed.\n");
           return EXIT_FAILURE;
         }
       pid = fork ();
       if (pid == (pid_t) 0)
       {
         close (mypipe[0]);
         close(1);
         dup(mypipe[1]);
         if (execl("/usr/local/bin/wlanconfig", "wlanconfig", "ath0", "list", NULL) < 0){
            printf("execl failed\n");
            exit(1);
        
         }
       
        
       }else if (pid < (pid_t) 0){
         fprintf (stderr, "Fork failed.\n");
         return EXIT_FAILURE;
       }
       else {
         close(mypipe[1]);
         close(0);
         dup(mypipe[0]);
         scanf ("%s",command);
         
         while(stop){
           strcpy(command2, command);
           
           if(strcmp(command,"ADDR") != 0){
            printf("result wlanconfig: %s \n ", command);
             //if(strcmp(command, addressToCheck) != 0)
                v[0].stringPtrValue = command; 
                scanf ("%s",command);
                printf("result wlanconfig: %s \n ", command);
                scanf ("%s",command);
                printf("result wlanconfig: %s \n ", command);
                scanf ("%s",command);
                printf("result wlanconfig: %s \n ", command);
                scanf ("%s",command);
                printf("result wlanconfig: %s \n ", command);
                v[1].longValue = atol(command);
                scanf ("%s",command);
                printf("result wlanconfig: %s \n ", command);
                v[2].longValue = atol(command);
                 omlc_process(oml_mp, v);
                 stop=0;
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                 scanf ("%s",command);
                //printf("result wlanconfig: %s \n ", command);
           }else{
            for(i = 0; i < 14; i++){
                
                scanf ("%s",command);
            }
            }
           //scanf ("%s",command);
        
         }
       
        }
        sleep(1);
   }
          
//        v[0].longValue = (long)interface_stat.rx_bytes;
//        v[1].longValue = (long)interface_stat.tx_bytes;
//        v[2].stringPtrValue = (char*) mem_used;
//        //printf("memory %s \n", mem_used);
//        v[3].longValue = (long)cpu_info.user;
//        v[4].longValue = (long)cpu_info.total;
//        
//        
//        omlc_process(sigar_mp->mp, v);

       
        
        
        
    
        
        
    
    exit(0);    
}

#include "dataqueue.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

DataQueue::DataQueue(){
        current_ = new data;
        current_->next_ = 0;
        base_ = current_;
}

DataQueue::~DataQueue(){

        data *databuf;
        //clear all data
        while((databuf = pop()) != 0 )
                delete databuf->payload_;
        delete current_;
}


struct data *DataQueue::pop(){
        struct data     *temp;

        //check for existence of data
        if(base_->next_ == 0)
                return 0;                       //no data is on stack
        temp = base_;
        base_ = base_->next_;     
        return temp;
}

/*
void eque::apush(char *format,...)
{
        va_list var_arg;
        char    new_string[100];

        va_start(var_arg,format);
        vsprintf(new_string,format,var_arg);
        push(new_string);
        va_end(var_arg);
}
*/

void DataQueue::push(char *newdata, short len)
{
        struct data *temp = new data;
        //Add new payload to waiting structure...
        current_->payload_ = (char *) malloc(len);
        memcpy(current_->payload_, newdata, len*sizeof(char));
        current_->len_ = len;
        //Add new structure to wait for new data
        current_->next_ = temp;
        current_ = temp;
        current_->next_ = 0;
}


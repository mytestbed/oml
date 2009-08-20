
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "otg2/packet.h"

///**
// * Constructor
// */
//
//SendParam::SendParam()
//{
//  seqnum_= 0;
//  //senderaddr_ =NULL;
//  senderaddr_ = new char[50];
//  sendermac_= new char[20];
//   txpower_ = 0; 
//   channel_= 0; 
//  streamid_= 0;;   
//  txtime_ = 0; 
//  mactxtime_= 0;
//  tcp_seqno_= 0;
//  txport_ = 0;
//  txrate_=0;
//}
//
// 
//ReceiveParam::ReceiveParam()
//{
//  receiveraddr_ = new char[50];
//  //receiveraddr_ = NULL;
//  rssi_= 0;
//  noise_ = 0 ;
//  mac_timestamp_ = 0;
//  flowid_ = 0;   ///<the flow who recived it.
//  seqno_ = 0;  ///< the sequence number attached by the flow.
//  macrxtime_= 0;
//  rxtime_ = 0;
//  rxport_ = 0;
//     
//}


/**
 * Default Packet constructor.
 * It is used only for generator to produce the very first empty packet.
 * this empty packet will be used again and agian for every packet generated.
 * A buffer with length 512 is initialzied for packet payload. If it is smaller, a new buffer will create to replace this.
 * set length to zero is dangerous, because that means a null pointer of payload_ is initialized...
 *@see setPayloadSize
 */

//Packet::Packet()
//{
////  txMeasure_ =  new SendParam();
////  rxMeasure_ =  new ReceiveParam();
//  reset();
//  length_ = DEFAULT_PAYLOAD_SIZE;
//  payload_ = new char[DEFAULT_PAYLOAD_SIZE];
//}

/**
 * Alternate  Packet Constructor:
 * Specify a large buffer....
 * This is useful to define a packet receiving buffer, used by some 
 * objects of OTR application
 * @see Flow
 */

Packet::Packet(
  int buffer_length,
  UnixTime* clock
) {
//  txMeasure_ =  new SendParam();
//  rxMeasure_ =  new ReceiveParam();
  reset();
  length_ = buffer_length;
  payload_ = new char[buffer_length];
  clock_ = clock;
}

/** 
 * set packet size.
 * As packet already has a defult buffer, 
 * there are two ways to determine the payload
 *  - set size only, let the payload be as it is --> SetPayloadSize
 *   -# if necessary, adjust the payload buffer size.
 *  - set size and also fill the payload with speficic data (e.g.  for Audio and Video Applications...) 
 * @see  fillPayload
 */

void Packet::reset()

{
  ts_ = 0;  
  tx_time_ = 0;  
  size_ = 0;
  flowid_ = -1;
  seqnum_ = 0;
  offset_ = 0;
}

void Packet::setPayloadSize(int size)
{
  size_ =  size;
  if (size > length_) {
    if (payload_ != NULL) delete [] payload_;
    length_ = (int)(1.5 * size);
    payload_ =  new char[length_];
  }
}

 /** 
  *  A function to fill payload.
  *  user can specify the content of payload for applications like audio/video playback...
  */
int Packet::fillPayload(int size, char *inputstream)
{
  setPayloadSize(size);
  if (memcpy((char *)payload_, (char *)inputstream,  size) == NULL) {
    throw "Fill payload Failed";
  }
  return 0;
}

void
Packet::stampPacket(
  char version
) {
  char* p = getBufferPtr(32, 0);
  *(p++) = SYNC_BYTE;
  *(p++) = SYNC_BYTE;
  *(p++) = version;
  offset_ = 3;
}

int
Packet::stampLongVal(
  long val,
  int offset
) {
  if (offset < 0) {
    offset = offset_;
    offset_ += 4;    
  }
  char* p = getBufferPtr(4 + offset, 0) + offset;
  uint32_t uv = (uint32_t)val;
  uint32_t nv = htonl(uv);

  *(p++) = (char)((nv >> 24) & 0xff);
  *(p++) = (char)((nv >> 16) & 0xff);
  *(p++) = (char)((nv >> 8) & 0xff);
  *(p++) = (char)(nv & 0xff);
  return offset;
}

int
Packet::stampShortVal(
  short val,
  int offset
) {
  if (offset < 0) {
    offset = offset_;
    offset_ += 2;    
  }
  char* p = getBufferPtr(2 + offset_, 0) + offset_;
  uint16_t uv = (uint16_t)val;
  uint16_t nv = htons(uv);

  *(p++) = (char)((nv >> 8) & 0xff);
  *(p++) = (char)(nv & 0xff);
  return offset;
}

/** A function to recover some packet information from the payload of received packet
 * As the sending application might stamp some information in the payload,
 * so be careful to use this function
 *  
 */
char 
Packet::checkStamp()

{
  offset_ = 0;
  if (size_ < 3) return -1; // not the best solution

  char* p = payload_;
  if (*(p++) == (char)SYNC_BYTE && *(p++) == (char)SYNC_BYTE) {
    offset_ = 3;
    return *p;
  }
  return -1; 
}

long 
Packet::extractLongVal()

{
  if (size_ < offset_ + 4) return 0; // not the best solution

  char* p = payload_ + offset_;  
  uint32_t nv = *p++ << 24; 
  nv += *p++ << 16;
  nv += *p++ << 8;
  nv += *p++;
  uint32_t hv = ntohl(nv);
  offset_ += 4;
  long v = (long)(hv);
  return v;
}

short
Packet::extractShortVal()

{
  if (size_ < offset_ + 2) return 0; // not the best solution

  char* p = payload_ + offset_;  
  uint16_t nv = *p++ << 8;
  nv += *p++;
  uint16_t hv = ntohs(nv);
  offset_ += 2;
  short v = (short)(hv);
  return v;
}

char*
Packet::getBufferPtr(
  int minLength, 
  int maintainContent
) {
  if (length_ >= minLength) return payload_;
  
  // Need to resize
  if (maintainContent) {
    throw "Not implemented";
  }
  
  if (length_ > 0) delete payload_;
  payload_ = new char[minLength];
  length_ = minLength;
  
  return payload_;
}

void
Packet::setTimeStamp(
  double time
) {
  if (time <= 0 && clock_ != NULL) {
    // set to current time
    ts_ = clock_->getAbsoluteTime() * 1e3;
  } else {
    ts_ = time;
  }
}

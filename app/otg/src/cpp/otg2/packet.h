#ifndef OTG_PACKET_H
#define OTG_PACKET_H

#include <string.h>
#include "otg2/unixtime.h"

#define MAX_HOSTNAME_LENGTH 256

#define SYNC_BYTE 0xAA



///**An auxillary class for binding some measurements with the packet.
// * Currently, we list all possible TX parameters here
// */
//class SendParam
//{
// public:
//   SendParam();
// private:
//     
//   int txpower_; ///< dbm, rounded to integer      
//   short channel_; ///< wireless channel number in 802.11 a/b/g (optional) (for wireless use)
//   short streamid_;  ///<the stream id
//   long seqnum_;     ///< sequence number attached by the stream
//   double txtime_;  ///<transmit timing in milliseconds      
//   long mactxtime_; ///<this parameter is set by the driver to show when the packet is delivered, in microseconds
//   long tcp_seqno_;  /// the Sequence number in TCP header TCP port's sequence number if TCP Port is used to sent.
//   short txport_;
//   short txrate_;    ///The channel rate used by the sender (optional) (for wireless use)
//   char *senderaddr_; ///IP address of the sender
//   char *sendermac_;  /// MAC address of the sender
// 
// public:
//   inline short getStreamId(){return streamid_;}
//   inline void setStreamId(short i){streamid_ = i;}
//   inline unsigned long getSequenceNum(){return seqnum_;}
//   inline void setSequenceNum(unsigned long i){seqnum_ = i;}
//   inline void setTxTime(double ms){ txtime_ = ms; }
//   inline double getTxTime(){return txtime_;}
//   inline void setSenderName(char *str){ strcpy(senderaddr_,str); }
//   inline char* getSenderName() {return senderaddr_; } 
//   inline void setSenderMAC(char *str){ strcpy(sendermac_,str); }
//   inline char* getSenderMAC() {return sendermac_; } 
//   inline void setSenderPort(short port) { txport_ = port;}
//   inline short getSenderPort(){ return txport_;}
//   inline void  setPHYXmitRate(short rate){txrate_ = rate;}
//   inline short getPHYXmitRate(){return txrate_;}
//
//};

///**An auxillary class for binding some receiver-specified 
// * measurements with the packet.
// * Currently, we list all possible RX parameters here
// */
//
//class ReceiveParam
//{
// public:
//      ReceiveParam();
//      
//  
// public:
//   inline short getFlowId(){return flowid_;}
//   inline void setFlowId(short i){flowid_ = i;}
//   inline unsigned long getFlowSequenceNum(){return seqno_;}
//   inline void setFlowSequenceNum(unsigned long i){seqno_ = i;}
//   inline void setRxTime(double ms){ rxtime_ = ms; }
//   inline double getRxTime(){return rxtime_;}
//   inline int getReceivedLength() {return rx_size_;}
//   inline void setReceivedLength(int le){rx_size_ = le;}
//   inline void setRSSI(short rssi){rssi_=rssi; }
//   inline void setMacTimestamp(int ts) {mac_timestamp_ = ts;}
//   inline int getMacTimestamp() {return mac_timestamp_;}
//   inline short getRSSI() {return rssi_;}
//   
//   inline void 
//   setPort(const short port){ port_ = port; }
//   
//   inline short 
//   getPort(){ return port_; }
//
//   inline void 
//   setHostname(const char* hostname) 
//   { 
//     if (hostname == NULL) 
//       hostname_[0] = '\0'; 
//     else 
//       strncpy(hostname_, hostname, MAX_HOSTNAME_LENGTH); 
//   }
//   
// private:     
//      short rssi_;   ///< RSSI value from Wireless PHY of received pacekt 
//      short noise_;  ///< Noise value from Wireless PHY of received packet
//      int mac_timestamp_; ///<MAC timestamp from the driver
//      int flowid_;   ///<Id of The flow which recived the packet.
//      long seqno_;   ///<The sequence number attached by the flow. This value will implicitly tell the flow throughput. 
//      long macrxtime_; ///<The timestampe when MAC_RX catches the packet
//      double rxtime_;    /// < The time when the paket is received by the Gate, in milliseconds
//      int rx_size_;   /// The actual size of received packet.
//      char *receiveraddr_;  ///IP address of the receiver
//      short rxport_;   ///<Port number, if applicable.
//      char   hostname_[MAX_HOSTNAME_LENGTH]; ///< both hostname and ipaddress format (10.0.0.1) could be given as a string
//      short port_;  ///< port number for UDP or TCP (Transport layer)
//   
//};


/**
 * Packet is an entity given by generator.
 * This packet class only carries
 * payload information and a sending timestamp, not any socket address information. Also, the packet would carry all measuremnts related to this packet with it.
 */
class Packet {
 
public:
  /**
   *  Default payload size is 512 Bytes
   */
  static const int DEFAULT_PAYLOAD_SIZE = 512;
  
  //Packet();
  Packet(int buffer_length = 512, UnixTime* clock = NULL);
  
  // Reset all infomation in this packet
  void reset();
  
  int fillPayload(int size, char *inputstream);  

  inline char* getPayload() { return payload_;} 
  void setPayloadSize(int size);
  
  /**get the size of packet buffer where payload is stored.
   */
  inline int getBufferSize(){return length_;}
  
  /** Returns a ptr to the packets buffer which
   * is ensured to be at least +size+ long.
   * 
   * If +maintainContent is set, copy current
   * content in case the buffer needs to be resized. 
   */
  char* getBufferPtr(int size, int maintainContent = 0);
  
 /** get the size of the packet 
  *  
  */
  inline int getPayloadSize(){ return size_;}

  /** Timestamp in seconds */
  void setTimeStamp(double stamp);
  inline double getTimeStamp()  {return ts_; }  

  /** Time when to send the packet */
  inline void setTxTime(double time) {tx_time_ = time;}
  inline double getTxTime()  {return tx_time_;}  

  inline short getFlowId()       {return flowid_;}
  inline void setFlowId(short i) {flowid_ = i;}
  
  inline unsigned long getSequenceNum() {return seqnum_;}
  inline void setSequenceNum(unsigned long i) {seqnum_ = i;}
  
  /** Mark the packet with a leading pattern followed by a 
   * version byte. Additional numbers can be added with ... 
   */
  void stampPacket(char version);
  char checkStamp();
  
  
  /** Write a value into the payload at a certain offset.
   * If the offset is < 0, the value will be appended to
   * previously stamped values. In both cases, the offset
   * will be returned.
   */
  int stampLongVal(long val, int offset = -1);
  int stampShortVal(short val, int offset = -1);
  
  /** Extract values stored in the header (of payload). */
  long extractLongVal();
  short extractShortVal();  

      
private:      
  /** Expected Deliver Timestamp.
   * Expected time to send this paket.absolute time in seconds.
   * the actual sending  time canot as precise as this double-precision variable. 
   * this is a relevant time, refer to the time when TG is started
   */
  double ts_;  
  double tx_time_;
  
  int size_;  ///< Packet length in Bytes
  int length_; ///< Maximum allocated Size of Payload buffer. it is no less than the size_
  char* payload_; ///< Payload Pointer.

  short flowid_;
  unsigned long seqnum_;
  
  int offset_;
  
  UnixTime* clock_; // Source for timestamping

  
};


#endif

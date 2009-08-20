
#include "otg2/address.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
using namespace std;

Address::Address():port_(-1)
{   
  hostname_[0] = '\0';  
  macaddr_[0] = '\0';
}

Address::Address(char* hostname, short port)
{ 
   setPort(port); 
   setHostname(hostname);
}

/**
 *Function to convert the input MAC address string to bytes.
 * First, check the MAC address is valid
 * - there are at least 12 Hex characters
 * - there are no other charcter except colon
 */
void Address::setHWAddrFromColonFormat(const char* colonmac)
{  
  char HexChar;
  //First verify the address
  int Count  = 0;
  int num_mac_char = 0;
  /* Two ASCII characters per byte of binary data */
  bool error_end = false;
  while (!error_end)
    { /* Scan string for first non-hex character.  Also stop scanning at end
         of string (HexChar == 0), or when out of six binary storage space */
      HexChar = (char)colonmac[Count++];
      if (HexChar == ':') continue;     
      if (HexChar > 0x39) HexChar = HexChar | 0x20;  /* Convert upper case to lower */
      if ( (HexChar == 0x00) || num_mac_char  >= (MAC_ADDR_LENGTH * 2) ||
           (!(((HexChar >= 0x30) && (HexChar <= 0x39))||  /* 0 - 9 */
             ((HexChar >= 0x61) && (HexChar <= 0x66))) ) ) /* a - f */ 
	{
	  error_end = true;
	} else 
            num_mac_char++;
    }
  if (num_mac_char != MAC_ADDR_LENGTH * 2 )
    throw "Given Wrong MAC address Format.";

  // Conversion
  unsigned char HexValue = 0x00;
  Count = 0;
  num_mac_char = 0;
  int mac_byte_num = 0;
  while (mac_byte_num < MAC_ADDR_LENGTH )
    {
      HexChar = (char)colonmac[Count++];
      if (HexChar == ':') continue;
      num_mac_char++;  // locate a HEX character
      if (HexChar > 0x39)
        HexChar = HexChar | 0x20;  /* Convert upper case to lower */
      HexChar -= 0x30;
      if (HexChar > 0x09)  /* HexChar is "a" - "f" */
	HexChar -= 0x27;
      HexValue = (HexValue << 4) | HexChar;
      if (num_mac_char % 2  == 0 ) /* If we've converted two ASCII chars... */
        {
          macaddr_[mac_byte_num] = HexValue;
	  HexValue = 0x0;
	  mac_byte_num++;
        }

    }  

  return;
   
}


/**
 * Convert HW Address to  Colon Seperated format
 */
char *Address::convertHWAddrToColonFormat()
{
  char *colonformat =  new char[17];
  //printf("HW Address: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",u[0], u[1], u[2], u[3], u[4], u[5]);
  sprintf(colonformat,"%02X:%02X:%02X:%02X:%02X:%02X",
          macaddr_[0],macaddr_[1],macaddr_[2],macaddr_[3],macaddr_[4],macaddr_[5]);
  // cout << colonformat << endl;
  return colonformat;

}

/**
 *copy mac address
 */
void Address::setHWAddr(unsigned char* hwaddr)
{
  memcpy(macaddr_, hwaddr , MAC_ADDR_LENGTH*sizeof(unsigned char));
}

/**
 * Compare if two mac address is same or not.
 * use memcmp to compare each byte.
 */
bool Address::isSameMACAddr( Address *addr)
{
  if ( memcmp(macaddr_, addr->getHWAddr(), MAC_ADDR_LENGTH*sizeof(unsigned char))  == 0 )
           return true;
  return false;

}

Address* Address::clone()
{
  Address * ad =  new Address(hostname_, port_);
  ad->setHWAddr(macaddr_);
  return ad;

}

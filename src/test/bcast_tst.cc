#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>


enum {
    MPSLinkProtocolVersionInvalid = 0,
    MPSLinkProtocolVersion1       = 1,
    MPSLinkProtocolVersion2       = 2,
    MPSLinkProtocolVersion3       = 3,
    MPSLinkProtocolVersionCurrent = MPSLinkProtocolVersion3,
    MPSLinkProtocolPort           = 56789,
    MPSLinkProtocolResponsePort   = 56790
};

enum MPSLinkProtocolMessageTypes {
    MPSMessageTypeInvalid                       = 0x00,

    /* Outgoing Message Types LP to LNs */
    MPSLinkProcessorMessageTypeSynchronization  = 0x01,
    MPSLinkProcessorMessageTypePermit           = 0x02,
    MPSLinkProcessorMessageTypeUnlatchStatus    = 0x10,
    MPSLinkProcessorMessageTypeUnlatchStatusBlm = 0x11,
    MPSLinkProcessorMessageTypeUnlatchStatusPic = 0x12,
    MPSLinkProcessorMessageTypeUnlatchStatusAnalog = 0x13,

    /* Incoming Message Types LNs to LP */
    MPSLinkNodeMessageTypeStatus                = 0x80,
    MPSLinkNodeMessageTypeStatusBlm             = 0x81,
    MPSLinkNodeMessageTypeStatusPic             = 0x82,
    MPSLinkNodeMessageTypeStatusAnalog          = 0x83,

    /* Beam Synchronous Acquisition Messages */
    MPSBSAMessageTypeEnble                      = 0x70,
    MPSBSAMessageTypeBsaData                    = 0xB0
};
typedef enum MPSLinkProtocolMessageTypes MPSLinkProtocolMessageType;
enum {
    MPSLinkProtocolHeaderSize     = 2,

    MPSLinkProtocolMessageSizeMin = MPSLinkProtocolHeaderSize,
    MPSLinkProtocolMessageSizeMax = 1400,

    MPSLinkProtocolDataSizeMin    = 0,
    MPSLinkProtocolDataSizeMax    = MPSLinkProtocolMessageSizeMax - MPSLinkProtocolHeaderSize
};

#define LINK_SYNC_MESSAGE_SIZE                                       (9 + MPSLinkProtocolHeaderSize)

#define LINK_SYNC_TIMESTAMP_SECPASTEPOCH_POINTER(_pointerToMLPData)  ((uint32_t *)((uint8_t *)(_pointerToMLPData) + 0))
#define LINK_SYNC_TIMESTAMP_NSEC_PULSEID_POINTER(_pointerToMLPData)  ((uint32_t *)((uint8_t *)(_pointerToMLPData) + 4))
#define LINK_SYNC_TIMESLOT_POINTER(_pointerToMLPData)                ((uint8_t  *)((uint8_t *)(_pointerToMLPData) + 8))

#define LINK_SYNC_TIMESTAMP_SECPASTEPOCH(_pointerToMLPData)          (*LINK_SYNC_TIMESTAMP_SECPASTEPOCH_POINTER(_pointerToMLPData))
#define LINK_SYNC_TIMESTAMP_NSEC_PULSEID(_pointerToMLPData)          (*LINK_SYNC_TIMESTAMP_NSEC_PULSEID_POINTER(_pointerToMLPData))
#define LINK_SYNC_TIMESLOT(_pointerToMLPData)                        (*LINK_SYNC_TIMESLOT_POINTER(_pointerToMLPData))

#define LINK_MESSAGE_HEADER_SECTION_POINTER(_pointerToMLPMessage)   ((uint8_t  *)(_pointerToMLPMessage) + 0)
#define LINK_MESSAGE_PROTOCOL_VERSION_POINTER(_pointerToMLPHeader)  ((uint8_t *)(_pointerToMLPHeader)  + 0)
#define LINK_MESSAGE_TYPE_POINTER(_pointerToMLPHeader)              ((uint8_t *)(_pointerToMLPHeader)  + 1)
#define LINK_MESSAGE_DATA_SECTION_POINTER(_pointerToMLPMessage)     ((uint8_t  *)(_pointerToMLPMessage) + 2)

#define LINK_MESSAGE_PROTOCOL_VERSION(_pointerToMLPHeader)          (*LINK_MESSAGE_PROTOCOL_VERSION_POINTER(_pointerToMLPHeader))
#define LINK_MESSAGE_TYPE(_pointerToMLPHeader)                      (*LINK_MESSAGE_TYPE_POINTER(_pointerToMLPHeader))

struct linkNodeMessage
{
  uint8_t               data[MPSLinkProtocolMessageSizeMax];
    int                 dataLength;
    struct sockaddr_in  address;
    unsigned int        addressLength;
};

void broadcast_msg(uint8_t *mess, int mess_size){
  std::cout << "Size: " << mess_size << std::endl;

  int sock;
  struct sockaddr_in broadcastAddr;
  const char *broadcastIP = "255.255.255.255";
  unsigned short broadcastPort = 56789;
  char *sendString;
  int broadcastPermission;
  int sendStringLen;

  sendString = (char *)mess;             /*  string to broadcast */

  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
    fprintf(stderr, "socket error");
    exit(1);
  }

  broadcastPermission = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0){
    fprintf(stderr, "setsockopt error");
    exit(1);
  }

  /* Construct local address structure */
  memset(&broadcastAddr, 0, sizeof(broadcastAddr));
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_addr.s_addr = inet_addr(broadcastIP);
  broadcastAddr.sin_port = htons(broadcastPort);

  sendStringLen = mess_size;

  /* Broadcast sendString in datagram to clients */
  if (sendto(sock, sendString, sendStringLen, 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) != sendStringLen){
    fprintf(stderr, "sendto error");
    exit(1);
  }

}

int main(int argc, char *argv[]) {
  uint8_t message[LINK_SYNC_MESSAGE_SIZE];
  uint8_t *data;

  /* Get pointer to data section */
  data = LINK_MESSAGE_DATA_SECTION_POINTER(message);

  /* Set the protocol version and message type */
  LINK_MESSAGE_PROTOCOL_VERSION(message) = MPSLinkProtocolVersionCurrent;
  LINK_MESSAGE_TYPE(message)             = MPSLinkProcessorMessageTypeSynchronization;

  /* Fill in the message data */
  LINK_SYNC_TIMESTAMP_SECPASTEPOCH(data) = htonl(0xAA);
  LINK_SYNC_TIMESTAMP_NSEC_PULSEID(data) = htonl(0xBB);
  LINK_SYNC_TIMESLOT(data)               = 0xCC;

  broadcast_msg(message,LINK_SYNC_MESSAGE_SIZE );

  return 0;
}


#include "uip.h"

/* hdr id for message used to pass ip of a node trying to join the ring*/
#define PASS_IP 1 // hdr | Ip | Id

/* hdr id for messages used to connect to a node, and tell the node what its new succor is*/
#define JOIN_SUCC 2  // hdr | Id | Id | Ip

/* hdr id for election messages*/
#define ELECTION 3 // hdr | Id

/* hdr id for elected messages*/
#define ELECTED 4 // hdr | Id

/* hdr id for join request messages - ONLY FOR ROOT */
#define REQUEST 5

/* hdr id for Ring message - used as dummy/test messages */
#define RING 6

typedef struct{
    uint8_t msg_type;
} Header;

typedef struct{ 
    Header hdr;
    uint8_t Id1; // id 1
    uint8_t Id2; // id 2
    uip_ipaddr_t ipaddr; 
} Ip_msg;

typedef struct{
    Header hdr;
    uint8_t Id;
} Election_msg;

typedef struct{
    Header hdr;
    unsigned long ticks;
} Timestamp_msg;

typedef union{
    Header hdr; // hdr
    Ip_msg Ip_msg; // ip msg
    Timestamp_msg Timestamp_msg; // timestamp message
    Election_msg election_packet; // election msg
    uint8_t Byte[sizeof(Ip_msg)]; /* byte level access*/
} Ring_msg;

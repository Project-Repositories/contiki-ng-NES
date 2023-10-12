
#include "uip.h"

/* hdr id for message used to pass ip of a node trying to join the ring*/
#define PASS_IP 1 // hdr | Ip | Id

/* hdr id for messages used to tell node to drop its connection to its pred*/
#define DROP 2 // hdr

/* hdr id for messages used to connect to a node, and tell the noed what its new succor is*/
#define JOIN_SUCC 3  // hdr | Ip | Id | Id

/* hdr id for messages used to establish connection with the new pred*/
#define JOIN_PRE 4 // hdr

/* hdr id for election messages*/
#define ELECTION 5 // hdr | Id

/* hdr id for elected messages*/
#define ELECTED 6 // hdr | Id

typedef struct{
    uint8_t msg_type;
} Header;

typedef struct{ 
    Header hdr;
    uip_ipaddr_t ipaddr; 
    uint8_t Id1; // id 1
    uint8_t Id2; // id 2

} Ip_msg;
typedef struct{
    Header hdr;
    uint8_t Id;
} Election_msg;


typedef union{
    Header hdr; // hdr
    Ip_msg Ip_msg; // ip msg
    Election_msg election_packet; // election msg
    uint8_t Byte[sizeof(Ip_msg)]; /* byte level access*/
} Ring_msg;
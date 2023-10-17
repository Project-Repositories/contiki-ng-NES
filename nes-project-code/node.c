/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         A RPL+TSCH node able to act as either a simple node (6ln),
 *         DAG Root (6dr) or DAG Root with security (6dr-sec)
 *         Press use button at startup to configure.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */


#include "contiki.h"
#include "msg-format.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "uip.h"
#include <stdlib.h>
#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#include "tcp-socket.h"

#define TCP_PORT_ROOT 8080
#define TCP_PORT_IN 8091
#define TCP_PORT_OUT 8092
//DEFINE NODE
#define IS_ROOT false
// buffers
#define BUFSIZE sizeof(Ring_msg)
static uint8_t inputbuf[BUFSIZE];
static uint8_t outputbuf[BUFSIZE];
// sockets
static struct tcp_socket socket_in;
char socket_in_name[] = "socket_in";
static struct tcp_socket socket_out;
char socket_out_name[] = "socket_out";
#if IS_ROOT
  static uint8_t rootbuf[BUFSIZE];
  static struct tcp_socket socket_root;
  char socket_root_name[] = "socket_root";
  #define VALID_RING() (socket_out.c != NULL)
#endif


// TCP Documentation

/*---------------------------------------------------------------------------*/


#if IS_ROOT
  PROCESS(root_process, "RPL Root");
  AUTOSTART_PROCESSES(&root_process);
#else
  PROCESS(node_process, "RPL Node");
  AUTOSTART_PROCESSES(&node_process);
#endif

static int sem = 0;

// Get Hardware ID somehow - retrieve from header file?
// We could use MAC address to determine which node serves as the root/coordinator.
// or maybe develop a new custom shell command 
// shell_command_set_register(custom_shell_command_set);

// if node is root make gen
static int id;
static int id_next;
int gen_id(){
  //for now return static id
  return 2;
}





/*    callback functions    */

void event_callback(struct tcp_socket *s, void *ptr, tcp_socket_event_t event){
    PRINTF("SOCKET: %s | ", (char *)s->ptr);
    switch (event)
    {
    case TCP_SOCKET_CONNECTED:
      PRINTF("EVENT: TCP_SOCKET_CONNECTED \n");
      break;
    case TCP_SOCKET_CLOSED:
      PRINTF("EVENT: TCP_SOCKET_CLOSED \n");
      /* code */
      break;
    case TCP_SOCKET_TIMEDOUT:
      PRINTF("EVENT: TCP_SOCKET_TIMEDOUT \n");
      /* code */
      break;
    case TCP_SOCKET_ABORTED:
      PRINTF("EVENT: TCP_SOCKET_ABORTED \n");
      /* code */
      break;
    case TCP_SOCKET_DATA_SENT:
      PRINTF("EVENT: TCP_SOCKET_DATA_SENT \n");
      /* code */
      break;
    default:
      PRINTF("EVENT: ERROR_UNKOWN_EVENT \n");
      break;
    }

}

int data_callback(struct tcp_socket *s, void *ptr, const uint8_t *input_data_ptr, int input_data_len){
    // Think it should just consume data directly?
    
    Ring_msg* msg = (Ring_msg*)input_data_ptr;
    PRINTF("DATA CALLBACK: %d, LENGTH: %d \n", msg->hdr.msg_type, input_data_len);
    switch (msg->hdr.msg_type)
        {
        case PASS_IP: ;
          /* code */
          
          uint8_t Id1 = msg->Ip_msg.Id1;

          // pass message
          if (Id1 > id_next){
              tcp_socket_send(&socket_out, (uint8_t* )msg, sizeof(Ip_msg));
              break;
          }
          // else: insert node
          while(tcp_socket_close(&socket_out)== -1){
            PRINTF("ERROR: couldnt close socket out... \n");
          }



          break;
        case DROP:
          /* code */
          break;
        case JOIN_SUCC:
          /* CHECK LENGTH */
          if (input_data_len != sizeof(Ip_msg)){
            PRINTF("JOIN_SUCC: INVALID DATA LENGTH %d \n", input_data_len);
          }
          Ip_msg* ipMsg = (Ip_msg*) input_data_ptr;

          // connect to next node
          while(tcp_socket_connect(&socket_out, &ipMsg->ipaddr, TCP_PORT_IN)){
              PRINTF("TCP socket OUT connection failed... \n");
          }
          PRINTF("TCP socket connection succeeded! \n");
          id = ipMsg->Id1;
          id_next = ipMsg->Id2;

          break;
        case JOIN_PRE:
          /* code */
          break;
        case ELECTION:
          /* code */
          break;
        case ELECTED:
          /* code */
          break;
      #if IS_ROOT
        case REQUEST:
          // must be root
          PRINTF("Recieved 'REQUEST' message... \n");
          if (VALID_RING()){
              // Construct pass message
              Ip_msg* new_msg = (Ip_msg*) inputbuf;
              new_msg->hdr.msg_type = PASS_IP;
              new_msg->Id1 = 2; // random id
              memcpy(&new_msg->ipaddr, &msg->Ip_msg.ipaddr, sizeof(uip_ipaddr_t));
              
              /* send message on outcomming socket */
              PRINTF("SENDING MESSAGE HDR: %d \n", new_msg->hdr.msg_type);
              while(tcp_socket_send(&socket_out, (uint8_t*)new_msg, sizeof(Ip_msg))==-1){
                PRINTF("ERROR: sending message to first node failed... \n");
              }
              PRINTF("Succesfully send PASS_IP message to first node! \n");

          }
          else{
            // try and connect to first node
            while(tcp_socket_connect(&socket_out, &(msg->Ip_msg.ipaddr),TCP_PORT_IN) == -1){
              PRINTF("TCP socket connection failed... \n");
            }
            PRINTF("TCP socket connection succeeded! \n");
  
            //put new message into input buffer
            Ip_msg* new_msg = (Ip_msg*)malloc(sizeof(Ip_msg));
            new_msg->hdr.msg_type = JOIN_SUCC;
            new_msg->Id1 = 2; // random id
            new_msg->Id2 = id; // self id

            //store id for next node
            id_next = new_msg->Id1;
            memcpy(&new_msg->ipaddr, &socket_root.c->ripaddr, sizeof(uip_ipaddr_t));

            PRINTF("SENDING MESSAGE HDR: %d", new_msg->hdr.msg_type);
            while(tcp_socket_send(&socket_out, (uint8_t*) new_msg, sizeof(Ip_msg))==-1){
                PRINTF("ERROR: sending message to first node failed... \n");
            }
            PRINTF("Succesfully send join message to first node! \n");
            free(new_msg);

            // Close root socket, to allow other nodes to join
            while(tcp_socket_close(&socket_root) == -1){
              PRINTF("ERROR: failed to close 'root' socket \n");
            }
            PRINTF("Root socket connection closed... \n");
           
            sem = 1; /* something is in buffer*/

          }


          break;
      #endif
        default:
          PRINTF("UNDEFINED MESSAG HDR: %d \n", msg->hdr.msg_type);
          break;
        }


    return 0;
}
/*--------------------------------------------*/

// Node struct is (id,ip)
// Node succesor (id,ip)
// Node predecessor (id,ip)

#if !IS_ROOT

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  
  static struct etimer timer;

  PROCESS_BEGIN();

  NETSTACK_MAC.on();
  
  // register sockets
  while (-1 == tcp_socket_register(&socket_in, NULL, inputbuf, sizeof(inputbuf),NULL,0,data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'IN' failed... \n");
        PROCESS_PAUSE();
  }
  while (-1 == tcp_socket_register(&socket_out, NULL, NULL,0, outputbuf, sizeof(outputbuf),data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'OUT' failed... \n");
        PROCESS_PAUSE();
  }
  PRINTF("Sockets registered successfully!");
  uip_ipaddr_t dest_ipaddr;
  while(!NETSTACK_ROUTING.node_is_reachable || !NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)){
        PRINTF("ERROR: Node could not recieve 'root' IP... \n");
        PROCESS_PAUSE();
  }
  while (-1 ==tcp_socket_listen(&socket_in, TCP_PORT_IN)){
        PRINTF("ERROR: In socket failed to listen \n");
        PROCESS_PAUSE();
  }
  PRINTF("Node sending join message... \n");
  
  while(tcp_socket_connect(&socket_out, &dest_ipaddr, TCP_PORT_ROOT) == -1){
      PRINTF("ERROR: failed to connect to root... \n");
      PROCESS_PAUSE();
  }
  PRINTF("Node connected to root!");

  /* Create request message */
  Header msg;
  msg.msg_type = REQUEST;
  while(tcp_socket_send(&socket_out, (uint8_t*)&msg, sizeof(Header)) == -1){
      PRINTF("ERROR: failed to send REQUEST message \n");
      PROCESS_PAUSE();
  }
  PRINTF("Node sending join message succesfully! \n");


  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);

  

  while(1) {   
    PRINTF("Node is idle... \n");   
    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#endif //!IS_ROOT

#if IS_ROOT

PROCESS_THREAD(root_process, ev, data){ 
  static struct etimer timer;

  PROCESS_BEGIN();

  NETSTACK_ROUTING.root_start();
  NETSTACK_MAC.on();

  // register sockets
  while (-1 == tcp_socket_register(&socket_in, &socket_in_name, inputbuf, sizeof(inputbuf),NULL, 0,data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'IN' failed... \n");
        PROCESS_PAUSE();
  }
  while (-1 == tcp_socket_register(&socket_out, &socket_out_name, NULL, 0, outputbuf, sizeof(outputbuf),data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'OUT' failed... \n");
        PROCESS_PAUSE();
  }
  while (-1 == tcp_socket_register(&socket_root,&socket_root_name, rootbuf,sizeof(rootbuf), NULL, 0,data_callback, event_callback)){
        PRINTF("ERROR: Socket registration 'ROOT' failed... \n");
        PROCESS_PAUSE();
  }
  PRINTF("Sockets registered successfully! \n");

  while (-1 ==tcp_socket_listen(&socket_root, TCP_PORT_ROOT)){
        PRINTF("ERROR: Root socket failed to listen \n");
        PROCESS_PAUSE();
  }
  while (-1 ==tcp_socket_listen(&socket_in, TCP_PORT_IN)){
        PRINTF("ERROR: In socket failed to listen \n");
        PROCESS_PAUSE();
  }
  PRINTF("Root now listening for new nodes.. \n");

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);

  

  while(1) {      
    PRINTF("Root is idle... \n");
    PRINTF("RING STATUS: %d \n", VALID_RING());
    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }


    PROCESS_END();

}
#endif // IS_ROOT

/*---------------------------------------------------------------------------*/

/* should send the messages from the inputbuffer*/
/*-------------------------------------------------------------------------*/
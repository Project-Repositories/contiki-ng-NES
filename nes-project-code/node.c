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
#include "net/ipv6/uip-ds6.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "uip.h"
#include <stdlib.h>
#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#include "tcp-socket.h"


#define TCP_PORT_ROOT 8080
#define TCP_PORT_IN 8091

//DEFINE NODE
<<<<<<< HEAD
=======
#define N_NODES 5
>>>>>>> f2e5c7e34f4bc95d5af99ef8b4b6733938d17d19
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

static uip_ipaddr_t self_ip; // ip of the node



// TCP socket Documentation

/*---------------------------------------------------------------------------*/

PROCESS(status_process, "Sender");
#if IS_ROOT
  PROCESS(root_process, "RPL Root");
  AUTOSTART_PROCESSES(&root_process);
#else
  PROCESS(node_process, "RPL Node");
  AUTOSTART_PROCESSES(&node_process);
#endif

// Get Hardware ID somehow - retrieve from header file?
// We could use MAC address to determine which node serves as the root/coordinator.
// or maybe develop a new custom shell command 
// shell_command_set_register(custom_shell_command_set);

// if node is root make gen
static int id;
static int id_next;
#if IS_ROOT
static int IdArr[N_NODES];
static int current_n_nodes = 0;

bool intisinarray(int val, int* arr, size_t arr_len) {
  for (size_t i = 0; i < arr_len; i++) {
    if (val == arr[i]) {
      return true;
    } 
  }
  return false;
}


int gen_id(){
  //generates a random, new ID between 1 and 30
  int id =(rand() % 30) +1;
  while (intisinarray(id, IdArr, N_NODES)) {
    id =(rand() % 30) +1;
  }
  IdArr[current_n_nodes] = id;
  current_n_nodes++;
  return id;
}
#endif // IS_ROOT




/*    callback functions    */
static bool DISCONNECT_FLAG = false;
void event_callback(struct tcp_socket *s, void *ptr, tcp_socket_event_t event){
    PRINTF("SOCKET: %s | ", (char *)s->ptr);
    switch (event)
    {
    case TCP_SOCKET_CONNECTED:
      PRINTF("EVENT: TCP_SOCKET_CONNECTED \n");
      break;
    case TCP_SOCKET_CLOSED:
      PRINTF("EVENT: TCP_SOCKET_CLOSED \n");
      if (strcmp(s->ptr, socket_out_name)){
          DISCONNECT_FLAG = 1;
      }
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
    PRINTF("SOCKET %s | DATA CALLBACK: %d, LENGTH: %d \n", (char*) s->ptr, msg->hdr.msg_type, input_data_len);
    switch (msg->hdr.msg_type)
        {
        case RING: ;
          #if IS_ROOT
            PRINTF("SUCCESS! Ring message recieved! \n");
          #else // IS_ROOT
            /* pass message */          
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) msg, sizeof(Header))){
              PRINTF("ERROR: couldnt send 'RING' message... \n");
            }
            PRINTF("Passed 'RING' message!");

          #endif // not IS_ROOT
          break;
        case PASS_IP: ;
          /* code */
          
          uint8_t Id1 = msg->Ip_msg.Id1;
          // pass message
          if (Id1 > id_next){
            while(-1 == tcp_socket_send(&socket_out, (uint8_t* )msg, sizeof(Ip_msg))) {
              PRINTF("ERROR: Couldn't pass IP_msg forward... \n");
            }
            break;
          }
          else {
            // else: insert node
            while(-1 == tcp_socket_connect(&socket_out, &msg->Ip_msg.ipaddr, TCP_PORT_IN)){
              PRINTF("ERROR: couldnt connect to new node... \n");
            }
            /* Construct join_succ message to new node*/
            Ip_msg new_msg;
            new_msg.hdr.msg_type = JOIN_SUCC;
            new_msg.Id1 = Id1;
            new_msg.Id2 = id_next;
            id_next = Id1; // update id
            memcpy(&new_msg.ipaddr, &msg->Ip_msg, sizeof(uip_ipaddr_t));
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) &new_msg, sizeof(Ip_msg))){
                PRINTF("ERROR: Couldnt send 'JOIN_SUCC' to new node");
            }
            break;
          }
        case DROP:
          /* code */
          break;
        case JOIN_SUCC:
          /* CHECK LENGTH */
          if (input_data_len != sizeof(Ip_msg)){
            PRINTF("JOIN_SUCC: INVALID DATA LENGTH %d \n", input_data_len);
          }
          
          PRINTF("Switching socket connection... \n"); 
          Ip_msg* ipMsg = (Ip_msg*) input_data_ptr;
          // connect to next node
          while(-1 == tcp_socket_connect(&socket_out, &ipMsg->ipaddr, TCP_PORT_IN)){
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
              
              // Construct pass_ip message
              Ip_msg new_msg;
              new_msg.hdr.msg_type = PASS_IP;
              new_msg.Id1 = gen_id(); // random id
              memcpy(&new_msg.ipaddr,  &(msg->Ip_msg.ipaddr), sizeof(uip_ipaddr_t));
              
              PRINTF("SENDING PASS_IP MESSAGE, HDR: %d \n", new_msg.hdr.msg_type);
              while(-1 == tcp_socket_send(&socket_out, (uint8_t*) &new_msg, sizeof(Ip_msg))){
                PRINTF("ERROR: sending message to first node failed... \n");
              }
              PRINTF("Succesfully send PASS_IP message to first node! \n");

          }
          else{
            PRINTF("FIRST NODE JOINING! \n");
            // Close root socket, to allow other nodes to join
            // try and connect to first node
            PRINTF("REQUEST MESSAGE FROM NODE WITH IP: "); uiplib_ipaddr_print(&msg->Ip_msg.ipaddr); PRINTF(" |\n");
            while(-1 == tcp_socket_connect(&socket_out, &msg->Ip_msg.ipaddr,TCP_PORT_IN) ){
              PRINTF("TCP socket connection failed... \n");
            }
            //process_post(&root_process, PROCESS_EVENT_CONNECT, &msg->Ip_msg.ipaddr);
            PRINTF("TCP socket connection succeeded! \n");

            uip_ipaddr_t* ipaddrs =(uip_ipaddr_t*) malloc(sizeof(uip_ipaddr_t));
            memcpy(ipaddrs, &msg->Ip_msg.ipaddr, sizeof(uip_ipaddr_t));

            //put new message into input buffer
            Ip_msg new_msg;
            new_msg.hdr.msg_type = JOIN_SUCC;
            new_msg.Id1 = gen_id(); // random id
            new_msg.Id2 = id; // self id

            //store id for next node
            id_next = new_msg.Id1;
            memcpy(&new_msg.ipaddr, &self_ip, sizeof(uip_ipaddr_t));
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) &new_msg, sizeof(Ip_msg))){
              PRINTF("ERROR: sending message to first node failed... \n");
            }
            PRINTF("Succesfully send join message to first node! \n");
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

#if !IS_ROOT

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  PROCESS_BEGIN();

  NETSTACK_MAC.on();

  // register sockets
  while (-1 == tcp_socket_register(&socket_in, &socket_in_name, inputbuf, sizeof(inputbuf),NULL,0,data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'IN' failed... \n");
        PROCESS_PAUSE();
  }
  while (-1 == tcp_socket_register(&socket_out, &socket_out_name, NULL,0, outputbuf, sizeof(outputbuf),data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'OUT' failed... \n");
        PROCESS_PAUSE();
  }
  while (-1 == tcp_socket_listen(&socket_in, TCP_PORT_IN) ){
        PRINTF("ERROR: In socket failed to listen \n");
        PROCESS_PAUSE();
  }


  PRINTF("Sockets registered successfully!");
  uip_ipaddr_t dest_ipaddr;

  while(!NETSTACK_ROUTING.node_is_reachable || !NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)){
        PRINTF("ERROR: Node could not recieve 'root' IP... \n");
        PROCESS_PAUSE();
  }
  PRINTF("HOST THIS NODE: "); uiplib_ipaddr_print(&dest_ipaddr); PRINTF("\n");
    /* get ip addresss of itself*/
  memcpy(&self_ip, &uip_ds6_get_global(ADDR_PREFERRED)->ipaddr, sizeof(uip_ipaddr_t));
  PRINTF("IP THIS NODE: "); uiplib_ipaddr_print(&self_ip); PRINTF("\n");


  PRINTF("Node sending join message... \n");
  
  while(-1 == tcp_socket_connect(&socket_out, &dest_ipaddr, TCP_PORT_ROOT)){
      PRINTF("ERROR: failed to connect to root... \n");
      PROCESS_PAUSE();
  }
  PRINTF("Node connected to root! \n");
  /* Create request message */
  Ip_msg msg;
  msg.hdr.msg_type = REQUEST;
  memcpy(&msg.ipaddr, &self_ip, sizeof(uip_ipaddr_t));
  while(-1 == tcp_socket_send(&socket_out, (uint8_t*)&msg, sizeof(Ip_msg))){
      PRINTF("ERROR: failed to send REQUEST message \n");
      PROCESS_PAUSE();
  }
  PRINTF("Node sending join message succesfully! \n");

  /* Setup a periodic timer that expires after 10 seconds. */

  
    process_start(&status_process, NULL);
    PROCESS_END();
  }

/*---------------------------------------------------------------------------*/
#endif //!IS_ROOT

#if IS_ROOT

PROCESS_THREAD(root_process, ev, data){ 
  PROCESS_BEGIN();


  NETSTACK_ROUTING.root_start();
  NETSTACK_MAC.on();
  id = 0; // root id

  memcpy(&self_ip, &uip_ds6_get_global(ADDR_PREFERRED)->ipaddr, sizeof(uip_ipaddr_t));
  PRINTF("IP THIS NODE: "); uiplib_ipaddr_print(&self_ip); PRINTF("\n");


  // register sockets
  while (-1 == tcp_socket_register(&socket_in, &socket_in_name, inputbuf, sizeof(inputbuf),outputbuf, sizeof(outputbuf),data_callback,event_callback)){
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

  process_start(&status_process, NULL);

  PROCESS_END();

}
#endif // IS_ROOT

/*---------------------------------------------------------------------------*/

/* Status process, to print status for nodes, and spawn ring messages from root*/

PROCESS_THREAD(status_process, ev, data){
    static struct etimer timer;
    PROCESS_BEGIN();
    etimer_set(&timer, CLOCK_SECOND*10);
    while(1){
      PRINTF("RUNNING STATUS... \n SELF ID: %d \n NEXT ID: %d \n ", id, id_next);
      #if IS_ROOT
        PRINTF("RING STATUS: %d\n", VALID_RING());
        PRINTF("CURRENT NODS: %d\n", current_n_nodes);

        /* Spawn ring messages*/
        if (socket_out.c != NULL){
          PRINTF("Sending ring message...");
          Header msg;
          msg.msg_type = RING;
          while(-1 == tcp_socket_send(&socket_out, (uint8_t*) &msg, sizeof(Header))){
            PRINTF("ERROR: Couldnt spawn ring message from root...");
          }
        }

      #endif // IS_ROOT

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);

    }

    PROCESS_END();

}



/*-------------------------------------------------------------------------*/
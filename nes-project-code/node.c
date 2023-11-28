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
// Can the node-id file be removed, or is it used?
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

#include "dev/leds.h"



//Ports
#define TCP_PORT_ROOT 8080
#define TCP_PORT_IN 8091
//DEFINE NODE
#define ROOT_ID 0
#define N_NODES 5
#define IS_ROOT true
// buffers
#define BUFSIZE sizeof(Ring_msg)
static uint8_t inputbuf[BUFSIZE];
static uint8_t outputbuf[BUFSIZE];
// sockets
static struct tcp_socket socket_in;
char socket_in_name[] = "socket_in";
static struct tcp_socket socket_out;
char socket_out_name[] = "socket_out";

// ip of the node
static uip_ipaddr_t self_ip; 


#if IS_ROOT
  static uint8_t rootbuf[BUFSIZE];
  static struct tcp_socket socket_root;
  char socket_root_name[] = "socket_root";
  #define VALID_RING() (socket_out.c != NULL)

  // determines if STRESS_TEST will be run by the node. if disabled, latency of ring will be measured instead.
  #define STRESS_TEST false
  #if STRESS_TEST
    static unsigned long stress_test_id; // Use the timestamp to determine if a message is from the current stress_test 
    static unsigned int stress_test_n_received = 0; // counter for messages received during current stress_test 
    static int stress_n_msg = 20;
    static int stress_delay = 3;
    static int stress_timeout = 5;

  #endif //STRESS_TEST

#else // !IS_ROOT
  // determines if elections will be initialized by the node
  #define RUN_ELECTIONS true
  #if RUN_ELECTIONS
    // seconds between each node initiating the election process
    #define ELECTION_FREQUENCY 20
  #endif

  
  // Events
  static process_event_t NODE_TO_ROOT_SUCCESS_EVENT; // Custom event for when node has connected to root
  static process_event_t NODE_TO_ROOT_FAILED_EVENT; // Custom event for when node has failed to connect to root
#endif // !IS_ROOT


// Processes
PROCESS(status_process, "Sender");
#if IS_ROOT
  PROCESS(root_process, "RPL Root");
  AUTOSTART_PROCESSES(&root_process);
#else
  PROCESS(node_process, "RPL Node");
  AUTOSTART_PROCESSES(&node_process);
#endif


// Election
static bool is_participating = false;
static uint8_t elected = -1; // -1 = not defined

void set_participation(bool new_participation_status) {
  // GREEN LED is manually turned on for election winner, outside this function.
  is_participating = new_participation_status;
  if (new_participation_status) {
    leds_on(LEDS_RED);
    leds_off(LEDS_GREEN);
  }
  else {
    leds_off(LEDS_RED);
  }
  return;
}


/*---------------------------------------------------------------------------*/

// if node is root make gen -1 to say not set yet
static int id = -1;
static int id_next = -1;

#if IS_ROOT
  static int IdArr[N_NODES];
  static int current_n_nodes = 0;

  bool int_is_in_array(int val, int* arr, size_t arr_len) {
    for (size_t i = 0; i < arr_len; i++) {
      if (val == arr[i]) {
        return true;
      } 
    }
    return false;
  }

  int gen_id(){
    int id_list[] = {3,6,2,8,4};
    int new_id;
    
    //generates a random, new ID between 1 and 30
    /*
    int new_id;
    do {
    new_id =(rand() % 30) +1;
    }while (int_is_in_array(new_id, IdArr, N_NODES));
    IdArr[current_n_nodes] = new_id;
    */

    new_id = id_list[current_n_nodes];
    IdArr[current_n_nodes] = new_id;
    current_n_nodes++;
    return new_id;
  }
#endif // IS_ROOT
static struct etimer sleeptimer;
void wait(int seconds){
  etimer_set(&sleeptimer, CLOCK_SECOND*seconds);
}

/* Allocates and generates an Ip_msg and return a pointer to it.
   Remember to FREE msg after use!
*/
Ip_msg* gen_Ip_msg(uint8_t msgType, uint8_t Id1, uint8_t Id2,uip_ipaddr_t* ipaddr ){
    Ip_msg* new_msg = malloc(sizeof(Ip_msg));
    new_msg->hdr.msg_type = msgType;
    new_msg->Id1 = Id1;
    new_msg->Id2 = Id2;
    memcpy(&new_msg->ipaddr, ipaddr, sizeof(uip_ipaddr_t));
    
    return new_msg;
}

/* Allocates and generates an Election_msg and return a pointer to it.
   Remember to FREE msg after use!
*/
Election_msg* gen_Election_msg(uint8_t msgType, uint8_t Id) {
  Election_msg* new_msg = malloc(sizeof(Election_msg));
  new_msg->hdr.msg_type = msgType;
  new_msg->Id = Id;
  return new_msg;
}



/*    callback functions    */
void event_callback(struct tcp_socket *s, void *ptr, tcp_socket_event_t event){
    PRINTF("SOCKET: %s | ", (char *)s->ptr);
    switch (event)
    {
    case TCP_SOCKET_CONNECTED:
      PRINTF("EVENT: TCP_SOCKET_CONNECTED \n");

      /* If the callback comes from socket out
         Then we post an event to the process
      */
      #if !IS_ROOT
        if (strcmp((char*) s->ptr,"socket_out") == 0){
          int status = process_post(&node_process, NODE_TO_ROOT_SUCCESS_EVENT, NULL);
          if (status == PROCESS_ERR_OK){
            PRINTF("PROCESS_ERR_OK");
          }
          else{
            PRINTF("PROCESS_ERR_FULL");
          }
        }
      #endif
      break;
    case TCP_SOCKET_CLOSED:
      PRINTF("EVENT: TCP_SOCKET_CLOSED \n");
      /* code */
      break;
    case TCP_SOCKET_TIMEDOUT:
      PRINTF("EVENT: TCP_SOCKET_TIMEDOUT \n");
      /* code */

      #if !IS_ROOT
      if (strcmp((char*) s->ptr,"socket_out") == 0){
        int status = process_post(&node_process, NODE_TO_ROOT_FAILED_EVENT, NULL);
        if (status == PROCESS_ERR_OK){
          PRINTF("PROCESS_ERR_OK");
        }
        else{
          PRINTF("PROCESS_ERR_FULL");
        }
      }
      #endif
      break;
    case TCP_SOCKET_ABORTED:
      PRINTF("EVENT: TCP_SOCKET_ABORTED \n");
      /* code */
      #if !IS_ROOT
      if (strcmp((char*) s->ptr, "socket_out")==0){
        int status = process_post(&node_process, NODE_TO_ROOT_FAILED_EVENT, NULL);
        if (status == PROCESS_ERR_OK){
          PRINTF("PROCESS_ERR_OK");
        }
        else{
          PRINTF("PROCESS_ERR_FULL");
        }
      }
      #endif

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
            #if STRESS_TEST
              Timestamp_msg* timestampMsg = (Timestamp_msg*) input_data_ptr;
              unsigned long msg_ticks = timestampMsg->ticks;
              PRINTF("Timestamp MSG received, Timestamp: %lu \n ", msg_ticks);
              if (msg_ticks == stress_test_id) {
                stress_test_n_received++;
              }
            #else // !STRESS_TEST
              // Latency measurement
              PRINTF("SUCCESS! Ring message recieved! \n");
              Timestamp_msg* timestampMsg = (Timestamp_msg*) input_data_ptr;
              unsigned long msg_ticks = timestampMsg->ticks;
              PRINTF("Ticks since msg was sent : %lu \n ", clock_time() - msg_ticks);
            #endif // !STRESS_TEST

          #else // IS_ROOT
            /* pass message */          
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) msg, sizeof(Timestamp_msg))){
              PRINTF("ERROR: couldnt send 'RING' message... \n");
            }
            PRINTF("Passed 'RING' message! \n");

          #endif // !IS_ROOT
          break;
        case PASS_IP: ;
          /* code */
          int Id1 = msg->Ip_msg.Id1;
          // pass message
          PRINTF("PASS_IP, with ID: %d \n", Id1);
          if (Id1 > id_next && id_next != ROOT_ID) { // root edge case to avoid infinite message passing
            while(-1 == tcp_socket_send(&socket_out, (uint8_t* )msg, sizeof(Ip_msg))) {
              PRINTF("ERROR: Couldn't pass IP_msg forward... \n");
            }
            break;
          }
          else {
            /*
              Insert a new node, the new node needs the ip of whoever this node is currently connected to.
              Therefore it needs to construct a new ip_msg with that ip.
            */



            /* Construct join_succ message to new node*/
            Ip_msg* new_msg = gen_Ip_msg(JOIN_SUCC,Id1,id_next, &socket_out.c->ripaddr);

            while(-1 == tcp_socket_connect(&socket_out, &msg->Ip_msg.ipaddr, TCP_PORT_IN)){
              PRINTF("ERROR: couldnt connect to new node... \n");
            }
            id_next = Id1; // update id
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) new_msg, sizeof(Ip_msg))){
                PRINTF("ERROR: Couldnt send 'JOIN_SUCC' to new node");
            }
            free(new_msg);
            break;
          }
        case JOIN_SUCC:
          /* CHECK LENGTH */
          if (input_data_len != sizeof(Ip_msg)){
            PRINTF("JOIN_SUCC: INVALID DATA LENGTH %d \n", input_data_len);
          }
          
          PRINTF("Switching socket connection... \n"); 
          Ip_msg* ipMsg = (Ip_msg*) input_data_ptr;
          id = ipMsg->Id1;
          id_next = ipMsg->Id2;
          // connect to next node
          while(-1 == tcp_socket_connect(&socket_out, &ipMsg->ipaddr, TCP_PORT_IN)){
              PRINTF("TCP socket OUT connection failed... \n");
          }
          PRINTF("TCP socket connection succeeded! \n");
          



          break;
        case ELECTION: ;
          Election_msg* election_msg = (Election_msg*) input_data_ptr;
          uint8_t elect_msg_id = election_msg->Id;
          PRINTF("ELECTION MSG RECEIVED WITH ID: %d \n", elect_msg_id);

          if (elect_msg_id > id) {
            // forward to neighbor, mark itself as participant
            PRINTF("ELECTION: forwarding id... \n");
            Election_msg* msg = gen_Election_msg(ELECTION, elect_msg_id);
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) msg, sizeof(Election_msg))) {
              PRINTF("ERROR: Couldnt send election message from node... \n");
            }
            free(msg);
            set_participation(true);
            
          }
          else if ((elect_msg_id < id) && !is_participating) {
            // substitutes its own identifier in the message;
            // it forwards the message to its neighbour;
            // it marks itself as a participant 
            PRINTF("ELECTION: substituting id... \n");
            Election_msg* msg = gen_Election_msg(ELECTION, id);
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) msg, sizeof(Election_msg))) { 
              PRINTF("ERROR: Couldnt send election message from node... \n");
            }
            free(msg);
            set_participation(true);
          }
          else if ((elect_msg_id < id) && is_participating)
          {
              // discards the message (i.e., it does not forward the message) -> do nothing
              PRINTF("ELECTION: discarding msg... \n");
          }
          else { 
            // the received identifier is that of the receiver itself
            // -> this process’s identifier must be the greatest: coordinator
            // mark as non-participant
            // send elected msg
            PRINTF("ELECTION: chosen as coordinator... \n");
            PRINTF("received id: %d, own id: %d \n",elect_msg_id, id); // Has to be equal
            set_participation(false);
            leds_on(LEDS_GREEN);
            Election_msg* msg = gen_Election_msg(ELECTED, id);
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) msg, sizeof(Election_msg))) { 
              PRINTF("ERROR: Couldnt send election message from node... \n");
            }
            free(msg);
          }
          break;
        case ELECTED: ;
          Election_msg* elected_msg = (Election_msg*) input_data_ptr;

          set_participation(false);
          elected = elected_msg->Id;
          PRINTF("ELECTED: Winner is %d \n",elected);
          if (elected != id) {
            Election_msg* msg = gen_Election_msg(ELECTED, elected);
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) msg, sizeof(Election_msg))) { 
              PRINTF("ERROR: Couldnt send election message from node... \n");
            }
            free(msg);
          }
          break;
      #if IS_ROOT
        case REQUEST:
          // must be root
          PRINTF("Recieved 'REQUEST' message... \n");
          if (VALID_RING()){
              int new_node_id = gen_id();

              /* If the new node should be inserted somewhere after roots successor*/
              if (new_node_id > id_next){
                // Construct pass_ip message
                Ip_msg* new_msg = gen_Ip_msg(PASS_IP, new_node_id, -1, &msg->Ip_msg.ipaddr);
                
                PRINTF("SENDING PASS_IP MESSAGE, ID %d \n", new_node_id);
                while(-1 == tcp_socket_send(&socket_out, (uint8_t*) new_msg, sizeof(Ip_msg))){
                  PRINTF("ERROR: sending message to first node failed... \n");
                }
                PRINTF("Succesfully send PASS_IP message to first node! \n");
              }
              else{ 
                  /*
                   Insert a new node, the new node needs the ip of whoever this node is currently connected to.
                   Therefore it needs to construct a new ip_msg with that ip.
                   */

                  // Generate new msg, the ip should be whoever the root is current talking to on socket out
                  Ip_msg* new_msg = gen_Ip_msg(JOIN_SUCC,new_node_id,id_next, &socket_out.c->ripaddr);
                  //store id for next node
                  id_next = new_node_id;
                  PRINTF("FAILSAFE: "); uiplib_ipaddr_print(&socket_out.c->ripaddr); PRINTF("\n");
                  while(-1 == tcp_socket_connect(&socket_out, &msg->Ip_msg.ipaddr,TCP_PORT_IN) ){
                        PRINTF("TCP socket connection failed... \n");
                      }

                  while(-1 == tcp_socket_send(&socket_out, (uint8_t*) new_msg, sizeof(Ip_msg))){
                    PRINTF("ERROR: sending message to first node failed... \n");
                  }
                  PRINTF("Succesfully send join message to first node! \n");
                  free(new_msg);
              }


          }
          else{ // EDGE CASE - FIRST NODE JOINING RING
            PRINTF("FIRST NODE JOINING! \n");
            // Close root socket, to allow other nodes to join
            // try and connect to first node
            PRINTF("REQUEST MESSAGE FROM NODE WITH IP: "); uiplib_ipaddr_print(&msg->Ip_msg.ipaddr); PRINTF(" |\n");
            while(-1 == tcp_socket_connect(&socket_out, &msg->Ip_msg.ipaddr,TCP_PORT_IN) ){
              PRINTF("TCP socket connection failed... \n");
            }
            //process_post(&root_process, PROCESS_EVENT_CONNECT, &msg->Ip_msg.ipaddr);
            PRINTF("TCP socket connection succeeded! \n");


            //store id for next node
            id_next = gen_id();
            //Generate new message
            Ip_msg* new_msg = gen_Ip_msg(JOIN_SUCC,id_next,id, &self_ip);
            while(-1 == tcp_socket_send(&socket_out, (uint8_t*) new_msg, sizeof(Ip_msg))){
              PRINTF("ERROR: sending message to first node failed... \n");
            }
            PRINTF("Succesfully send join message to first node! \n");
            free(new_msg);
          }


          break;
      #endif
        default:
          PRINTF("UNDEFINED MESSAGE HDR: %d \n", msg->hdr.msg_type);
          break;
        }


    return 0;
}
/*--------------------------------------------*/

#if !IS_ROOT

/*--------------------------------NODE-------------------------------------*/

PROCESS_THREAD(node_process, ev, data)
{
  long ticks = clock_time();
  long timestamp = clock_seconds();
  PRINTF("ticks | timestamp = %lu | %lu \n !!!",ticks,timestamp);
  static struct etimer sleep_timer;
  PROCESS_BEGIN();
  NODE_TO_ROOT_FAILED_EVENT = process_alloc_event(); // allocate event number
  NODE_TO_ROOT_SUCCESS_EVENT = process_alloc_event(); // allocate event number

  NETSTACK_MAC.on();


  // Peripheral LEDs for 26XX. Both enabled until it joins the network
  leds_on(LEDS_GREEN);
  leds_on(LEDS_RED);

  // register sockets
  while (-1 == tcp_socket_register(&socket_in, &socket_in_name, inputbuf, sizeof(inputbuf),NULL,0,data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'IN' failed... \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
        ticks = clock_time();
        timestamp = clock_seconds();
        PRINTF("ticks | timestamp = %lu | %lu \n !!!",ticks,timestamp);

  }
  while (-1 == tcp_socket_register(&socket_out, &socket_out_name, NULL,0, outputbuf, sizeof(outputbuf),data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'OUT' failed... \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  }
  while (-1 == tcp_socket_listen(&socket_in, TCP_PORT_IN) ){
        PRINTF("ERROR: In socket failed to listen \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  }


  PRINTF("Sockets registered successfully! \n");
  uip_ipaddr_t dest_ipaddr;
  
  while(!NETSTACK_ROUTING.node_is_reachable || !NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)){
        PRINTF("ERROR: Node could not recieve 'root' IP... \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
        
        
  }
  PRINTF("HOST THIS NODE: "); uiplib_ipaddr_print(&dest_ipaddr); PRINTF("\n");
    /* get ip addresss of itself*/
  memcpy(&self_ip, &uip_ds6_get_global(ADDR_PREFERRED)->ipaddr, sizeof(uip_ipaddr_t));
  PRINTF("IP THIS NODE: "); uiplib_ipaddr_print(&self_ip); PRINTF("\n");


  PRINTF("Node sending join message... \n");
  

  /* Need to wait for the callback of tcp_socket_connect to see its connect to root, as the socket cuould be taken.
    Here we use a custom process event to signal from the event callback.
  */
  while (true){
    while(-1 == tcp_socket_connect(&socket_out, &dest_ipaddr, TCP_PORT_ROOT)){
        PRINTF("ERROR: failed to send connection request to root... \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
    }
    PRINTF("Node send connection request to root! \n");
    PROCESS_WAIT_EVENT_UNTIL(ev == NODE_TO_ROOT_FAILED_EVENT || ev == NODE_TO_ROOT_SUCCESS_EVENT);
    if (ev == NODE_TO_ROOT_SUCCESS_EVENT ){
       //dont try and reconnect - hop out of while loop
       break;
    }
    else if (ev == NODE_TO_ROOT_FAILED_EVENT){
      PRINTF("Node failed to connect to root, will try again... \n");
    }
    else{
      PRINTF("Undefined event... something went wrong? \n");
    }
  }
  PRINTF("Node has connected to root successfully! \n");


  /* Create request message */
  Ip_msg msg;
  msg.hdr.msg_type = REQUEST;
  memcpy(&msg.ipaddr, &self_ip, sizeof(uip_ipaddr_t));
  while(-1 == tcp_socket_send(&socket_out, (uint8_t*)&msg, sizeof(Ip_msg))){
      PRINTF("ERROR: failed to send REQUEST message \n");
      wait(1);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  }
  PRINTF("Node sending join message succesfully! \n");

  // Disable both LEDs, now that it has joined the network.
  leds_off(LEDS_GREEN);
  leds_off(LEDS_RED);
  /* Setup a periodic timer that expires after 10 seconds. */


    process_start(&status_process, NULL);
    PROCESS_END();
  }

#endif //!IS_ROOT

#if IS_ROOT
/*--------------------------------ROOT-----------------------------------*/
PROCESS_THREAD(root_process, ev, data){ 
  PROCESS_BEGIN();
  static struct etimer sleep_timer;

  NETSTACK_ROUTING.root_start();
  NETSTACK_MAC.on();
  id = ROOT_ID; // root id

  memcpy(&self_ip, &uip_ds6_get_global(ADDR_PREFERRED)->ipaddr, sizeof(uip_ipaddr_t));
  PRINTF("IP THIS NODE: "); uiplib_ipaddr_print(&self_ip); PRINTF("\n");


  // register sockets
  while (-1 == tcp_socket_register(&socket_in, &socket_in_name, inputbuf, sizeof(inputbuf),outputbuf, sizeof(outputbuf),data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'IN' failed... \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  }
  while (-1 == tcp_socket_register(&socket_out, &socket_out_name, NULL, 0, outputbuf, sizeof(outputbuf),data_callback,event_callback)){
        PRINTF("ERROR: Socket registration 'OUT' failed... \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  }
  while (-1 == tcp_socket_register(&socket_root,&socket_root_name, rootbuf,sizeof(rootbuf), NULL, 0,data_callback, event_callback)){
        PRINTF("ERROR: Socket registration 'ROOT' failed... \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  }
  PRINTF("Sockets registered successfully! \n");

  while (-1 ==tcp_socket_listen(&socket_root, TCP_PORT_ROOT)){
        PRINTF("ERROR: Root socket failed to listen \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  }
  while (-1 == tcp_socket_listen(&socket_in, TCP_PORT_IN)){
        PRINTF("ERROR: In socket failed to listen \n");
        wait(1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
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
    #if (!IS_ROOT) && RUN_ELECTIONS
      static struct etimer election_timer;
    #elif STRESS_TEST
      static int j = 0;
      static Timestamp_msg* new_msg; //= malloc(sizeof(Timestamp_msg));
      static struct etimer stress_timeout_timer;
      static struct etimer stress_delay_timer;
      
    #endif // !IS_ROOT || STRESS_TEST  
    PROCESS_BEGIN();
    etimer_set(&timer, CLOCK_SECOND*20);
    
    #if (!IS_ROOT) && RUN_ELECTIONS
      etimer_set(&election_timer, CLOCK_SECOND * ELECTION_FREQUENCY);
    #elif STRESS_TEST
      etimer_set(&stress_timeout_timer, CLOCK_SECOND*stress_timeout);
      etimer_set(&stress_delay_timer, CLOCK_SECOND>>stress_delay);
    #endif


    while(1){
      if(id == -1 && id_next == -1){
        PRINTF("RUNNING STATUS... \n SELF ID: NOT SET BY ROOT \n NEXT ID: NOT SET BY ROOT \n ");
      }
      PRINTF("RUNNING STATUS... \n SELF ID: %d \n NEXT ID: %d \n ", id, id_next);
      PRINTF("participating in election? %d \n",is_participating);
      #if IS_ROOT
        PRINTF("RING STATUS: %d\n", VALID_RING());
        PRINTF("CURRENT NODES: %d\n", current_n_nodes);

        #if STRESS_TEST
          if (socket_out.c != NULL){
            // When the ring is fully formed, send numerous messages around the ring and keep count of how many get through 
            stress_test_n_received = 0;
            stress_test_id = clock_time();

            PRINTF("# of MSG | TIMEOUT : %d | %d  \n", stress_n_msg, stress_timeout);
            PRINTF("STRESS_TEST ID: %lu \n", stress_test_id);
            
            new_msg = malloc(sizeof(Timestamp_msg));
            new_msg->hdr.msg_type = RING;
            new_msg->ticks = stress_test_id; 
            j = 0;
            for (j=0; j<stress_n_msg;j++) {

              PRINTF("Sending ring message... \n");
                while(-1 == tcp_socket_send(&socket_out, (uint8_t*) new_msg, sizeof(Timestamp_msg))){
                  PRINTF("ERROR: Couldnt spawn ring message from root... \n");
                }
                
                
                etimer_set(&stress_delay_timer,CLOCK_SECOND>>stress_delay);
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&stress_delay_timer));
                
              }
              
            free(new_msg);
            etimer_set(&stress_timeout_timer,CLOCK_SECOND*stress_timeout);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&stress_timeout_timer));
            PRINTF("%d MESSAGES RECEIVED / %d MESSAGES SENT \n",stress_test_n_received,stress_n_msg);
          }
        #else // !STRESS_TEST
        /* Spawn ring messages that determines the latency*/
        if (socket_out.c != NULL){
          PRINTF("Sending ring message... \n");
          Timestamp_msg* new_msg = malloc(sizeof(Timestamp_msg));
          new_msg->hdr.msg_type = RING;
          new_msg->ticks = clock_time();
          while(-1 == tcp_socket_send(&socket_out, (uint8_t*) new_msg, sizeof(Timestamp_msg))){
            PRINTF("ERROR: Couldnt spawn ring message from root... \n");
          }
          free(new_msg);
        }
        #endif // !STRESS_TEST

      #elif (!IS_ROOT) && RUN_ELECTIONS
      if ((-1 != id) && (-1 != id_next) && etimer_expired(&election_timer) && (!is_participating)) {
        /* Start election message*/
        if (socket_out.c != NULL) {
          PRINTF("Sending first election message... \n");
          Election_msg* msg = gen_Election_msg(ELECTION, id);
          while(-1 == tcp_socket_send(&socket_out, (uint8_t*) msg, sizeof(Election_msg))) { 
            PRINTF("ERROR: Couldnt send election message from node... \n");
          }
          free(msg);
          set_participation(true);
        }
        etimer_reset(&election_timer);
      }
      #endif // !IS_ROOT
      PRINTF("status resetting .... \n");
      etimer_reset(&timer);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    PROCESS_END();
}
/*-------------------------------------------------------------------------*/

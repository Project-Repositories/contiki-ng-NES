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

#include "sys/node-id.h"
#include "sys/log.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#include "tcp-socket.h"

#define TCP_PORT_ROOT 8080

// TCP Documentation

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "RPL Node");
AUTOSTART_PROCESSES(&node_process);


// Get Hardware ID somehow - retrieve from header file?
// We could use MAC address to determine which node serves as the root/coordinator.
// or maybe develop a new custom shell command 
// shell_command_set_register(custom_shell_command_set);




/*
// Ring topology:
typedef struct Node{
  uint8_t id;
  const uip_ipaddr_t ip;
} Node;

Node predecessor;
Node successor;


bool is_in_ring() {
  return &predecessor != NULL && &successor != NULL;
} 
*/


// Node struct is (id,ip)
// Node succesor (id,ip)
// Node predecessor (id,ip)

void join_ring(uip_ipaddr_t *dest_ipaddr) {
    struct tcp_socket out;
    tcp_socket_register(&out,NULL, NULL, 0, NULL, 0, NULL, NULL);
    int res = tcp_socket_connect(&out, dest_ipaddr, TCP_PORT_ROOT);
    if (res==-1) {
      printf("failure1");
      return;
    }

    char test[] = "test";
    uint8_t *dataptr = (uint8_t*)&test; 
    res = tcp_socket_send(&out,dataptr,sizeof(test));
    if (res==-1) {
      printf("failure2");
    }
    printf("success!!!");
    

    // Send a "join" packet to the known_node (the root)
    // open a TCP socket at its own IP address
    // Listen, wait to be contacted by its predecessor.     
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  
  static struct etimer timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);

  NETSTACK_MAC.on();


  while(1) {
      uip_ipaddr_t dest_ipaddr;
      if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.  (&dest_ipaddr)) {
        join_ring(&dest_ipaddr);
      }

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }


  PROCESS_END();
}




/*
void stabilize()

void handle_tcp_message(packet )
    // JOIN1: initial packet from node wanting to join the ring network
    // JOIN2: packet from root node to predecessor of new node
    // JOIN3: packet from predecessor to new node

    // stabilize
*/



/*---------------------------------------------------------------------------*/

// simple_ipc_server.c
   
#include <stdio.h>
#include <stdlib.h>
#include "simple_ipc_common.h"
   
int
factorial(int n)
{
    if (n < 1)
        return 1;
    else return n * factorial(n - 1);
}
   
int
main(int argc, char **argv)
{
    kern_return_t      kr;
    msg_format_recv_t  recv_msg;
    msg_format_send_t  send_msg;
    mach_msg_header_t *recv_hdr, *send_hdr;
    mach_port_t        server_port;


  mach_port_t self_task = mach_task_self();

  mach_port_t port_;
  kr = mach_port_allocate(self_task,
                                    MACH_PORT_RIGHT_RECEIVE,
                                    &port_);
  if (kr != KERN_SUCCESS) {
    EXIT_ON_MACH_ERROR("mach_port_allocate(): ", kr, BOOTSTRAP_SUCCESS);
    return kr;
  }


  kr = mach_port_insert_right(self_task,
                                        port_,
                                        port_,
                                        MACH_MSG_TYPE_MAKE_SEND);
  if (kr != KERN_SUCCESS) {
    EXIT_ON_MACH_ERROR("mach_port_insert_right(): ", kr, BOOTSTRAP_SUCCESS);
    return kr;
  }


  mach_port_t bootstrap_port;
  kr = task_get_bootstrap_port(self_task, &bootstrap_port);
  if (kr != KERN_SUCCESS) {
    EXIT_ON_MACH_ERROR("task_get_bootstrap_port(): ", kr, BOOTSTRAP_SUCCESS);
    return kr;
  }

    kr = bootstrap_register(bootstrap_port, SERVICE_NAME, port_);
    EXIT_ON_MACH_ERROR("bootstrap_register", kr, BOOTSTRAP_SUCCESS);
   
    server_port = port_;
    printf("server_port = %d\n", server_port);
   
    for (;;) { // server loop
   
        // receive message
        recv_hdr                  = &(recv_msg.header);
        recv_hdr->msgh_local_port = server_port;
        recv_hdr->msgh_size       = sizeof(recv_msg);
        kr = mach_msg(recv_hdr,              // message buffer
                      MACH_RCV_MSG,          // option indicating receive
                      0,                     // send size
                      recv_hdr->msgh_size,   // size of header + body
                      server_port,           // receive name
                      MACH_MSG_TIMEOUT_NONE, // no timeout, wait forever
                      MACH_PORT_NULL);       // no notification port
        EXIT_ON_MACH_ERROR("mach_msg(recv)", kr, MACH_MSG_SUCCESS);
  /* 
        printf("recv data = %d, id = %d, local_port = %d, remote_port = %d\n",
               recv_msg.data, recv_hdr->msgh_id,
               recv_hdr->msgh_local_port, recv_hdr->msgh_remote_port);
   */
        // process message and prepare reply
        send_hdr                   = &(send_msg.header);
        send_hdr->msgh_bits        = MACH_MSGH_BITS_LOCAL(recv_hdr->msgh_bits);
        send_hdr->msgh_size        = sizeof(send_msg);
        send_hdr->msgh_local_port  = MACH_PORT_NULL;
        send_hdr->msgh_remote_port = recv_hdr->msgh_remote_port;
        send_hdr->msgh_id          = recv_hdr->msgh_id;
        send_msg.data              = factorial(recv_msg.data);
   
        // send message
        kr = mach_msg(send_hdr,              // message buffer
                      MACH_SEND_MSG,         // option indicating send
                      send_hdr->msgh_size,   // size of header + body
                      0,                     // receive limit
                      MACH_PORT_NULL,        // receive name
                      MACH_MSG_TIMEOUT_NONE, // no timeout, wait forever
                      MACH_PORT_NULL);       // no notification port
        EXIT_ON_MACH_ERROR("mach_msg(send)", kr, MACH_MSG_SUCCESS);
   
    }
   
    exit(0);
}

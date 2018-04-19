#ifndef __init_h__
#define __init_h__

struct addrinfo* bind_to_first_address(int *server_sock, struct addrinfo *servinfo);

int init(unsigned short port);

#endif
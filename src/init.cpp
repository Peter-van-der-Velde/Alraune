/*
                __
 __          __/\ \__
/\_\    ___ /\_\ \ ,_\
\/\ \ /' _ `\/\ \ \ \/
 \ \ \/\ \/\ \ \ \ \ \_
  \ \_\ \_\ \_\ \_\ \__\
   \/_/\/_/\/_/\/_/\/__/

*/

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "init.h"
#include "dbg.h"

#define BACKLOG 10 // how many pending connections queue will hold


/*
 * bind to first usable address
 */
struct addrinfo* bind_to_first_address(int *server_sock, struct addrinfo *servinfo)
{
	struct addrinfo *rp;
	const int yes=1; // needed to set the value in the setsockopt function

	for(rp = servinfo; rp != NULL; rp = rp->ai_next) {

		if ((*server_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		// sets socket to reuse the address
		check(setsockopt(*server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != -1, "setsockopt");

		if (bind(*server_sock, rp->ai_addr, rp->ai_addrlen) == -1) {
			close(*server_sock);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure
	return rp;

error:
	freeaddrinfo(servinfo);
	exit(1);
}


/*
 * initialize a listening socket on the 'port' you specified in the argument
 */
int init(int port)
{
	struct addrinfo hints, *servinfo;
	int server_sock = -1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use either ipv4 or ipv6
	hints.ai_socktype = SOCK_STREAM; // sets it to a tcp socket
	hints.ai_flags = AI_PASSIVE; // use my IP

	char port_str[12];
	sprintf(port_str, "%d", port);

	int rv;
	if ((rv = getaddrinfo(NULL, port_str, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// bind to first usable address
	struct addrinfo *rp = bind_to_first_address(&server_sock, servinfo);
	check(rp != NULL, "server: failed to bind\n")

	check(listen(server_sock, BACKLOG) != -1, "listening error");

	return server_sock;

error:
	close(server_sock);
	exit(1);
}

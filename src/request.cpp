/*                                         __
					  /\ \__
 _ __    __     __   __  __     __    ____\ \ ,_\
/\`'__\/'__`\ /'__`\/\ \/\ \  /'__`\ /',__\\ \ \/
\ \ \//\  __//\ \L\ \ \ \_\ \/\  __//\__, `\\ \ \_
 \ \_\\ \____\ \___, \ \____/\ \____\/\____/ \ \__\
  \/_/ \/____/\/___/\ \/___/  \/____/\/___/   \/__/
		   \ \_\
		    \/_/
*/

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cctype>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "dbg.h"
#include "request.h"

#define SERVER_INFO "Server: Alraune v0.8.1\r\n"
#define FILE_BUFFER_SIZE 8096


/*
 * this ensures that all bytes will be send,
 * because sometimes send() does't send the full buffer.
 */
int send_all(int sockfd, char *buf, int *len)
{
	int total_send = 0;

	int bytesleft = *len;
	int n;
	while(total_send < *len) {
		n = send(sockfd, buf+total_send, bytesleft, 0);
		if (n == -1) { break; }
		total_send += n;
		bytesleft -= n;
	}

	// debug("sck: %d", sockfd);
	// debug("buf: %s", buf);
	// debug("len: %d", *len);
	// debug("  n: %d", n);
	// debug("tls: %d", total_send);

	*len = total_send; // return number actually sent here
	return (n==-1) ? -1 : 0; // return -1 on failure, 0 on success
}


/*
 * reads a single line from the socket
 */
int read_line(int sockfd, char *buf, int len)
{
	int i = 0;
	char c = '\a';
	int n;

	for (i = 0;(i < len - 1) && (c != '\n'); i++) {
		n = recv(sockfd, &c, 1, 0);

		if (n <= 0) {
			break;
		}
		else {
			if (c == '\r') {
				n = recv(sockfd, &c, 1, MSG_PEEK); // peek at the first char of the queue

				if (n > 0 && c == '\n')
					recv(sockfd, &c, 1, 0);

				c = '\n';
			}
			buf[i] = c;
		}
	}
	buf[i] = '\0';

	return i;
}


/*
 * handles the incoming requests
 */
void handle_request(int client_sock)
{
	char buf[1024];
	char url[255];

	int len;
	len = read_line(client_sock, buf, sizeof buf);
	debug("%s", buf);

	switch(buf[0]) {
		case 'G': // GET
			get_request(client_sock, buf);
			break;
		case 'H': // HEAD
			not_implemented_501(client_sock);
			break;
		case 'P':
			if(buf[1] == 'O')      // POST
				printf("POST");
			else if(buf[1] == 'U') // PUT
				printf("PUT");
			else if(buf[1] == 'A') // PATCH
				printf("PATCH");
			not_implemented_501(client_sock);
			break;
		case 'D': // DELETE
			not_implemented_501(client_sock);
			break;
		case 'T': // TRACE
			not_implemented_501(client_sock);
			break;
		case 'O': // OPTIONS
			not_implemented_501(client_sock);
			break;
		case 'C': // CONNECT
			not_implemented_501(client_sock);
			break;
		default:
			not_implemented_501(client_sock);
			sentinel("unimplemented request: %s", buf);
			break;
	}

error:
	return;
}


/*
 * get the url from the request
 */
int get_url(char* request, char* url, int url_len)
{
	int first_space = 0;
	int i = 0;

	for (int j = 1; i < url_len && request[j] != '\0' && request[j] != '\n' && request[j] != '?'; j++) {
		if (isspace((int) request[j])) {

			if (first_space == 0) {
				first_space = 1;
				j++; // skip first character (the first '/') after the first space
				continue;
			}
			else
				break;
		}

		if (first_space) {
			if (request[j] == '%') { // if the character is a special html url character translate it to a readable one
				log_warn("You still have to implement special html characters");
			}

			url[i] = request[j];
			i++;
		}
	}
	url[i] = '\0';

	if (i < 3) // if there was no specified page requested, set the the url to index.html
		sprintf(url, "index.html");

	return 0;
}


/*
 * gets the file length, what did you expect?
 */
int get_file_length(FILE* fp)
{
	int f_strsize;
	if (fp) {
		fseek(fp, 0, SEEK_END);
		f_strsize = ftell(fp); // get the length of the file
		rewind(fp); // reset fp to the start of the file

	}
	return f_strsize;
}


/*
 * reverses a string
 */
char* reverse_string(char *str)
{
    char temp;
    int len = strlen(str) - 1;
    if (len == -1) // if it is an empty string, just return
    	return str;
    int i;
    int j = len;

    for(i = 0; i <= len / 2; i++)
    {
        temp = str[j];
        str[j] = str[i];
        str[i] = temp;
        j--;
    }
}


/*
 * get the content type of a file
 */
void get_content_type(char* buf, char* fpath, int buf_size)
{
	char filetype[20];
	int fpath_length = strlen(fpath);
	int found_dot = 0;
	int i = 0;
	for(int j = fpath_length-1; j > 0 && i < 20; j--) {
		if (fpath[j] != '.') {
			filetype[i] = fpath[j];
			i++;
		}
		else {
			filetype[i] = '\0';
			found_dot = 1;
			break;
		}
	}
	filetype[19] = '\0';
	reverse_string(filetype);
	debug("%s", filetype);

	if (!found_dot) {
		strcpy(buf, "text/plain");
	}
	else if (strcasecmp(filetype, "css") == 0) {
		strcpy(buf, "text/css");
	}
	else if (strcasecmp(filetype, "html") == 0) {
		strcpy(buf, "text/html");
	}
	else if (strcasecmp(filetype, "htm") == 0) {
		strcpy(buf, "text/html");
	}
	else if (strcasecmp(filetype, "js") == 0) {
		strcpy(buf, "text/javascript");
	}
	else if (strcasecmp(filetype, "gif") == 0) {
		strcpy(buf, "image/gif");
	}
	else if (strcasecmp(filetype, "jpeg") == 0) {
		strcpy(buf, "image/jpeg");
	}
	else if (strcasecmp(filetype, "jpg") == 0) {
		strcpy(buf, "image/jpg");
	}
	else if (strcasecmp(filetype, "bmp") == 0) {
		strcpy(buf, "image/bmp");
	}
	else if (strcasecmp(filetype, "webp") == 0) {
		strcpy(buf, "image/webp");
	}
	else if (strcasecmp(filetype, "svg") == 0) {
		strcpy(buf, "image/svg+xml");
	}
	else if (strcasecmp(filetype, "ico") == 0) {
		strcpy(buf, "image/x-icon");
	}
	else if (strcasecmp(filetype, "wav") == 0) {
		strcpy(buf, "audio/wave");
	}
	else if (strcasecmp(filetype, "webm") == 0) {
		strcpy(buf, "video/webm");
	}
	else if (strcasecmp(filetype, "ogg") == 0) {
		strcpy(buf, "application/ogg");
	}
	else if (strcasecmp(filetype, "midi") == 0) {
		strcpy(buf, "audio/midi");
	}
	else if (strcasecmp(filetype, "mp3") == 0) {
		strcpy(buf, "application/mpeg");
	}
	else if (strcasecmp(filetype, "pdf") == 0) {
		strcpy(buf, "application/pdf");
	}
	else if (strcasecmp(filetype, "md") == 0) {
		strcpy(buf, "text/plain");
	}
	else if (strcasecmp(filetype, "txt") == 0) {
		strcpy(buf, "text/plain");
	}
	else {
		log_warn("not implemented file type: %s", filetype);
		strcpy(buf, "application/octet-stream");
	}

	return;
}

/*
 * takes care of the get request
 */
void get_request(int client_sock, char* request)
{
	char url[255];
	char fpath[255];

	int rc = get_url(request, url, sizeof(url));
	check(rc == 0, "Something went wrong when getting the url of request");
	debug("fpath: ./%s", url);

	// add ./ to the start of the requested path
	// this is done so that the client can't reach outside of the root_folder the server has selected
	// i.o.w. it can't acces your system files any more
	sprintf(fpath, "./%s", url);

	if (strstr(fpath, "..") != NULL) { // we can't have clients access anything outside the "root_folder".
		forbidden_403(client_sock);
		goto error;
	}

	rc = ok_200(client_sock, fpath);
	check(rc >= 0, "something went wrong while responding with OK 200");

	sleep(1); // stupid hack wich solves the "connection is reset" errors atleast on linux;
	rc = close(client_sock);
	if (rc < 0)
		log_err("socketfd failed to close.");
	return;

error:
	log_warn("f: %s", fpath);
	sleep(1); // stupid hack wich solves the "connection is reset" errors atleast on linux;
	rc = close(client_sock);
	if (rc < 0)
		log_err("socketfd failed to close.");
	return;
}


/*
 * responds with a 200 OK message and the requested file
 */
int ok_200(int client_sock, char* fpath)
{
	int f_strsize;
	FILE * fp;
	fp = fopen(fpath, "rb");

	if (!fp) { // if you cannot read the file return an 404 error
		not_found_404(client_sock);
		return -1;
	}
	f_strsize = get_file_length(fp);

	char buf[1024];
	char type_buf[20];
	int len;
	strcpy(buf, "HTTP/1.1 200 OK\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, SERVER_INFO);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	sprintf(buf, "Content-Length: %d\r\n", f_strsize);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "Connection: close\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	get_content_type(type_buf, fpath, sizeof type_buf); // get content type
	sprintf(buf, "Content-Type: %s\r\n", type_buf);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);

	fclose(fp);

	int fd;
	fd = open(fpath,O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	static char buffer[FILE_BUFFER_SIZE+1];
	int ret;
	while (	(ret = read(fd, buffer, FILE_BUFFER_SIZE)) > 0 ) {
		write(client_sock, buffer, ret);
	}

	return 0;
}


/*
 * responds with a 403 FORBIDDEN message
 */
void forbidden_403(int client_sock)
{
	int cont_len;
	// first get the content copied so that we know the length of the content
	char content[1024];
	strcpy(content, "<html><body><H1>403 forbidden</H1></body></html>\r\n");
	cont_len = strlen(content);

	char buf[1024];
	int len;

	strcpy(buf, "HTTP/1.1 403 FORBIDDEN\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, SERVER_INFO);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	sprintf(buf, "Content-length: %d\r\n", cont_len);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "Connection: close\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "Content-Type: text/html\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);

	// send actual content
	send_all(client_sock, content, &cont_len);

	return;
}


/*
 * responds with a 404 NOT FOUND message
 */
void not_found_404(int client_sock)
{
	int cont_len;
	// first get the content copied so that we know the length of the content
	char content[1024];
	strcpy(content, "<html><body><H1>404 Not found</H1></body></html>\r\n");
	cont_len = strlen(content);
	char buf[1024];
	int len;
	strcpy(buf, "HTTP/1.1 404 NOT FOUND\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, SERVER_INFO);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	sprintf(buf, "Content-length: %d\r\n", cont_len);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "Connection: close\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "Content-Type: text/html\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);

	// send actual content
	send_all(client_sock, content, &cont_len);

	return;
}


/*
 * responds with a 501 METHOD NOT IMPLEMENTED message
 */
void not_implemented_501(int client_sock)
{
	int cont_len;
	// first get the content copied so that we know the length of the content
	char content[1024];
	strcpy(content, "<html><body><H1>501 Method not implemented</H1></body></html>\r\n");
	cont_len = strlen(content);

	char buf[1024];
	int len;
	strcpy(buf, "HTTP/1.1 501 Method Not Implemented\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, SERVER_INFO);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	sprintf(buf, "Content-length: %d\r\n", cont_len);
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "Connection: close\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "Content-Type: text/html\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);
	strcpy(buf, "\r\n");
	len = strlen(buf);
	send_all(client_sock, buf, &len);

	// send actual content
	send_all(client_sock, content, &cont_len);
	return;
}


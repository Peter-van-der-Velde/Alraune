#ifndef __reqeust_h__
#define __reqeust_h__

#include <cstdio>

int send_all(int socketfd, char* buf, int* len);
void handle_request(int client_socket);

void get_request(int client_sock, char* buf);
int get_url(char* request, char* url, int url_len);
int get_file_length(FILE* fp);

int ok_200(int client_sock, char* fpath);
void forbidden_403(int client_sock);
void not_found_404(int client_sock);
void not_implemented_501(int client_sock);

#endif

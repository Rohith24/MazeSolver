#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
struct node
{
	char msg[128];
	int msg_id;
	node *next;
}*flist, *alist, *printid;

struct bufserv{

	int userId;
	int forumId;
	int msgId;
	int commentId;
	int choice;
	char *forumname;
	char msg[128];
}buf1;

bool flag = true;
int mid = 0;
int count1 = 0;
char *Data[100];
int count = 1;
int values[100];
DWORD WINAPI SocketHandler(void*);
void replyto_client(char *buf, int *csock);

void socket_server() {

	//The port you want the server to listen on
	int host_port = 1101;

	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)) {
		fprintf(stderr, "No sock dll %d\n", WSAGetLastError());
		goto FINISH;
	}

	//Initialize sockets and set options
	int hsock;
	int * p_int;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1){
		printf("Error initializing socket %d\n", WSAGetLastError());
		goto FINISH;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) ||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1)){
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	//Bind and listen
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	/* if you get error in bind
	make sure nothing else is listening on that port */
	if (bind(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
		fprintf(stderr, "Error binding to socket %d\n", WSAGetLastError());
		goto FINISH;
	}
	if (listen(hsock, 10) == -1){
		fprintf(stderr, "Error listening %d\n", WSAGetLastError());
		goto FINISH;
	}

	//Now lets do the actual server stuff

	int* csock;
	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);

	while (true){
		printf("waiting for a connection\n");
		csock = (int*)malloc(sizeof(int));

		if ((*csock = accept(hsock, (SOCKADDR*)&sadr, &addr_size)) != INVALID_SOCKET){
			printf("Received connection from %s", inet_ntoa(sadr.sin_addr));
			CreateThread(0, 0, &SocketHandler, (void*)csock, 0, 0);
		}
		else{
			fprintf(stderr, "Error accepting %d\n", WSAGetLastError());
		}
	}

FINISH:
	;
}

int load_buff(char *buff, int *csock){
	buff = ""; int recv_byte_cnt;
	if ((recv_byte_cnt = recv(*csock, buff, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return 0;
	}
}

int load_data(char *buff, int index, int *csock){
	if (index >= 1023){
		load_buff(buff, csock);
	}
	return buff[index] - '0';
}

int ** load_array(char *buff, int *m, int *n, int *x1, int *x2, int *y1, int *y2, int *csock){
	char ch; int index = 0;
	ch = buff[index++];
	*m = ch - '0';
	ch = buff[++index];
	*n = ch - '0';
	index++;
	int **arr;
	arr = (int **)malloc(sizeof(int *)**m);
	for (int i = 0; i < *m; i++)
		arr[i] = (int *)malloc(sizeof(int)**n);
	for (int i = 0; i < *m; i++){
		for (int j = 0; j < *n; j++){
			if (index >= 1023){
				load_buff(buff, csock);
			}
			arr[i][j] = buff[++index] - '0';
			index++;
		}
	}
	index++;
	*x1 = load_data(buff, index, csock);
	index += 2;
	*y1 = load_data(buff, index, csock);
	index += 2;
	*x2 = load_data(buff, index, csock);
	index += 2;
	*y2 = load_data(buff, index, csock);
	return arr;
}
int path_exists(int **arr, int m, int n, int x1, int y1, int x2, int y2, char path[]){
	if (x1 < 0 || y1 < 0 || x1 >= m || y1 >= n || arr[x1][y1] == 0 || arr[x1][y1] == 0)
		return 0;
	if (x1 == x2 && y1 == y2)
	{
		char buf[10];
		itoa(x1, buf, 10);
		strcat(path, strcat(buf, ","));
		itoa(y1, buf, 10);
		strcat(path, strcat(buf, "->"));
		//strcat(path,strcat(itoa(x1,buf,1), itoa(y1,buf,1)));
		//printf("%d,%d\t", x1, y1);
		return 1;
	}
	int result = 0;
	arr[x1][y1] = 0;
	result = path_exists(arr, m, n, x1, y1 + 1, x2, y2, path);
	arr[x1][y1] = 1;
	if (result == 1){
		char buf[10];
		itoa(x1, buf, 10);
		strcat(path, strcat(buf, ","));
		itoa(y1, buf, 10);
		strcat(path, strcat(buf, "->"));
		//printf("%d,%d\t", x1, y1);
		return 1;
	}
	arr[x1][y1] = 0;
	result = path_exists(arr, m, n, x1 + 1, y1, x2, y2, path);
	arr[x1][y1] = 1;
	if (result == 1){
		char buf[10];
		itoa(x1, buf, 10);
		strcat(path, strcat(buf, ","));
		itoa(y1, buf, 10);
		strcat(path, strcat(buf, "->"));
		//printf("%d,%d\t", x1, y1);
		return 1;
	}
	arr[x1][y1] = 0;
	result = path_exists(arr, m, n, x1, y1 - 1, x2, y2, path);
	arr[x1][y1] = 1;
	if (result == 1){
		char buf[10];
		itoa(x1, buf, 10);
		strcat(path, strcat(buf, ","));
		itoa(y1, buf, 10);
		strcat(path, strcat(buf, "->"));
		//printf("%d,%d\t", x1, y1);
		return 1;
	}
	arr[x1][y1] = 0;
	result = path_exists(arr, m, n, x1 - 1, y1, x2, y2, path);
	arr[x1][y1] = 1;
	if (result == 1){
		char buf[10];
		itoa(x1, buf, 10);
		strcat(path, strcat(buf, ","));
		itoa(y1, buf, 10);
		strcat(path, strcat(buf, "->"));
		//printf("%d,%d\t", x1, y1);
		return 1;
	}
}
void get_path(char *recvbuf, char *replybuf, int *csock){
	int m, n, x1, y1, x2, y2;
	int **arr = load_array(recvbuf, &m, &n, &x1, &x2, &y1, &y2, csock);
	int r = path_exists(arr, m, n, 0, 0, 1, 3, replybuf);
	printf("%s\n", replybuf);
}

void process_input(char *recvbuf, int recv_buf_cnt, int* csock)
{

	char replybuf[1024] = "";
	//printf("%s",recvbuf);
	get_path(recvbuf, replybuf, csock);
	replyto_client(replybuf, csock);
	replybuf[0] = '\0';
}

void replyto_client(char *buf, int *csock) {
	int bytecount;

	if ((bytecount = send(*csock, buf, strlen(buf), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
	}
	printf("\nreplied to client: %s\n", buf);
}

DWORD WINAPI SocketHandler(void* lp){
	int *csock = (int*)lp;

	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;

	memset(recvbuf, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return 0;
	}

	//printf("Received bytes %d\nReceived string \"%s\"\n", recv_byte_cnt, recvbuf);
	process_input(recvbuf, recv_byte_cnt, csock);

	return 0;
}
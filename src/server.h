#ifndef SERVER_H
#define SERVER_H

#include "udp.h"


typedef struct udpServer
{
	udp_t u;
	struct sockaddr_in server;

} udpServer_t;

void udpServer_zero(udpServer_t * restrict server);

bool udpServer_open(udpServer_t * restrict server, uint16_t port);
int udpServer_read(udpServer_t * restrict server, void * restrict buffer, int bufSize);
bool udpServer_write(const udpServer_t * restrict server, const void * restrict buffer, int numBytes);
bool udpServer_close(udpServer_t * restrict server);


#endif

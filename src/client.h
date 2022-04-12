#ifndef CLIENT_H
#define CLIENT_H

#include "udp.h"


typedef struct udpClient
{
	udp_t u;

} udpClient_t;

void udpClient_zero(udpClient_t * restrict client);

bool udpClient_open(udpClient_t * restrict client, const char * restrict ipstr, uint16_t port);
int udpClient_read(udpClient_t * restrict client, void * restrict buffer, int bufSize);
bool udpClient_write(const udpClient_t * restrict client, const void * restrict buffer, int numBytes);
bool udpClient_close(udpClient_t * restrict client);



#endif

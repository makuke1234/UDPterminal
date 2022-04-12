#ifndef UDP_H
#define UDP_H

#include <stdint.h>
#include <stdbool.h>
#include <WinSock2.h>

typedef struct udp
{
	SOCKET s;
	struct sockaddr_in si_other;

} udp_t;

bool udp_init(void);

void udp_zero(udp_t * restrict udp);

#define UDP_READ_ERROR SOCKET_ERROR
int udp_read(udp_t * restrict udp, void * restrict buffer, int bufSize);
bool udp_write(const udp_t * restrict udp, const void * restrict buffer, int numBytes);
bool udp_close(udp_t * restrict udp);

bool udp_free(void);

#endif

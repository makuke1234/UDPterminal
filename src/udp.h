#ifndef UDP_H
#define UDP_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <WinSock2.h>

typedef struct udp
{
	SOCKET s;
	struct sockaddr_in si_other;

} udp_t;

typedef struct udpThread
{
	udp_t * u;
	HANDLE hThread;
	bool killSwitch, hasIncome;

	void * buffer;
	int bufSize;

	FILE * fout;

} udpThread_t;

bool udp_init(void);

void udp_zero(udp_t * restrict udp);

#define UDP_READ_ERROR SOCKET_ERROR
int udp_read(udp_t * restrict udp, void * restrict buffer, int bufSize);
bool udp_write(const udp_t * restrict udp, const void * restrict buffer, int numBytes);
bool udp_close(udp_t * restrict udp);

bool udp_free(void);

void udpThread_init(udp_t * restrict udp, udpThread_t * restrict uThread);

bool udpThread_read(udpThread_t * restrict uThread, void * restrict buffer, int bufSize, FILE * restrict fout);

bool udpThread_stopRead(udpThread_t * restrict uThread);

#endif

#include "udp.h"

#include <assert.h>


static WSADATA s_wsa = { 0 };

bool udp_init(void)
{
	return WSAStartup(MAKEWORD(2, 2), &s_wsa) == 0;
}

void udp_zero(udp_t * restrict udp)
{
	*udp = (udp_t){
		.s        = INVALID_SOCKET,
		.si_other = { 0 }
	};
}

int udp_read(udp_t * restrict udp, void * restrict buffer, int bufSize)
{
	int slen = sizeof(udp->si_other);
	return recvfrom(udp->s, buffer, bufSize, 0,(struct sockaddr *)&udp->si_other, &slen);
}
bool udp_write(const udp_t * restrict udp, const void * restrict buffer, int numBytes)
{
	return sendto(udp->s, buffer, numBytes, 0, (const struct sockaddr *)&udp->si_other, sizeof(udp->si_other)) != SOCKET_ERROR;
}
bool udp_close(udp_t * restrict udp)
{
	if (udp->s != INVALID_SOCKET)
	{
		bool ret = closesocket(udp->s) == 0;
		udp->s = INVALID_SOCKET;
		return ret;
	}

	return true;
}

bool udp_free(void)
{
	return WSACleanup() == 0;
}

void udpThread_init(udp_t * restrict udp, udpThread_t * restrict uThread)
{
	assert(udp     != NULL);
	assert(uThread != NULL);

	*uThread = (udpThread_t){
		.u          = udp,
		.hThread    = INVALID_HANDLE_VALUE,
		.killSwitch = false,
		.hasIncome  = false
	};
}

#define UDP_THREAD_STACK_SIZE 20
#define UDP_THREAD_TIMEOUT_US 10

static inline DWORD WINAPI s_udpThread_read_thread(LPVOID lpParams)
{
	udpThread_t * uThread = lpParams;
	assert(uThread != NULL);

	fd_set currentSockets, readySockets;
	FD_ZERO(&currentSockets);
	FD_SET(uThread->u->s, &currentSockets);

	while (!uThread->killSwitch)
	{
		struct timeval timeout = {
			.tv_sec  = 0,
			.tv_usec = UDP_THREAD_TIMEOUT_US
		};

		readySockets = currentSockets;
	
		int retval = select(0, &readySockets, NULL, NULL, &timeout);
		if (retval < 0)
		{
			return 1;
		}

		if (FD_ISSET(uThread->u->s, &readySockets))
		{
			if (udp_read(uThread->u, uThread->buffer, uThread->bufSize) > 0)
			{
				uThread->hasIncome = true;
			}
		}
	}

	return 0;
}
bool udpThread_read(udpThread_t * restrict uThread, void * restrict buffer, int bufSize)
{
	uThread->killSwitch = false;
	uThread->hasIncome  = false;
	uThread->buffer     = buffer;
	uThread->bufSize    = bufSize;
	uThread->hThread    = CreateThread(
		NULL,
		UDP_THREAD_STACK_SIZE,
		&s_udpThread_read_thread,
		uThread,
		0,
		NULL
	);
	return uThread->hThread != INVALID_HANDLE_VALUE;
}
bool udpThread_hasIncome(const udpThread_t * restrict uThread)
{
	return uThread->hasIncome;
}
void udpThread_received(udpThread_t * restrict uThread)
{
	uThread->hasIncome = false;
}

bool udpThread_stopRead(udpThread_t * restrict uThread)
{
	if (uThread->hThread != INVALID_HANDLE_VALUE)
	{
		uThread->killSwitch = true;
		WaitForSingleObject(uThread->hThread, INFINITE);
		uThread->hThread = INVALID_HANDLE_VALUE;

		return true;
	}
	return false;
}


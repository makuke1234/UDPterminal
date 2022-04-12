#include "udp.h"


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

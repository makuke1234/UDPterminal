#include "client.h"


void udpClient_zero(udpClient_t * restrict client)
{
	udp_zero(&client->u);
}

bool udpClient_open(udpClient_t * restrict client, const char * restrict ipstr, uint16_t port)
{
	udpClient_zero(client);

	if ((client->u.s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		return false;
	}

	client->u.si_other.sin_family      = AF_INET;
	client->u.si_other.sin_port        = htons(port);
	client->u.si_other.sin_addr.s_addr = inet_addr(ipstr);

	return true;
}
int udpClient_read(udpClient_t * restrict client, void * restrict buffer, int bufSize)
{
	return udp_read(&client->u, buffer, bufSize);
}
bool udpClient_write(const udpClient_t * restrict client, const void * restrict buffer, int numBytes)
{
	return udp_write(&client->u, buffer, numBytes);
}
bool udpClient_close(udpClient_t * restrict client)
{
	return udp_close(&client->u);
}

#include "server.h"

void udpServer_zero(udpServer_t * restrict server)
{
	*server = (udpServer_t){ 0 };
	udp_zero(&server->u);
}

bool udpServer_open(udpServer_t * restrict server, uint16_t port)
{
	udpServer_zero(server);

	if ((server->u.s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		return false;
	}

	server->server.sin_family      = AF_INET;
	server->server.sin_addr.s_addr = INADDR_ANY;
	server->server.sin_port        = htons(port);

	if (bind(server->u.s, (const struct sockaddr *)&server->server, sizeof(server->server)) == SOCKET_ERROR)
	{
		udpServer_close(server);
		return false;
	}


	return true;
}
int udpServer_read(udpServer_t * restrict server, void * restrict buffer, int bufSize)
{
	return udp_read(&server->u, buffer, bufSize);
}
bool udpServer_write(const udpServer_t * restrict server, const void * restrict buffer, int numBytes)
{
	return udp_write(&server->u, buffer, numBytes);
}
bool udpServer_close(udpServer_t * restrict server)
{
	return udp_close(&server->u);
}


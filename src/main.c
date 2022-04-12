#include "server.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define LOCALHOST "127.0.0.1"
#define DEF_PORT 8888

#define BUFLEN 128

#define MAX_BUF MAX_PATH
#define NUM_BUFS 3

#define NUM_SERVERFORMATS 2
#define NUM_CLIENTFORMATS 3

bool scanarg(
	int argc, char * const * argv, bool * helpflag,
	bool * marked, bool ignoremarked,
	const char * format, char * buf, size_t buflen
)
{
	if (argc == 1)
	{
		return false;
	}
	for (int i = 1; i < argc; ++i)
	{
		const char * str = argv[i];
		str += *str == '-' || *str == '/';
		str += *str == '-';
		
		if (strcmp(str, "help") == 0)
		{
			*helpflag = true;
			return true;
		}
		else if (ignoremarked || !marked[i])
		{
			if (sscanf_s(argv[i], format, buf, buflen) > 0)
			{
				marked[i] = true;
				return true;
			}
		}
	}

	return false;
}
bool scanargs(
	int argc, char * const * argv,
	bool * helpflag,
	const char ** formats,
	char ** bufs,
	const size_t * buflens,
	size_t numArgs
)
{
	bool marked[argc];
	memset(marked, 0, sizeof(bool) * (size_t)argc);

	for (size_t i = 0; i < numArgs; ++i)
	{
		if (!scanarg(argc, argv, helpflag, marked, false, formats[i], bufs[i], buflens[i]))
		{
			return false;
		}
		if (helpflag)
		{
			return true;
		}
	}

	return true;
}

void printhelp(const char * app)
{
	printf(
		"Correct usage:\n"
		"%s -server -port=[PORT NUMBER]\n"
		"%s -client -ip=[IP ADDRESS] -port=[PORT NUMBER]\n"
		"Miscellanious commands:\n"
		"-help    -    Shows this page\n",
		app, app
	);
}

int main(int argc, char ** argv)
{
	if (!udp_init())
	{
		fprintf(stderr, "UDP initialization failed!\n");
		return 1;
	}

	bool helpflag = false;
	const char * serverFormats[NUM_SERVERFORMATS] = {
		"-se%s",
		"-port=%s"
	};
	const char * clientFormats[NUM_CLIENTFORMATS] = {
		"-cl%s",
		"-ip=%s",
		"-port=%s"
	};

	char * bufs[NUM_BUFS];
	size_t buflens[NUM_BUFS];
	for (size_t i = 0; i < NUM_BUFS; ++i)
	{
		if ((bufs[i] = calloc(MAX_BUF, 1)) == NULL)
		{
			fprintf(stderr, "Error allocating memory!\n");
			return 1;
		}
		buflens[i] = MAX_BUF;
	}

	bool isServer = true;
	if (!scanargs(argc, argv, &helpflag, serverFormats, bufs, buflens, NUM_SERVERFORMATS))
	{
		if (!helpflag)
		{
			isServer = false;
			if (!scanargs(argc, argv, &helpflag, clientFormats, bufs, buflens, NUM_CLIENTFORMATS))
			{
				fprintf(stderr, "Invalid arguments given!\n");
				if (!helpflag)
				{
					return 1;
				}
			}
		}
	}
	for (size_t i = 0; i < NUM_BUFS; ++i)
	{
		free(bufs[i]);
	}

	if (helpflag)
	{
		printhelp(argv[0]);
		return 1;
	}
	int buflen = BUFLEN;
	
	buflen = (buflen < BUFLEN) ? BUFLEN : buflen;
	char * buffer = malloc((size_t)buflen);
	if (buffer == NULL)
	{
		fprintf(stderr, "Error allocating memory for the buffer!\n");
		return 1;
	}

	if (isServer)
	{
		uint16_t port = (uint16_t)atoi(bufs[1]);
		printf("Configuration: port: %hu\n", port);
	
		udpServer_t server;
		if (!udpServer_open(&server, port))
		{
			fprintf(stderr, "Error initializing server!\n");
			return 1;
		}

		while (1)
		{
			printf("Waiting for data...\n");
			udpServer_read(&server, buffer, buflen);
			printf("Received data: \"%s\"\n", buffer);
		}

		udpServer_close(&server);
	}
	else
	{
		uint16_t port = (uint16_t)atoi(bufs[1]);
		const char * ip = bufs[2];
		printf("Configuration: IP: %s, port: %hu\n", ip, port);


		udpClient_t client;
		if (!udpClient_open(&client, ip, port))
		{
			fprintf(stderr, "Error initializing client!\n");
			return 1;
		}

		while (1)
		{
			printf("Send data: ");
			fgets(buffer, buflen, stdin);
			// Remove trailing newline
			int len = (int)strlen(buffer);
			if (buffer[len - 1] == '\n')
			{
				--len;
				buffer[len] = '\0';
			}

			if (strcmp(buffer, "exit") == 0)
			{
				printf("Exiting...\n");
				break;
			}

			printf("Sending data...\n");
			udpClient_write(&client, buffer, len + 1);
			printf("Data sent.\n");
		}

		udpClient_close(&client);
	}


	free(buffer);

	udp_free();

	return 0;
}

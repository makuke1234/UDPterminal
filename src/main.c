#include "server.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define LOCALHOST "127.0.0.1"
#define DEF_PORT 8888

#define BUFLEN 128

#define MAX_BUF MAX_PATH
#define NUM_BUFS 3

#define NUM_SERVERFORMATS 2
#define NUM_CLIENTFORMATS 3

static inline int scanFunc(
	const char * restrict input,
	const char * restrict format,
	char * restrict outbuf
)
{
	assert(input  != NULL);
	assert(format != NULL);
	assert(outbuf != NULL);
	
	// Find the slash
	const char * formtemp = format;
	
	for (; *formtemp != '\0'; ++formtemp)
	{
		if (*formtemp == '/')
		{
			break;
		}
	}
	
	const char * fullFMatch = format;
	size_t required = 0;
	if (*formtemp == '/')
	{
		required = (size_t)atoll(format);
		fullFMatch = formtemp + 1;
	}

	formtemp = (*formtemp != '\0') ? formtemp + 1 : formtemp;
	for (; *formtemp != '\0'; ++formtemp)
	{
		if (*formtemp == '=')
		{
			break;
		}
	}

	size_t fullFMatchLen = (size_t)(formtemp - fullFMatch);
	required = (required == 0) ? fullFMatchLen : required;

	input += (input[0] == '-') + (input[1] == '-');
	input += (input[0] == '/');

	// First match the full
	bool found = false;
	for (size_t i = 0; i < fullFMatchLen; ++i)
	{
		if (input[i] == '=')
		{
			if (i < required || *formtemp != '=')
			{
				return -1;
			}
			input += i + 1;
			found = true;
			break;
		}
		else if (input[i] == '\0')
		{
			if (i < required)
			{
				return -1;
			}
			break;
		}
		else if (fullFMatch[i] != input[i])
		{
			return -1;
		}
	}
	if (!found)
	{
		input += fullFMatchLen + 1;
	}

	if (*formtemp == '=')
	{
		++formtemp;
		size_t maxChars = (size_t)atoll(formtemp);
		size_t inpLen = strlen(input);
		maxChars = (inpLen < maxChars) ? inpLen : maxChars;
		memcpy(outbuf, input, maxChars);
		outbuf[maxChars] = '\0';
	}

	return 1;
}

static inline bool scanarg(
	int argc, char * const * restrict argv, bool * restrict helpflag,
	bool * restrict marked, bool ignoremarked,
	const char * restrict format, char * restrict buf
)
{
	assert(argc >= 1);
	assert(argv != NULL);
	assert(helpflag != NULL);
	assert(marked != NULL);
	assert(format != NULL);
	assert(buf != NULL);

	if (argc == 1)
	{
		buf[0] = '\0';
		return false;
	}
	for (int i = 1; i < argc; ++i)
	{
		if (scanFunc(argv[i], "1/help", buf) >= 0 || scanFunc(argv[i], "?", buf) >= 0)
		{
			*helpflag = true;
			return true;
		}
		else if (ignoremarked || !marked[i])
		{
			int res = scanFunc(argv[i], format, buf);
			if (res > 0 || strcmp(argv[i], format) == 0)
			{
				marked[i] = true;
				return true;
			}
		}
	}

	buf[0] = '\0';
	for (int i = 1; i < argc; ++i)
	{
		if (!marked[i])
		{
			strcpy(buf, argv[i]);
			break;
		}
	}

	return false;
}
static inline bool scanargs(
	int argc, char * const * restrict argv, bool * restrict helpflag,
	char * const * restrict formats, char * restrict bufs, size_t bufSize,
	size_t numArgs
)
{
	assert(argc >= 1);
	assert(argv != NULL);
	assert(helpflag != NULL);
	assert(formats != NULL);
	assert(bufs != NULL);
	assert(bufSize >= 2);
	assert(numArgs >= 1);	

	bool * marked = calloc((size_t)argc, sizeof(bool));
	if (marked == NULL)
	{
		bufs[0] = '\0';
		return false;
	}

	for (size_t i = 0; i < numArgs; ++i)
	{
		if (!scanarg(
			argc, argv, helpflag,
			marked, false,
			formats[i], &bufs[bufSize * i]
		))
		{
			free(marked);
			strcpy(bufs, &bufs[bufSize * i]);
			return false;
		}
		if (*helpflag)
		{
			free(marked);
			return true;
		}
	}

	free(marked);
	return true;
}

void printhelp(const char * restrict app)
{
	assert(app != NULL);

	printf(
		"Correct usage:\n"
		"\n"
		"%s --server --port=[PORT NUMBER]\n"
		"%s --client --ip=[IP ADDRESS] --port=[PORT NUMBER]\n"
		"\n"
		"All switches:\n"
		"  --help    -  Shows this page\n"
		"  --?       -  Shows this page\n"
		"  --server  -  Specify server mode\n"
		"  --client  -  Specify client mode\n"
		"  --ip      -  Specify IPv4 address (format: 255.255.255.255)\n"
		"  --port    -  Specify port number (format: 65535)\n"
		"\n"
		"Methods for closing the program:\n"
		"Ctrl+Z\n"
		"Ctrl+C\n"
		"exit\n",
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
	char * const serverFormats[NUM_SERVERFORMATS] = {
		"1/server",
		"1/port=6"
	};
	char * const clientFormats[NUM_CLIENTFORMATS] = {
		"1/client",
		"1/ip=16",
		"1/port=6"
	};

	char bufs[NUM_BUFS][MAX_BUF];

	bool isServer = true;
	if (!scanargs(argc, argv, &helpflag, serverFormats, (char *)bufs, MAX_BUF, NUM_SERVERFORMATS))
	{
		if (!helpflag)
		{
			isServer = false;
			if (!scanargs(argc, argv, &helpflag, clientFormats, (char *)bufs, MAX_BUF, NUM_CLIENTFORMATS))
			{
				fprintf(stderr, "Un-recognized command-line option '%s'\n", bufs[0]);
				if (!helpflag)
				{
					return 1;
				}
			}
		}
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
		printf("Configuration port: %hu\n", port);
	
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
		const char * ip = bufs[1];
		uint16_t port = (uint16_t)atoi(bufs[2]);
		printf("Configuration IP: %s, port: %hu\n", ip, port);


		udpClient_t client;
		if (!udpClient_open(&client, ip, port))
		{
			fprintf(stderr, "Error initializing client!\n");
			return 1;
		}

		while (1)
		{
			printf("Send data: ");
			if (fgets(buffer, buflen, stdin) == NULL)
			{
				printf("Exiting...");
				break;
			}
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

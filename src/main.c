#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define NOMINMAX
#include <Windows.h>

#include "server.h"
#include "client.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <conio.h>

#define LOCALHOST "127.0.0.1"
#define DEF_PORT 8888

#define BUFLEN 1024

#define MAX_BUF MAX_PATH
#define NUM_BUFS 4

#define NUM_SERVERFORMATS 3
#define NUM_CLIENTFORMATS 4

#define MAX_RECORD 128

static inline void keyboardHandler(udp_t * restrict udp, char * restrict buffer, int buflen, FILE * restrict fout)
{
	udpThread_t udpThread;
	udpThread_init(udp, &udpThread);
	if (!udpThread_read(&udpThread, buffer, buflen, fout))
	{
		fprintf(stderr, "Async server receiver thread creation failed!\n");
		exit(1);
	}
	
	char sendBuf[BUFLEN];
	int len = 0, idx = 0;
	INPUT_RECORD records[MAX_RECORD];
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

	printf("Ready to send data!\n");

	while (1)
	{
		DWORD nEvents;
		if ((kbhit() || (GetAsyncKeyState(VK_CONTROL) & 0x8000)) && ReadConsoleInputW(hStdIn, records, MAX_RECORD, &nEvents) && (nEvents > 0))
		{
			// Parse all events to buffer and possibly send out messages
			bool breakFlag = false;
			for (DWORD i = 0; i < nEvents; ++i)
			{
				INPUT_RECORD * event = &records[i];
				if (event->EventType == KEY_EVENT)
				{
					KEY_EVENT_RECORD * kev = &event->Event.KeyEvent;
					if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && ((kev->wVirtualKeyCode == 'C') || (kev->wVirtualKeyCode == 'Z')))
					{
						breakFlag = true;
						break;
					}
					else if ((kev->bKeyDown && (kev->wVirtualKeyCode == VK_RETURN)) || (len >= (BUFLEN - 1)))
					{
						// Send data
						sendBuf[len] = '\0';
						putchar('\n');
						if (strncmp(sendBuf, "exit", 4) == 0)
						{
							breakFlag = true;
							break;
						}
						// Send message
						udp_write(udp, sendBuf, len + 1);

						len = 0;
						idx = 0;
					}
					else if (kev->bKeyDown && (kev->wRepeatCount == 1))
					{
						if (kev->uChar.AsciiChar >= ' ')
						{
							sendBuf[idx] = kev->uChar.AsciiChar;
							putchar(sendBuf[idx]);
							++idx;
							if (idx > len)
							{
								len = idx;
							}
						}
						else if ((kev->wVirtualKeyCode == VK_BACK) && (len > 0))
						{
							--len;
							--idx;
							putchar('\b');
							putchar(' ');
							putchar('\b');
						}
						else if ((kev->wVirtualKeyCode == VK_LEFT) && (idx > 0))
						{
							--idx;
							putchar('\b');
						}
						else if ((kev->wVirtualKeyCode == VK_RIGHT) && (idx < len))
						{
							putchar(sendBuf[idx]);
							++idx;
						}
					}
				}
			}
			if (breakFlag)
			{
				break;
			}
		}
		Sleep(1);
	}

	printf("Exiting...\n");
	udpThread_stopRead(&udpThread);
}

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
	char * const * restrict formats, const bool * restrict isOptional, char * restrict bufs, size_t bufSize,
	size_t numArgs
)
{
	assert(argc >= 1);
	assert(argv != NULL);
	assert(helpflag != NULL);
	assert(formats != NULL);
	assert(isOptional != NULL);
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
			if (!isOptional[i])
			{
				free(marked);
				strcpy(bufs, &bufs[bufSize * i]);
				return false;
			}
			else
			{
				bufs[bufSize * i] = '\0';
			}
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
		"%s --server --port=<port number> [--output=<file>]\n"
		"%s --client --ip=<IP address> --port=<port number> [--output=<file>]\n"
		"\n"
		"All switches:\n"
		"  --help           -  Shows this page\n"
		"  --?              -  Shows this page\n"
		"  --server         -  Specifies server mode\n"
		"  --client         -  Specifies client mode\n"
		"  --ip=<addr>      -  Specifies <addr> as the IPv4 address to connect to\n"
		"  --port=<port>    -  Specifies <port> as the port number in the range of 0 to 65535\n"
		"  --output=<file>  -  Specifies the binary output file, optional\n"
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
	SetConsoleCtrlHandler(NULL, TRUE);

	if (!udp_init())
	{
		fprintf(stderr, "UDP initialization failed!\n");
		return 1;
	}

	bool helpflag = false;
	char * const serverFormats[NUM_SERVERFORMATS] = {
		"1/server",
		"1/output=260",
		"1/port=6",
	};
	bool serverOptional[NUM_SERVERFORMATS] = {
		false,
		true,
		false
	};
	char * const clientFormats[NUM_CLIENTFORMATS] = {
		"1/client",
		"1/output=260",
		"1/port=6",
		"1/ip=16"
	};
	bool clientOptional[NUM_CLIENTFORMATS] = {
		false,
		true,
		false,
		false
	};

	char bufs[NUM_BUFS][MAX_BUF];

	bool isServer = true;
	if (!scanargs(argc, argv, &helpflag, serverFormats, serverOptional, (char *)bufs, MAX_BUF, NUM_SERVERFORMATS))
	{
		if (!helpflag)
		{
			isServer = false;
			if (!scanargs(argc, argv, &helpflag, clientFormats, clientOptional, (char *)bufs, MAX_BUF, NUM_CLIENTFORMATS))
			{
				if (argc > 1)
				{
					fprintf(stderr, "Un-recognized command-line option '%s'\n", bufs[0]);
					if (!*helpflag)
					{
						return false;
					}
				}
				else
				{
					*helpflag = true;
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

	// Opens output file in binary mode
	FILE * fout = stdout;
	if (bufs[1][0] != '\0')
	{
		fout = fopen(bufs[1], "wb");
		if (fout == NULL)
		{
			fprintf(stderr, "Error opening output file in binary write mode!\n");
			return 1;
		}
	}
	
	buflen = (buflen < BUFLEN) ? BUFLEN : buflen;
	char * buffer = malloc((size_t)buflen);
	if (buffer == NULL)
	{
		fprintf(stderr, "Error allocating memory for the buffer!\n");
		return 1;
	}

	uint16_t port = (uint16_t)atoi(bufs[2]);
	
	if (isServer)
	{
		printf("Configuration port: %hu\n", port);
	
		udpServer_t server;
		if (!udpServer_open(&server, port))
		{
			fprintf(stderr, "Error initializing server!\n");
			return 1;
		}

		keyboardHandler(&server.u, buffer, buflen, fout);

		udpServer_close(&server);
	}
	else
	{
		const char * ip = bufs[3];
		printf("Configuration IP: %s, port: %hu\n", ip, port);


		udpClient_t client;
		if (!udpClient_open(&client, ip, port))
		{
			fprintf(stderr, "Error initializing client!\n");
			return 1;
		}

		keyboardHandler(&client.u, buffer, buflen, fout);

		udpClient_close(&client);
	}

	fclose(fout);

	free(buffer);

	udp_free();


	return 0;
}

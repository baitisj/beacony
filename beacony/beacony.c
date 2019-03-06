#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curses.h>
#include <signal.h>
#include <errno.h>

#include <sys/socket.h>
#include <net/if.h>
#ifdef __GLIBC__
#include <net/ethernet.h>
#else
#include <linux/if_ether.h>
#endif
#include <netax25/ax25.h>
#include <netax25/axconfig.h>

#include <config.h>
#include "beacony.h"

int timestamp;
static int sigint;
static int sock;

void display_timestamp(void)
{
	time_t timenowx;
	struct tm *timenow;

	time(&timenowx);
	timenow = localtime(&timenowx);

	lprintf(T_TIMESTAMP, "%02d:%02d:%02d", timenow->tm_hour,
		timenow->tm_min, timenow->tm_sec);
}

static void handle_sigint(int signal) 
{
	sigint++;
	close(sock);	/* disturb blocking recvfrom  */
	sock = -1;
}

#define ASCII		0
#define HEX 		1
#define READABLE	2

#define BUFSIZE		1500

int main(int argc, char **argv)
{
	unsigned char buffer[BUFSIZE];
	int dumpstyle = ASCII;
	int size;
	int s;
	char *port = NULL, *dev = NULL;
	struct sockaddr sa;
	socklen_t asize = sizeof(sa);
	struct ifreq ifr;
	int proto = ETH_P_AX25;
	int exit_code = EXIT_SUCCESS;

	timestamp = 0;

	while ((s = getopt(argc, argv, "8achip:rtv")) != -1) {
		switch (s) {
		case 'a':
			proto = ETH_P_ALL;
			break;
		case 'c':
			color = 1;
			break;
		case 'p':
			port = optarg;
			break;
		case 't':
			timestamp = 1;
			break;
		case 'v':
			printf("listen: %s\n", VERSION);
			return 0;
		case ':':
			fprintf(stderr,
				"listen: option -p needs a port name\n");
			return 1;
		case '?':
			fprintf(stderr,
				"Usage: listen [-a] [-c] [-p port] [-t] [-v]\n");
			return 1;
		}
	}

	if (ax25_config_load_ports() == 0)
		fprintf(stderr, "listen: no AX.25 port data configured\n");

	if (port != NULL) {
		if ((dev = ax25_config_get_dev(port)) == NULL) {
			fprintf(stderr, "listen: invalid port name - %s\n",
				port);
			return 1;
		}
	}

	if ((sock = socket(PF_PACKET, SOCK_PACKET, htons(proto))) == -1) {
		perror("socket");
		return 1;
	}

	if (color) {
		color = initcolor();	/* Initialize color support */
		if (!color)
			printf("Could not initialize color support.\n");
	}

	setservent(1);

	while (!sigint) {
		asize = sizeof(sa);

		signal(SIGINT, handle_sigint);
		signal(SIGTERM, handle_sigint);
		if ((size =
		     recvfrom(sock, buffer, sizeof(buffer), 0, &sa,
			      &asize)) == -1) {
			/*
			 * Signals are cared for by the handler, and we
			 * don't want to abort on SIGWINCH
			 */
			if (errno == EINTR) {
				refresh();
				continue;
			} else if (!(errno == EBADF && sigint)) {
				perror("recv");
				exit_code = errno;
			}
			break;
		}
		signal(SIGINT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		if (sock == -1 || sigint)
			break;

		if (dev != NULL && strcmp(dev, sa.sa_data) != 0)
			continue;

		if (proto == ETH_P_ALL) {
			strcpy(ifr.ifr_name, sa.sa_data);
			signal(SIGINT, handle_sigint);
			signal(SIGTERM, handle_sigint);
			if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
				if (!(errno == EBADF && sigint)) {
					perror("SIOCGIFHWADDR");
					exit_code = errno;
					break;
				}
			}
			signal(SIGINT, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			if (sock == -1 || sigint)
				break;
			if (ifr.ifr_hwaddr.sa_family == AF_AX25) {
				ki_dump(buffer, size, dumpstyle);
			}
		} else {
			ki_dump(buffer, size, dumpstyle);
		}
		if (color)
			refresh();
	}
	if (color)
		endwin();

	return exit_code;
}

static void baitisj_dump(unsigned char *data, int length) {
	unsigned char buf[1024];
	unsigned char* src=data;
	unsigned char* dst=buf;
        memset(buf, 0, sizeof(buf));
	while (*src != '\0' && src < (data + length) && dst+2 < buf+sizeof(buf)) {
		*dst = *src;
		if (*src == '\n') {
			*dst = '\\';
			++dst;
			*dst = 'n';
		}
		++src;
		++dst;
	}
	lprintf(T_DATA, "%s\n", buf);
}

void data_dump(unsigned char *data, int length, int dumpstyle)
{
	baitisj_dump(data, length);
}


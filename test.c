#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>

#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "type.h"


char *http_string = "GET /iphone/samples/bipbop/gear1/prog_index.m3u8 HTTP/1.1\\r\\n Accept: text/html, application/xhtml+xml, */*\\r\\nReferer: http://alones.kr/1898\\r\\nAccept-Language: ko-KR\\r\\nUser-Agent: Mozilla/5.0 (compatible; MSIE 9.0;IPMS/5E0D000A-14F56EB10A0-00000009596C; Windows NT 6.1; Trident/5.0)\\r\\nAccept-Encoding: gzip, deflate\\r\\nHost: devimages.apple.com\\r\\nConnection: Keep-Alive\\r\\n\\r\\n";

											  
char *http_string2 = "host : www.dal.kr, path : /data/index.html\\r\\nGet /data/index.html HTTP/1.0\\r\\n\\r\\n";

static size_t Net_Recv(int fd, void *buf, size_t buf_len);
static size_t Net_Write( int fd, void *buf, size_t buf_len);

static inline void SetWBE( char *p, unsigned short i_dw )
{
	p[0] = ( i_dw >>  8 )&0xff;
	p[1] = ( i_dw       )&0xff;
}

int Net_Socket(int pf, int type, int proto, bool nonblock)
{
	int fd;

	fd = socket(pf, type, proto);

	if ( fd == -1 )
	{
		fprintf(stderr, "socket create fail\n");
		return -1;
	}

	fcntl (fd, F_SETFD, FD_CLOEXEC);

	if ( nonblock)
	{
		fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK);
	}

	return fd;
}

static int Net_SockHandshake(int fd, char *host, int port)
{
	char buffer[128+2*256];
	int hlen = strlen(host);
	int len;

	buffer[0] = 5; /* sock version */
	buffer[1] = 0x01;
	buffer[2] = 0x00;               /* Reserved */
	buffer[3] = 3;                  /* ATYP: for now domainname */

	buffer[4] = hlen;
	memcpy( &buffer[5], host, hlen );
	SetWBE( &buffer[5+hlen], port );

	len = 5 + hlen + 2;


	if( Net_Write( fd, buffer, len ) != len )
	{
		fprintf(stderr, "write fail\n");
		return -1;
	}

	/* Read the header */
	if( Net_Recv( fd, buffer, 5 ) != 5 )
	{
		return -1;
	}

	fprintf( stderr, "socks: v=%d rep=%d atyp=%d", buffer[0], buffer[1], buffer[3] );

	if( buffer[1] != 0x00 )
	{
		fprintf( stderr, "socks: CONNECT request fariled" );
		return -1;
	}

	/* Read the remaining bytes */
	if( buffer[3] == 0x01 )
	{
		len = 4-1 + 2;
	}
	else if( buffer[3] == 0x03 )
	{
		len = buffer[4] + 2;
	}
	else if( buffer[3] == 0x04 )
	{
		len = 16-1+2;
	}
	else
	{
		return -1;
	}


	if( Net_Recv( fd, buffer, len ) != len )
	{
		return -1;
	}
}


int Net_Connect(char *host, char *port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	int evfd;
	int val;
	int reuseaddr = 1;
	socklen_t  sock_len;
	int res;
	int i_port;


	evfd = eventfd (0, EFD_CLOEXEC);

	struct pollfd ufd[2] = {
		{ .fd = sfd,   .events = POLLOUT },
		{ .fd = evfd,  .events = POLLIN },
	};

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = IPPROTO_TCP;      
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(host, port, &hints, &result);
	if (s != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(0);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) 
	{
		sfd = Net_Socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol, true);
		setsockopt (sfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof (int));

		if (sfd == -1)
		{
			fprintf(stderr, "Socket error\n");
			continue;
		}

		if (res = connect(sfd, rp->ai_addr, rp->ai_addrlen))
		{
			if( errno != EINPROGRESS && errno != EINTR )
			{
				fprintf( stderr, "2connection failed\n" );
				goto next_ai;
			}

			do
			{
				val = poll (ufd, sizeof (ufd) / sizeof (ufd[0]), 3000);
			} while ((val == -1) && (errno == EINTR));

			switch(val)
			{
				case -1:
					fprintf(stderr, " polling error\n");
				case 0:
					fprintf(stderr, " tiemout \n");
					goto next_ai;
				default:
					if (ufd[1].revents)
					{
						goto next_ai; 
					}
			}

			if (getsockopt (sfd, SOL_SOCKET, SO_ERROR, &val, &sock_len ))
			{
				errno = val;
				fprintf(stderr, "getsockopt connection failed: \n");
				goto next_ai;
			}
		
		}
		printf("connection succed %d\n", sfd);
	    break;
next_ai:
		close(sfd);
		continue;
	}	

	if ( result == NULL)
	{
		printf("result is null\n");
	}

	printf("res = %d\n", res);

	if ( sfd == -1 )
	{
		fprintf(stderr, "sfd Connection fail\n");
	}

	freeaddrinfo(result);           /* No longer needed */
	i_port = atoi(port);
    if ( Net_SockHandshake(sfd, host, i_port ))
	{
		fprintf(stderr, "Hand shake fail\n");
		return -1;
	}
	return sfd;
}


static size_t Net_Recv(int fd, void *buf, size_t buf_len)
{
	size_t recv_size;
	int evfd = eventfd (0, EFD_CLOEXEC);

	struct pollfd ufd[2] = {
		{ .fd = fd     , .events = POLLIN },
		{ .fd = evfd   , .events = POLLIN },
	};

	if (ufd[1].fd == -1)
	{
		return -1;
	}
	
	while (buf_len > 0)
	{
		ufd[0].revents = ufd[1].revents = 0;

		if (poll (ufd, sizeof (ufd) / sizeof (ufd[0]), -1) < 0)
		{
			if (errno != EINTR)
			{
				return -1;
			}
			continue;
		}

		if ( recv_size > 0 )
		{
			/*
			if (ufd[0].revents & ( POLLERR|POLLNVAL|POLLRDHUP) )
			*/
			if (ufd[0].revents & ( POLLERR|POLLNVAL) )
			{
				break;
			}
			if ( ufd[1].revents)
			{
				break;
			}
		}
		else
		{
			if (ufd[1].revents)
			{
				fprintf(stderr, "socket polling interrupt\n");
				errno = EINTR;
			}
			return -1;
		}
		size_t n;

		n = read(fd, buf, buf_len);


		if (n == -1)
		{
			switch (errno)
			{
				case EAGAIN:
				case EINTR:
					continue;
			}
			return -1;
		}

		if ( n == 0)
		{
			break;
		}

		printf("recive size = %d\n", n);
		recv_size  += n;
		buf = (char *)buf + n;
		buf_len -= n;
	}

	return recv_size;
}

static size_t Net_Write( int fd, void *buf, size_t buf_len)
{
	size_t write_size;
	size_t total;
	int evfd = eventfd (0, EFD_CLOEXEC);

	struct pollfd ufd[2] = {
		{ .fd = fd     , .events = POLLOUT },
		{ .fd = evfd   , .events = POLLIN },
	};

	while(buf_len > 0 )
	{

		ufd[0].revents = 0;
		ufd[1].revents = 0;

		if ( poll (ufd, sizeof(ufd) / sizeof(ufd[0]), -1) == -1)
		{
			if ( errno == EINTR)
			{
				continue;
			}
			fprintf(stderr, "plling error\n");
			return -1;
		}

		if ( total > 0 )
		{
			if (ufd[0].revents & (POLLHUP| POLLERR | POLLNVAL))
			{
				break;
			}
			if (ufd[1].revents)
			{
				break;
			}
		}
		else
		{
			if (ufd[1].revents)
			{
				errno = EINTR;
				return -1;
			}
		}


		write_size = write(fd, buf, buf_len);

		if ( write_size == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			fprintf(stderr, "write error\n");
			break;
		}

		buf = (const char *)buf + write_size;
		buf_len -= write_size;
		total += write_size;
	}

	if (( buf_len == 0 ) || (total > 0 ) )
	{
		return total;
	}
}

int Net_BindListen(const char *host, const char *port, int type, int protocol)
{
	int s, sfd;
	struct addrinfo hints, *rp, *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = type;
	hints.ai_protocol = protocol;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(host, port, &hints, &result);
	if (s != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(0);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) 
	{
		sfd = Net_Socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol, true);

		if ( sfd == -1 )
		{
			fprintf(stderr, "Socket error\n");
			continue;
		}

		if ( bind(sfd, rp->ai_addr, rp->ai_addrlen)) 
		{
			printf("close sfd\n");
			close(sfd);
		}

		if ( listen (sfd, 10))
		{
			printf("listen error\n");
			close(sfd);
			continue;
		}
	}

	freeaddrinfo(result);

	return sfd;
}

int accept_wrapper(int sfd, struct sockadr *addr, socklen_t *alen , bool nonblock)
{
	int fd;
	int flags;

	do
	{
		fd = accept(sfd, addr, alen);
		if ( fd != -1)
		{
			fcntl(fd, F_SETFD, FD_CLOEXEC);
			if ( nonblock)
			{
				flags = fcntl(fd, F_GETFL, 0);
				fcntl(fd, F_SETFL, flags | O_NONBLOCK);
			}
			return fd;
		}
	} while (errno == EINTR);

	return -1;
}

			
int Net_AcceptAlone(int sfd)
{
	int fd;
	int buflen = 2048;

	fd = accept_wrapper(sfd, NULL , NULL, true);
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (char *)&buflen, sizeof(int));

	return fd;
}

int Net_Accept(int server_fd)
{
	int n = 1024;
	unsigned int i;
	int evfd = eventfd (0, EFD_CLOEXEC);
	struct pollfd ufd[1024];

	for (i = 0; i <= n; i++)
	{
		ufd[i].fd = server_fd;
		ufd[i].events = POLLIN;
	}
	ufd[n].revents = 0;

	for (;;)
	{
		while (poll (ufd, n , 1000) == -1)
		{
			if (errno != EINTR)
			{
				fprintf(stderr, "poll error\n");
				return -1;
			}
		}

		for (i = 0; i < n; i++)
		{
			if (ufd[i].revents == 0)
				continue;

			int sfd = ufd[i].fd;
			int fd = Net_AcceptAlone (server_fd);
			if (fd == -1)
				continue;
			return fd;
		}

		if (ufd[n].revents)
		{
			errno = EINTR;
			break;
		}
	}
	return -1;
}





int main(int argc, char *argv[])
{

	char *buffer;
	int fd;
	size_t size;
	/*
http://devimages.apple.com/iphone/samples/bipbop/gear1/prog_index.m3u8
*/
	buffer = (char *)malloc(1024);
//	fd = Net_Connect("devimages.apple.com", "80");

#if 1
	fd = Net_Connect("127.0.0.1", "60000");
	//fd = Net_Connect("www.ccnaver.com", "10000");

	printf("fd = %d\n", fd);
	if ( fd < 0 )
	{
		return -1;
	}
	
	/*
	size = write(fd, http_string , strlen((const char *)http_string));
	*/
	size = Net_Write(fd, http_string , strlen((const char *)http_string));
	printf("string = %s\n", http_string);
	printf("write size = %d\n", size);

	
	/*
	size = read(fd, buffer, 1024);
	*/
	
	
	size = Net_Recv(fd, buffer, 1024);
	
	printf("read size = %d\n", size);
	printf("buffer = %s\n", buffer);
	if ( buffer != NULL )
	{
		free(buffer);
	}
#endif

	return 0;
}


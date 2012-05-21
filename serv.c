#include <stdio.h>
#include <fcntl.h>

int net_socket(int pf, int type, int proto, bool nonblock)
{
	int fd;

	fd = socket(pf, type, proto);

	if (fd == -1)
	{
		return -1;
	}

	fcntl (fd, F_SETFD, FD_CLOEXEC);
	if (nonblock)
	{
		fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK);
	}

	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &(int)1, sizeof (int));
	return fd;
}





	

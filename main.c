#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define DATA_NUM 16
#define WAIT_USEC 100000


static int open_serial(char** strs)
{
	int fd = open(*strs, O_RDWR | O_NOCTTY);
	if(fd < 0)
	{
		return -1;
	}
	if(fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
	{
		close(fd);
		return -1;
	}

	return fd;
}


enum RTS_MODE
{
	DISABLED,
	SOFTWARE,
	HARDWARE,

	UNKNOWN
};

static enum RTS_MODE get_mode(char** strs)
{
	enum RTS_MODE mode = DISABLED;
	for(; *strs != NULL; ++strs)
	{
		if(!strcmp(*strs, "disabled")) mode = DISABLED;
		if(!strcmp(*strs, "software")) mode = SOFTWARE;
		if(!strcmp(*strs, "hardware")) mode = HARDWARE;
	}
	return mode;
}


static void termios_makeraw(int fd, bool hardware_rts)
{
	struct termios options = {0};
	tcgetattr(fd, &options);
	cfmakeraw(&options);

	if(hardware_rts)
		options.c_cflag |= CRTSCTS;
	else
		options.c_cflag &= ~CRTSCTS;

	tcsetattr(fd, TCSANOW, &options);
}


static void setRTS(int fd)
{
	const int flags = TIOCM_RTS;
	ioctl(fd, TIOCMBIS, &flags);
}

static void clearRTS(int fd)
{
	const int flags = TIOCM_RTS;
	ioctl(fd, TIOCMBIC, &flags);
}


int main(int argc, char** argv)
{
	int data[DATA_NUM];

	int fd = open_serial(argv + 1);
	if(fd < 0)
	{
		perror(NULL);
		return EXIT_FAILURE;
	}

	switch(get_mode(argv + 2))
	{
	case DISABLED:
		termios_makeraw(fd, false);
		for(;;)
		{
			usleep(WAIT_USEC);

			for(size_t i = 0; i < DATA_NUM; ++i)
				data[i] = rand();

			write(fd, &data, sizeof(data));
			tcdrain(fd);
		}
		break;
	case SOFTWARE:
		termios_makeraw(fd, false);
		for(;;)
		{
			usleep(WAIT_USEC);

			for(size_t i = 0; i < DATA_NUM; ++i)
				data[i] = rand();

			setRTS(fd);
			write(fd, &data, sizeof(data));
			tcdrain(fd);
			clearRTS(fd);
		}
		break;
	case HARDWARE:
		termios_makeraw(fd, true);
		for(;;)
		{
			usleep(WAIT_USEC);

			for(size_t i = 0; i < DATA_NUM; ++i)
				data[i] = rand();

			write(fd, &data, sizeof(data));
			tcdrain(fd);
		}
		break;
	default:
		break;
	}//end switch

	close(fd);
}

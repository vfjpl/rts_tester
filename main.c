#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define DATA_NUM 32
#define WAIT_USEC 100000


enum RTS_MODE
{
	DISABLED,
	SOFTWARE,
	HARDWARE,

	UNKNOWN
};

static enum RTS_MODE get_mode(const char* mode)
{
	if(strcmp(mode, "disabled") == 0) return DISABLED;
	if(strcmp(mode, "software") == 0) return SOFTWARE;
	if(strcmp(mode, "hardware") == 0) return HARDWARE;
	return UNKNOWN;
}

static speed_t get_speed(const char* speed)
{
	if(strcmp(speed, "B0") == 0) return B0;
	if(strcmp(speed, "B9600") == 0) return B9600;
	if(strcmp(speed, "B19200") == 0) return B19200;
	if(strcmp(speed, "B38400") == 0) return B38400;
	if(strcmp(speed, "B57600") == 0) return B57600;
	if(strcmp(speed, "B115200") == 0) return B115200;
	if(strcmp(speed, "B230400") == 0) return B230400;
	return 0;
}

static void set_termios(int fd, speed_t speed, bool hardware_rts)
{
	struct termios options = {0};
	tcgetattr(fd, &options);
	cfmakeraw(&options);
	cfsetspeed(&options, speed);

	if(hardware_rts)
		options.c_cflag |= CRTSCTS;
	else
		options.c_cflag &= ~CRTSCTS;

	tcsetattr(fd, TCSANOW, &options);
}

static void set_RTS(int fd)
{
	const int flags = TIOCM_RTS;
	ioctl(fd, TIOCMBIS, &flags);
}

static void clear_RTS(int fd)
{
	const int flags = TIOCM_RTS;
	ioctl(fd, TIOCMBIC, &flags);
}

int main(int argc, char** argv)
{
	int data[DATA_NUM];

	if(argc < 4)
	{
		puts(argv[0]);
		return EXIT_SUCCESS;
	}

	int fd = open(argv[1], O_RDWR | O_NOCTTY);
	if(fd < 0)
	{
		perror(NULL);
		return EXIT_FAILURE;
	}

	switch(get_mode(argv[3]))
	{
	case DISABLED:
		set_termios(fd, get_speed(argv[2]), false);
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
		set_termios(fd, get_speed(argv[2]), false);
		for(;;)
		{
			usleep(WAIT_USEC);

			for(size_t i = 0; i < DATA_NUM; ++i)
				data[i] = rand();

			set_RTS(fd);
			write(fd, &data, sizeof(data));
			tcdrain(fd);
			clear_RTS(fd);
		}
		break;
	case HARDWARE:
		set_termios(fd, get_speed(argv[2]), true);
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

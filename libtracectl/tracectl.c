#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sched.h>
#define UNIX_PATH_MAX 108

//#define SOCKETDIR "/var/run/ust/socks"
#define SOCKETDIR "/tmp/socks"
#define SOCKETDIRLEN sizeof(SOCKETDIR)
#define USTSIGNAL SIGIO

#define DBG(fmt, args...) fprintf(stderr, fmt "\n", ## args)
#define WARN(fmt, args...) fprintf(stderr, "usertrace: WARNING: " fmt "\n", ## args)
#define ERR(fmt, args...) fprintf(stderr, "usertrace: ERROR: " fmt "\n", ## args)
#define PERROR(call) perror("usertrace: ERROR: " call)

#define MAX_MSG_SIZE (100)
#define MSG_NOTIF 1
#define MSG_REGISTER_NOTIF 2

struct tracecmd { /* no padding */
	uint32_t size;
	uint16_t command;
};

//struct listener_arg {
//	int pipe_fd;
//};

struct trctl_msg {
	/* size: the size of all the fields except size itself */
	uint32_t size;
	uint16_t type;
	/* Only the necessary part of the payload is transferred. It
         * may even be none of it.
         */
	char payload[94];
};

pid_t mypid;
char mysocketfile[UNIX_PATH_MAX] = "";
int pfd = -1;

void do_command(struct tracecmd *cmd)
{
}

void receive_commands()
{
}

int fd_notif = -1;
void notif_cb(void)
{
	int result;
	struct trctl_msg msg;

	/* FIXME: fd_notif should probably be protected by a spinlock */

	if(fd_notif == -1)
		return;

	msg.type = MSG_NOTIF;
	msg.size = sizeof(msg.type);

	/* FIXME: don't block here */
	result = write(fd_notif, &msg, msg.size+sizeof(msg.size));
	if(result == -1) {
		PERROR("write");
		return;
	}
}

int listener_main(void *p)
{
	int result;

	/* Allowing only 1 connection for now. */
	result = listen(pfd, 1);
	if(result == -1) {
		PERROR("listen");
		return 1;
	}

	for(;;) {

		uint32_t size;
		int fd;
		struct sockaddr_un addr;
		socklen_t addrlen = sizeof(addr);

		result = fd = accept(pfd, (struct sockaddr *)&addr, &addrlen);
		if(result == -1) {
			PERROR("accept");
			continue;
		}

		for(;;) {
			struct trctl_msg msg;
			unsigned char dontclose=0;

			result = read(fd, &msg.size, sizeof(msg.size));
			if(result == -1) {
				PERROR("read");
				continue;
			}

			if(size > MAX_MSG_SIZE) {
				ERR("trctl message too big");
				break;
			}

			result = read(fd, &msg.type, sizeof(msg.type));
			if(result == -1) {
				PERROR("read");
				continue;
			}

			switch(msg.type) {
				case MSG_REGISTER_NOTIF:
					/* TODO: put it in notif mode */
					goto next_conn;
			};
		}
		next_conn:;
	}
}

void create_listener(void)
{
	int result;
	static char listener_stack[16384];

	result = clone(listener_main, listener_stack+sizeof(listener_stack)-1, CLONE_FS | CLONE_FILES | CLONE_VM, NULL);
	if(result == -1) {
		perror("clone");
	}
}

/* The signal handler itself. */

void sighandler(int sig)
{
	DBG("sighandler");
	receive_commands();
}

/* Called by the app signal handler to chain it to us. */

void chain_signal(void)
{
	sighandler(USTSIGNAL);
}

static int init_socket(void)
{
	int result;
	int fd;
	char pidstr[6];
	int pidlen;

	struct sockaddr_un addr;
	
	result = fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if(result == -1) {
		PERROR("socket");
		return -1;
	}

	addr.sun_family = AF_UNIX;

	result = snprintf(addr.sun_path, UNIX_PATH_MAX, "%s/%d", SOCKETDIR, mypid);
	if(result >= UNIX_PATH_MAX) {
		ERR("string overflow allocating socket name");
		goto close_sock;
	}
	//DBG("opening socket at %s", addr.sun_path);

	result = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if(result == -1) {
		PERROR("bind");
		goto close_sock;
	}

	strcpy(mysocketfile, addr.sun_path);

	pfd = fd;
	return 0;

	close_sock:
	close(fd);

	return -1;
}

static void destroy_socket(void)
{
	int result;

	if(mysocketfile[0] == '\0')
		return;

	result = unlink(mysocketfile);
	if(result == -1) {
		PERROR("unlink");
	}
}

static int init_signal_handler(void)
{
	/* Attempt to handler SIGIO. If the main program wants to
	 * handle it, fine, it'll override us. They it'll have to
	 * use the chaining function.
	 */

	int result;
	struct sigaction act;

	result = sigemptyset(&act.sa_mask);
	if(result == -1) {
		PERROR("sigemptyset");
		return -1;
	}

	act.sa_handler = sighandler;
	act.sa_flags = SA_RESTART;

	/* Only defer ourselves. Also, try to restart interrupted
	 * syscalls to disturb the traced program as little as possible.
	 */
	result = sigaction(SIGIO, &act, NULL);
	if(result == -1) {
		PERROR("sigaction");
		return -1;
	}

	return 0;
}

static void __attribute__((constructor)) init()
{
	int result;

	mypid = getpid();

	/* Must create socket before signal handler to prevent races
         * on pfd variable.
         */
	result = init_socket();
	if(result == -1) {
		ERR("init_socket error");
		return;
	}
	result = init_signal_handler();
	if(result == -1) {
		ERR("init_signal_handler error");
		return;
	}

	return;

	/* should decrementally destroy stuff if error */

}

/* This is only called if we terminate normally, not with an unhandled signal,
 * so we cannot rely on it. */

static void __attribute__((destructor)) fini()
{
	destroy_socket();
}

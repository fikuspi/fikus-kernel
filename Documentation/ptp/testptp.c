/*
 * PTP 1588 clock support - User space test program
 *
 * Copyright (C) 2010 OMICRON electronics GmbH
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <fikus/ptp_clock.h>

#define DEVICE "/dev/ptp0"

#ifndef ADJ_SETOFFSET
#define ADJ_SETOFFSET 0x0100
#endif

#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif

/* When glibc offers the syscall, this will go away. */
#include <sys/syscall.h>
static int clock_adjtime(clockid_t id, struct timex *tx)
{
	return syscall(__NR_clock_adjtime, id, tx);
}

static clockid_t get_clockid(int fd)
{
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)	((~(clockid_t) (fd) << 3) | CLOCKFD)

	return FD_TO_CLOCKID(fd);
}

static void handle_alarm(int s)
{
	printf("received signal %d\n", s);
}

static int install_handler(int signum, void (*handler)(int))
{
	struct sigaction action;
	sigset_t mask;

	/* Unblock the signal. */
	sigemptyset(&mask);
	sigaddset(&mask, signum);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);

	/* Install the signal handler. */
	action.sa_handler = handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	sigaction(signum, &action, NULL);

	return 0;
}

static long ppb_to_scaled_ppm(int ppb)
{
	/*
	 * The 'freq' field in the 'struct timex' is in parts per
	 * million, but with a 16 bit binary fractional field.
	 * Instead of calculating either one of
	 *
	 *    scaled_ppm = (ppb / 1000) << 16  [1]
	 *    scaled_ppm = (ppb << 16) / 1000  [2]
	 *
	 * we simply use double precision math, in order to avoid the
	 * truncation in [1] and the possible overflow in [2].
	 */
	return (long) (ppb * 65.536);
}

static void usage(char *progname)
{
	fprintf(stderr,
		"usage: %s [options]\n"
		" -a val     request a one-shot alarm after 'val' seconds\n"
		" -A val     request a periodic alarm every 'val' seconds\n"
		" -c         query the ptp clock's capabilities\n"
		" -d name    device to open\n"
		" -e val     read 'val' external time stamp events\n"
		" -f val     adjust the ptp clock frequency by 'val' ppb\n"
		" -g         get the ptp clock time\n"
		" -h         prints this message\n"
		" -p val     enable output with a period of 'val' nanoseconds\n"
		" -P val     enable or disable (val=1|0) the system clock PPS\n"
		" -s         set the ptp clock time from the system time\n"
		" -S         set the system time from the ptp clock time\n"
		" -t val     shift the ptp clock time by 'val' seconds\n",
		progname);
}

int main(int argc, char *argv[])
{
	struct ptp_clock_caps caps;
	struct ptp_extts_event event;
	struct ptp_extts_request extts_request;
	struct ptp_perout_request perout_request;
	struct timespec ts;
	struct timex tx;

	static timer_t timerid;
	struct itimerspec timeout;
	struct sigevent sigevent;

	char *progname;
	int c, cnt, fd;

	char *device = DEVICE;
	clockid_t clkid;
	int adjfreq = 0x7fffffff;
	int adjtime = 0;
	int capabilities = 0;
	int extts = 0;
	int gettime = 0;
	int oneshot = 0;
	int periodic = 0;
	int perout = -1;
	int pps = -1;
	int settime = 0;

	progname = strrchr(argv[0], '/');
	progname = progname ? 1+progname : argv[0];
	while (EOF != (c = getopt(argc, argv, "a:A:cd:e:f:ghp:P:sSt:v"))) {
		switch (c) {
		case 'a':
			oneshot = atoi(optarg);
			break;
		case 'A':
			periodic = atoi(optarg);
			break;
		case 'c':
			capabilities = 1;
			break;
		case 'd':
			device = optarg;
			break;
		case 'e':
			extts = atoi(optarg);
			break;
		case 'f':
			adjfreq = atoi(optarg);
			break;
		case 'g':
			gettime = 1;
			break;
		case 'p':
			perout = atoi(optarg);
			break;
		case 'P':
			pps = atoi(optarg);
			break;
		case 's':
			settime = 1;
			break;
		case 'S':
			settime = 2;
			break;
		case 't':
			adjtime = atoi(optarg);
			break;
		case 'h':
			usage(progname);
			return 0;
		case '?':
		default:
			usage(progname);
			return -1;
		}
	}

	fd = open(device, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "opening %s: %s\n", device, strerror(errno));
		return -1;
	}

	clkid = get_clockid(fd);
	if (CLOCK_INVALID == clkid) {
		fprintf(stderr, "failed to read clock id\n");
		return -1;
	}

	if (capabilities) {
		if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps)) {
			perror("PTP_CLOCK_GETCAPS");
		} else {
			printf("capabilities:\n"
			       "  %d maximum frequency adjustment (ppb)\n"
			       "  %d programmable alarms\n"
			       "  %d external time stamp channels\n"
			       "  %d programmable periodic signals\n"
			       "  %d pulse per second\n",
			       caps.max_adj,
			       caps.n_alarm,
			       caps.n_ext_ts,
			       caps.n_per_out,
			       caps.pps);
		}
	}

	if (0x7fffffff != adjfreq) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_FREQUENCY;
		tx.freq = ppb_to_scaled_ppm(adjfreq);
		if (clock_adjtime(clkid, &tx)) {
			perror("clock_adjtime");
		} else {
			puts("frequency adjustment okay");
		}
	}

	if (adjtime) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_SETOFFSET;
		tx.time.tv_sec = adjtime;
		tx.time.tv_usec = 0;
		if (clock_adjtime(clkid, &tx) < 0) {
			perror("clock_adjtime");
		} else {
			puts("time shift okay");
		}
	}

	if (gettime) {
		if (clock_gettime(clkid, &ts)) {
			perror("clock_gettime");
		} else {
			printf("clock time: %ld.%09ld or %s",
			       ts.tv_sec, ts.tv_nsec, ctime(&ts.tv_sec));
		}
	}

	if (settime == 1) {
		clock_gettime(CLOCK_REALTIME, &ts);
		if (clock_settime(clkid, &ts)) {
			perror("clock_settime");
		} else {
			puts("set time okay");
		}
	}

	if (settime == 2) {
		clock_gettime(clkid, &ts);
		if (clock_settime(CLOCK_REALTIME, &ts)) {
			perror("clock_settime");
		} else {
			puts("set time okay");
		}
	}

	if (extts) {
		memset(&extts_request, 0, sizeof(extts_request));
		extts_request.index = 0;
		extts_request.flags = PTP_ENABLE_FEATURE;
		if (ioctl(fd, PTP_EXTTS_REQUEST, &extts_request)) {
			perror("PTP_EXTTS_REQUEST");
			extts = 0;
		} else {
			puts("external time stamp request okay");
		}
		for (; extts; extts--) {
			cnt = read(fd, &event, sizeof(event));
			if (cnt != sizeof(event)) {
				perror("read");
				break;
			}
			printf("event index %u at %lld.%09u\n", event.index,
			       event.t.sec, event.t.nsec);
			fflush(stdout);
		}
		/* Disable the feature again. */
		extts_request.flags = 0;
		if (ioctl(fd, PTP_EXTTS_REQUEST, &extts_request)) {
			perror("PTP_EXTTS_REQUEST");
		}
	}

	if (oneshot) {
		install_handler(SIGALRM, handle_alarm);
		/* Create a timer. */
		sigevent.sigev_notify = SIGEV_SIGNAL;
		sigevent.sigev_signo = SIGALRM;
		if (timer_create(clkid, &sigevent, &timerid)) {
			perror("timer_create");
			return -1;
		}
		/* Start the timer. */
		memset(&timeout, 0, sizeof(timeout));
		timeout.it_value.tv_sec = oneshot;
		if (timer_settime(timerid, 0, &timeout, NULL)) {
			perror("timer_settime");
			return -1;
		}
		pause();
		timer_delete(timerid);
	}

	if (periodic) {
		install_handler(SIGALRM, handle_alarm);
		/* Create a timer. */
		sigevent.sigev_notify = SIGEV_SIGNAL;
		sigevent.sigev_signo = SIGALRM;
		if (timer_create(clkid, &sigevent, &timerid)) {
			perror("timer_create");
			return -1;
		}
		/* Start the timer. */
		memset(&timeout, 0, sizeof(timeout));
		timeout.it_interval.tv_sec = periodic;
		timeout.it_value.tv_sec = periodic;
		if (timer_settime(timerid, 0, &timeout, NULL)) {
			perror("timer_settime");
			return -1;
		}
		while (1) {
			pause();
		}
		timer_delete(timerid);
	}

	if (perout >= 0) {
		if (clock_gettime(clkid, &ts)) {
			perror("clock_gettime");
			return -1;
		}
		memset(&perout_request, 0, sizeof(perout_request));
		perout_request.index = 0;
		perout_request.start.sec = ts.tv_sec + 2;
		perout_request.start.nsec = 0;
		perout_request.period.sec = 0;
		perout_request.period.nsec = perout;
		if (ioctl(fd, PTP_PEROUT_REQUEST, &perout_request)) {
			perror("PTP_PEROUT_REQUEST");
		} else {
			puts("periodic output request okay");
		}
	}

	if (pps != -1) {
		int enable = pps ? 1 : 0;
		if (ioctl(fd, PTP_ENABLE_PPS, enable)) {
			perror("PTP_ENABLE_PPS");
		} else {
			puts("pps for system time request okay");
		}
	}

	close(fd);
	return 0;
}

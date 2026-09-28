// rtc_test.c - Kiem thu driver RTC qua giao dien chuan /dev/rtcN (ioctl)
// Build: make   (hoac: gcc -Wall -O2 -o rtc_test rtc_test.c)
// Chay : sudo ./rtc_test /dev/rtc1   (VM)    |    sudo ./rtc_test /dev/rtc0   (Pi)
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

static int n_pass, n_fail;

static void check(int ok, const char *what)
{
	printf("[%s] %s\n", ok ? "PASS" : "FAIL", what);
	if (ok)
		n_pass++;
	else
		n_fail++;
}

/* struct rtc_time -> giay tu 1970. RTC trong Linux luon giu gio UTC */
static time_t rtc_to_epoch(const struct rtc_time *r)
{
	struct tm t = {
		.tm_sec = r->tm_sec, .tm_min = r->tm_min, .tm_hour = r->tm_hour,
		.tm_mday = r->tm_mday, .tm_mon = r->tm_mon, .tm_year = r->tm_year,
	};
	return timegm(&t);
}

/* Thoi gian thuc tu luc boot (giay, co phan le): thuoc do doc lap de so voi RTC */
static double now_boot(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_BOOTTIME, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void print_rtc(const char *label, const struct rtc_time *r)
{
	printf("       %s %04d-%02d-%02d %02d:%02d:%02d UTC\n", label,
	       r->tm_year + 1900, r->tm_mon + 1, r->tm_mday,
	       r->tm_hour, r->tm_min, r->tm_sec);
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/dev/rtc0";
	struct rtc_time a, b, t;
	struct tm *g;
	time_t now;
	double t_a, real, diff;
	long d;
	int fd, rc;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		/* ENOENT: chua probe | EBUSY: tien trinh khac dang mo | EACCES: thieu sudo */
		perror(path);
		return 2;
	}

	/* TC1: doc gio -> RTC core goi read_time() cua driver */
	rc = ioctl(fd, RTC_RD_TIME, &a);
	t_a = now_boot();
	check(rc == 0, "TC1 RTC_RD_TIME doc duoc gio");
	if (rc == 0)
		print_rtc("RTC     =", &a);

	/*
	 * TC2: RTC phai tang DUNG BANG thoi gian thuc da troi qua (do bang
	 * CLOCK_BOOTTIME), khong so voi hang so 3: tren may ao, tien trinh co the
	 * bi "khung" va thuc day tre (ngu 4-5 s thay vi 3 s) -> se FAIL oan.
	 * RTC chi co do phan giai 1 s nen cho phep lech toi da ~1 s.
	 */
	sleep(3);
	rc = ioctl(fd, RTC_RD_TIME, &b);
	real = now_boot() - t_a;
	d = (long)(rtc_to_epoch(&b) - rtc_to_epoch(&a));
	diff = d > real ? d - real : real - d;
	printf("       sau sleep(3): RTC tang %ld s, thoi gian thuc %.2f s\n", d, real);
	check(rc == 0 && diff <= 1.1, "TC2 dong ho chay dung toc do");

	/* TC3: dat 2030-01-01 00:00:00 roi doc lai -> set_time() + read_time() */
	memset(&t, 0, sizeof(t));
	t.tm_year = 2030 - 1900;
	t.tm_mon = 0;
	t.tm_mday = 1;
	rc = ioctl(fd, RTC_SET_TIME, &t);
	check(rc == 0, "TC3a RTC_SET_TIME 2030-01-01 00:00:00");
	rc = ioctl(fd, RTC_RD_TIME, &b);
	if (rc == 0)
		print_rtc("doc lai =", &b);
	check(rc == 0 && b.tm_year == 130 && b.tm_mon == 0 && b.tm_mday == 1 &&
	      b.tm_hour == 0 && b.tm_min == 0 && b.tm_sec <= 1,
	      "TC3b doc lai dung gio vua dat");

	/* TC4: kiem thu bien - nam 2150 nam ngoai range 2000..2099 -> phai bi tu choi */
	t.tm_year = 2150 - 1900;
	errno = 0;
	rc = ioctl(fd, RTC_SET_TIME, &t);
	check(rc < 0 && errno == ERANGE, "TC4 tu choi nam 2150 (ERANGE)");

	/* TC5: khoi phuc RTC = gio he thong hien tai (UTC), giong "hwclock -w" */
	now = time(NULL);
	g = gmtime(&now);
	memset(&t, 0, sizeof(t));
	t.tm_sec = g->tm_sec;
	t.tm_min = g->tm_min;
	t.tm_hour = g->tm_hour;
	t.tm_mday = g->tm_mday;
	t.tm_mon = g->tm_mon;
	t.tm_year = g->tm_year;
	check(ioctl(fd, RTC_SET_TIME, &t) == 0, "TC5 khoi phuc RTC = gio he thong");

	close(fd);
	printf("\n==> %d PASS, %d FAIL\n", n_pass, n_fail);
	return n_fail ? 1 : 0;
}

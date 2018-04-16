#ifndef RTC_H_
#define RTC_H_

#include <stddef.h>
#include <driverlib.h>

struct rtc_time
{
	int sec;
	int min;
	int hour;
	int day_of_week;
	int date;
	int month;
	int year;
};

enum IntervalSetting { POLL_SECOND, POLL_MINUTE, POLL_HOUR };

extern volatile bool rtc_minute_passed;
extern volatile bool rtc_second_passed;

int rtc_timechanged(struct rtc_time *current);
void rtc_init();
void rtc_gettime(struct rtc_time *result);
void rtc_settime(struct rtc_time *time);
int rtc_getinterval();
void rtc_setinterval(int setting);

#endif // RTC_H_

#ifndef I386_RTC_H
#define I386_RTC_H

#define RTC_CTRL 0x70
#define RTC_DATA 0x71

#define RTC_CUR_SEC    0
#define RTC_ALARM_SEC  1
#define RTC_CUR_MIN    2
#define RTC_ALARM_MIN  3
#define RTC_CUR_HOUR   4
#define RTC_ALARM_HOUR 5
#define RTC_DAY_OF_WEEK 6
#define RTC_DAY_OF_MONTH 7
#define RTC_CUR_MONTH 8
#define RTC_CUR_YEAR  9
#define RTC_STATUS_A 10
#define RTC_STATUS_B 11
#define RTC_STATUS_C 12
#define RTC_STATUS_D 13
#define RTC_DIAG     14
#define RTC_END_STATUS 15
#define RTC_FLOPPY_DESC 16
/* 17: reserved */
#define RTC_HD_DESC 18
/* 19: reserved */
#define RTC_CONFIG 20
#define RTC_MAINMEM_LO 21
#define RTC_MAINMEM_HI 22
#define RTC_ADDMEM_LO 23
#define RTC_ADDMEM_HI 24
/* 25-45: reserved */
#define RTC_CHECK_LO 46
#define RTC_CHECK_HI 47
/* 48-49: reserved */
#define RTC_YEAR_BCD 50

enum rtc_day_of_week
{
	RTC_SUNDAY = 1,
	RTC_MONDAY,
	RTC_TUESDAY,
	RTC_WEDNESDAY,
	RTC_THURSDAY,
	RTC_FRIDAY,
	RTC_SATURDAY,
};

#endif /*I386_RTC_H*/

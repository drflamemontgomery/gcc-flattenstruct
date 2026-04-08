#include <stdio.h>

// Always nice to have these type definitions
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

struct time {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
};

struct date {
  uint16_t year;
  uint8_t month;
  uint8_t day;
};

struct timestamp {
  struct time time __attribute__((flatten_struct));
  struct date date __attribute__((flatten_struct));
};

int main(void) {
  // Initialize the full date using date and time identifiers
  struct timestamp full_date;
  full_date.time.hours = 9;
  full_date.time.minutes = 30;
  full_date.time.seconds = 0;

  full_date.date.year = 2026;
  full_date.date.month = 4;
  full_date.date.day = 9;

  // Access the date without the date and time identifiers
  printf("The saved date: %02u/%02u/%04u %uh %um %us\n", full_date.day,
         full_date.month, full_date.year, full_date.hours, full_date.minutes,
         full_date.seconds);

  return 0;
}

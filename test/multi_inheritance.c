#include <stdio.h>

struct a {
  int a;
  int b;
};

struct b {
  int b;
  int c;
};

struct c {
  [[gnu::flatten_struct(true)]]
  struct a super_a;
  [[gnu::flatten_struct(false)]]
  struct b super_b;
};

int main(void) {
  struct c c;
  c.a = 1;
  c.b = 2;
  c.c = 3;

  printf("c.super_a.a = %d\n", c.super_a.a);
  printf("c.super_a.b = %d\n", c.super_a.b);
  printf("c.super_b.b = %d\n", c.super_b.b);
  printf("c.super_b.c = %d\n", c.super_b.c);
  return 0;
}

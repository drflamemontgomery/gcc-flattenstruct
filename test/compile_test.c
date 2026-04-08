#include <stdio.h>

struct a {
  int b;
};

struct b {
  struct a super __attribute__((flatten_struct));
  int c;
};

struct c {
  [[gnu::flatten_struct]]
  struct a super;

  int c;
};

int main(void) {
  struct b b;
  b.b = 1;
  printf("b.super.b = %d\n", b.super.b);

  b.super.b = 2;
  printf("b.b = %d\n", b.b);

  struct c c;
  c.b = 3;
  printf("c.super.b = %d\n", c.super.b);

  c.super.b = 4;
  printf("c.b = %d\n", c.b);
}

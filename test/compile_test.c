#include <stdio.h>

struct a {
  int b;
};

struct b {
  struct a super __attribute__((flatten_struct));
  int c;
};

int main(void) {
  struct b b;
  b.b = *((int *)"Hello");
  printf("super.b = %d\n", b.super.b);
  //  struct b b;
  //  b.c = 1;
  b.super.b = 2;
  printf("b = %d\n", b.b);
  // b.b = 3;

  // printf("b = %d\nc = %d\n", b.b, b.c);
}

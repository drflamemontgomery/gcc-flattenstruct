# gcc-flattenstruct

A GCC Plugin for adding more control over nested structure scope. 
The main purpose of this is to have a more OOP feel to C without having to 
use C++ and all of the C++ boilerplate. 
With this plugin, you will be able to _extend_ structs easily.

# Install

```bash
$ make
$ sudo make install
```

# Compiling

Compiling using this plugin is just like any other GCC plugin

`$ gcc -fplugin=flattenstruct main.c -o main`

# Examples

```c
struct s1 {
  int field1;
};

struct s2 {
  struct s1 super __attribute__((flatten_struct));
  int field2;
};

struct s2 {
  [[gnu::flatten_struct]]
  struct s1 super; 
  int field2;
};

// struct s2 test;
// test.super.field1 = 1;
// test.field1 = 2;
// test.field2 = 3;
//
// assert(test.super.field1 == 2);
// assert(test.field1 == 2);
// assert(test.field2 == 3);
```

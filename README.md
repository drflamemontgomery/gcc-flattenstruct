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

`flatten_struct(no_overwrite_duplicate_name, only_anonymous)`

```c
struct s1 {
  int field1;
};

struct s2 {
  struct s1 super __attribute__((flatten_struct));
  int field2;
};

//struct s2 {
//  [[gnu::flatten_struct]]
//  struct s1 super; 
//  int field2;
//};

struct s2 test;
test.super.field1 = 1;
test.field1 = 2;
test.field2 = 3;

assert(test.super.field1 == 2);
assert(test.field1 == 2);
assert(test.field2 == 3);


// Without overwriting duplicate names
struct s3 {
  [[gnu::flatten_struct]]
  struct s1 super;

  [[gnu::flatten_struct(true)]]
  struct s1 super2;
};

struct s3 test;
test.super.field1 = 1;
test.super2.field1 = 2;

assert(test.field1 == test.super.field1);
assert(test.super.field1 != test.super2.field1);


// Only having anonymous
struct s4 {
    [[gnu::flatten_struct(false, true)]]
    struct s1 super;
};

struct s4 test;
test.field1 = 2;

assert(test.field1 == test.super.field1); //  ERROR: struct s3 has no field 'super'

// Conflicting Names
struct s5 {
    [[gnu::flatten_struct]]
    struct s1 super;
    [[gnu::flatten_struct]]
    struct s1 super2; // ERROR: Duplicate field 'field1' in 'struct s5'
};
```



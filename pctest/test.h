// Copyright (C) Lars Christensen

#ifndef _test_h_
#define _test_h_

#include <list>

/** Test function prototype */
typedef void (*fn_t)();

/** Global list of tests */
std::list<fn_t> tests;

/** Test registration class */
class test_registrator
{
public:
  test_registrator(fn_t fn) { tests.push_back(fn); }
};

/** Test declaration macro */
#define TEST(fn)                 \
  void fn();                     \
  test_registrator reg_##fn(fn); \
  void fn()

/** number of passed assertions */
static int passed = 0;

/** number of failed assertions */
static int failed = 0;

#include <iostream>

template <typename T, typename S>
void assert_equal_f(const T &expected, const S &given, const char *file,
                    int line)
{
  if (!(expected == given)) {
    std::cout << file << "(" << line << "): "
              << "ERROR: got " << (int)given << ", expected " << (int)expected
              << std::endl;
    ++failed;
  } else {
    ++passed;
  }
}

#define assert_equal(expected, given) \
  assert_equal_f((expected), (given), __FILE__, __LINE__)

#define assert(expr) \
  assert_equal_f(true, (expr), __FILE__, __LINE__)

#endif // _test_h_
#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Configuration for Unity test framework on embedded S32K118

// Use 32-bit integers
#define UNITY_INT_WIDTH 32

// Don't include standard library headers (embedded system)
// Comment out if you have newlib/nano support
// #define UNITY_EXCLUDE_STDLIB_MALLOC

#define UNITY_DIFFERENTIATE_FINAL_FAIL

#endif

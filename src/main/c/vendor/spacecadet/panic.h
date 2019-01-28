#pragma once

void (panic)(const char * restrict location, const char * restrict message) __attribute__((noreturn));
void (panicf)(const char * restrict location, const char * restrict format, ...) __attribute__((noreturn, format (printf, 2, 3)));

#define VAL(x) #x
#define STR(x) VAL(x)

#define panic(MESSAGE)   panic(__FILE__":"STR(__LINE__), MESSAGE)
#define panicf(FMT, ...) panicf(__FILE__":"STR(__LINE__), FMT, ##__VA_ARGS__)

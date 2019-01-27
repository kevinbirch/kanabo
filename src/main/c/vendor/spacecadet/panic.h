#pragma once

void _panic_at(const char * restrict location, const char * restrict message) __attribute__((noreturn));
void _panicf_at(const char * restrict location, const char * restrict format, ...) __attribute__((noreturn, format (printf, 2, 3)));

#define VAL(x) #x
#define STR(x) VAL(x)

#define panic(MESSAGE)   _panic_at(__FILE__":"STR(__LINE__), MESSAGE)
#define panicf(FMT, ...) _panicf_at(__FILE__":"STR(__LINE__), FMT, ##__VA_ARGS__)

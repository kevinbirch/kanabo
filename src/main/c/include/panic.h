#pragma once

void _panic_at(const char * restrict file, int line, const char * restrict message) __attribute__((noreturn));
void _panicf_at(const char * restrict file, int line, const char * restrict format, ...) __attribute__((noreturn, format (printf, 3, 4)));

#define panic(MESSAGE)   _panic_at(__FILE__, __LINE__, MESSAGE)
#define panicf(FMT, ...) _panicf_at(__FILE__, __LINE__, FMT, ##__VA_ARGS__)

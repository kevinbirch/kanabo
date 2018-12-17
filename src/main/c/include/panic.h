#pragma once

void  _panic_at(const char * restrict message, const char * restrict file, int line) __attribute__((noreturn));

#define panic(MESSAGE) _panic_at(MESSAGE, __FILE__, __LINE__)

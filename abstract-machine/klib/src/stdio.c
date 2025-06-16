#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static int write_char(char *buf,size_t n,size_t *pos,char c){
  if (*pos+1 < n){
    buf[*pos] = c;
  }
  (*pos)++;
  return 1;
}

static int write_str(char *buf, size_t n, size_t *pos, const char *s) {
  int cnt = 0;
  while (*s) {
    write_char(buf, n, pos, *s++);
    cnt++;
  }
  return cnt;
}

static int write_int(char *buf, size_t n, size_t *pos, int v) {
  char tmp[16];
  int t = 0, cnt = 0;
  bool neg = false;
  if (v < 0) { neg = true; v = -v; }
  // 先把数字倒着写到 tmp
  do {
    tmp[t++] = '0' + (v % 10);
    v /= 10;
  } while (v);
  if (neg) tmp[t++] = '-';
  // 反过来写入 buf
  while (t--) {
    write_char(buf, n, pos, tmp[t]);
    cnt++;
  }
  return cnt;
}

int printf(const char *fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  for (int i = 0; i < len; i++) {
    putch(buf[i]);
  }
  return len;
}


int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, (size_t)-1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsprintf(out, fmt, ap);
  va_end(ap);
  return r;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return r;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  size_t pos = 0;
  const char *p = fmt;
  while (*p) {
    if (*p != '%') {
      write_char(out, n, &pos, *p++);
    } else {
      p++; // 跳过 '%'
      switch (*p) {
        case 's': {
          const char *s = va_arg(ap, const char *);
          write_str(out, n, &pos, s);
          break;
        }
        case 'd': {
          int v = va_arg(ap, int);
          write_int(out, n, &pos, v);
          break;
        }
        case '%': {
          write_char(out, n, &pos, '%');
          break;
        }
        default: {
          // 不支持的格式，输出 '%' 和它
          write_char(out, n, &pos, '%');
          write_char(out, n, &pos, *p);
          break;
        }
      }
      p++;
    }
  }
  // 终结符
  if (n > 0) {
    size_t term = pos < n-1 ? pos : n-1;
    out[term] = '\0';
  }
  return pos;
}

#endif

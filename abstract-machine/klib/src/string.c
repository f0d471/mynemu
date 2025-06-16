#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  const char *p = s;
  while (*p) p++;
  return p-s;
}

char *strcpy(char *dst, const char *src) {
  char *p = dst;
  while ((*p++ = *src++) != '\0'){}
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *p = dst;
  size_t i;
  for(i=0;i<n&&src[i];i++){
    p[i] = src[i];
  }
  for(;i<n;i++){
    p[i] = '\0';  
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  char *p = dst + strlen(dst);
  while ((*p++ = *src++) != '\0'){}
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 != '\0' && *s1 == *s2){
    s1++;
    s2++;
  }
  return (uint8_t)*s1 - (uint8_t)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i = 0;
  for (;i<n;i++){
    if (s1[i] != s2[i] || s1 [i] == '\0'){
      return (uint8_t)s1[i] - (uint8_t)s2[i];
    }
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  unsigned char uc = (unsigned)c;
  for (size_t i = 0;i < n; i++){
    p[i] = uc;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;
  if (d<s){
    for (size_t i=0;i<n;i++){
      d[i] = s[i];
    }
  }
  else if (d>s){
    for (size_t i = n; i>0; i--){
      d[i-1] = s[i-1];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = out;
  const unsigned char *s = in;
  for (size_t i =0;i<n;i++){
    d[i] = s[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 =s1,*p2 =s2;
  for (size_t i = 0;i<n;i++){
    if (p1[i] != p2[i]){
      return p1[i] - p2[i];
    }
  }
  return 0;
}

#endif

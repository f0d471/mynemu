/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/


/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <isa.h>
#include "memory/paddr.h"


enum {
  TK_NOTYPE = 256,     
  TK_ADD = '+',     
  TK_SUB = '-',     
  TK_MUL = '*',     
  TK_DIV = '/',     
  TK_LP = '(',  
  TK_RP = ')',
  TK_NE = '!',

  NUM = 1001,
  REG = 1002,
  HEX = 1003,

  EQ = 1004,
  LEQ = 1005,
  REQ = 1006,
  NOTEQ = 1007,

  OR = 1008,
  AND = 1009,
  DEREF = 1010,
  TK_NEG = 1011
   
  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
    {" +", TK_NOTYPE},       
    {"\\+", TK_ADD},      
    {"\\-", TK_SUB},      
    {"\\*", TK_MUL},      
    {"\\/", TK_DIV},      
    {"\\(", TK_LP},   
    {"\\)", TK_RP},

    {"0[xX][0-9a-fA-F]+", HEX},
    {"[0-9]+",NUM},
    {"\\$\\$?[a-zA-Z0-9]+", REG},
                 
    {"\\=\\=", EQ},
    {"\\!\\=", NOTEQ},
    {"\\<\\=", LEQ},
    {"\\>\\=", REQ},        
    {"\\|\\|", OR}, 
    {"\\&\\&", AND},
    {"\\!", '!'}
  /*
   * Inset the '(' and ')' on the [0-9] bottom case Bug.
   */

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

         switch (rules[i].token_type) {
              case 256:
                  break;

              case '+': case '-': case '*': case '/': case '(': case ')': case '!': 
              case EQ: case LEQ: case REQ: case NOTEQ: case OR: case AND: 
                  tokens[nr_token].type = rules[i].token_type;
                  nr_token++;
                  break;

              case NUM: case HEX:
                  tokens[nr_token].type = rules[i].token_type;
                  if (substr_len >= 32) {
                      printf("too long at %d\n", position);
                      return false;
                  }
                  strncpy(tokens[nr_token].str, substr_start, substr_len);
                  tokens[nr_token].str[substr_len] = '\0';
                  nr_token++;
                  break;
                
              case REG:
                  tokens[nr_token].type = REG;
                  if (substr_len >= 32) {
                      printf("too long at  %d\n", position);
                      return false;
                  }
                  strncpy(tokens[nr_token].str, substr_start, substr_len);
                  tokens[nr_token].str[substr_len] = '\0';
                  nr_token++;
                  break;

              default:
                  printf("Unknown token type at %d\n", position);
                  return false;
        }
            break;
      }
    }
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

void preprocess_tokens() {
  for (int i = 0; i < nr_token; i++) {
      if (tokens[i].type == TK_MUL) {
        if (i == 0 || (tokens[i-1].type != NUM && tokens[i-1].type != HEX && tokens[i-1].type != REG && tokens[i-1].type != TK_RP)) {
          tokens[i].type = DEREF;
        }
      }
      if (tokens[i].type == TK_SUB) {
        if (i == 0 || (tokens[i-1].type != NUM && tokens[i-1].type != HEX && tokens[i-1].type != REG && tokens[i-1].type != TK_RP)){
          tokens[i].type = TK_NEG;
        }    
      }
    }
  }

static int get_precedence(int type){
  switch (type){
    case OR: return 1;
    case AND: return 2;
    case EQ: case NOTEQ: case LEQ: case REQ: return 3;
    case '-': case '+': return 4;
    case '*': case '/': return 5;
    case DEREF: return 6;
    case TK_NEG: return 7;
    default : return 0;
  }
}

static bool check_parentheses(int p,int q){
  if (tokens[p].type != TK_LP || tokens[q].type != TK_RP){
    return false;
  }
  int count = 0;
  for (int i = p;i <= q; i++){
    if (tokens[i].type == TK_LP) count++;
    if (tokens[i].type == TK_RP) count--;
    if (count == 0 && i < q) return false;
  }
  return (count == 0);
}

static int find_main_operator(int p,int q){
  int op = -1;
  int min = INT_MAX;
  int count = 0;

  for (int i =p;i <= q ;i++){
    if (tokens[i].type == TK_LP) count++;
    else if (tokens[i].type == TK_RP) count--;

    if (count < 0) return -1;

    if (count == 0 && get_precedence(tokens[i].type) > 0){
      int pre = get_precedence(tokens[i].type);
      if (tokens[i].type == TK_NEG || tokens[i].type == TK_NE || tokens[i].type == DEREF) {
        if (pre < min) {
          min = pre;
          op = i;
        }
      } 
      else {
        if (pre <= min) {
          min = pre;
          op = i;
        }
      }
    }
  }

  if ((tokens[p].type == TK_NE || tokens[p].type == TK_NEG || tokens[p].type == DEREF) && op == -1) {
    return p;
  }  

  return op;
}

static uint32_t eval (int p,int q,bool *success){
  if (!*success) return 0;
  else {
    if (p>q){
      *success = false;
      return 0;
    }
    else if (p==q){
  
      if (tokens[p].type == NUM){
        return strtoul(tokens[p].str,NULL,10);
      }
      else if (tokens[p].type == HEX){
        return strtoul(tokens[p].str,NULL,16);
      }
      else if (tokens[p].type == REG){
        bool success_reg;
        word_t val = isa_reg_str2val(tokens[p].str + 1, &success_reg);
        if (!success_reg){
          *success = false;
          return 0;
        }
        return val;
      }
      else {
        *success = false;
        return 0;
      }

    }
    else if (check_parentheses(p,q)){
      return eval(p+1,q-1,success);
    }
    else {
      int op = find_main_operator(p,q);
  
      if (op == -1){
        *success = false;
        return 0;
      }
      else {
        if (op == p && tokens[op].type == TK_NEG){
          uint32_t val = eval(p+1,q,success);
          uint32_t a = -val;
          if (!*success) return 0;
          return a;
        }
        else if (tokens[op].type == TK_NE){
          uint32_t val = eval(op+1,q,success);
          uint32_t a = !val;
          if (!*success) return 0;
          return a;
        }
        else if (tokens[op].type == DEREF) {
          if (tokens[op + 1].type == NUM || tokens[op + 1].type == HEX) {
            uint32_t addr = strtoul(tokens[op + 1].str, NULL,tokens[op + 1].type == NUM ? 10 : 16);
            return paddr_read(addr, 4);
          }
          else {
            *success = false;
            return 0;
          }
        } 
        else {
          uint32_t val1 = eval(p,op-1,success);
          if (!*success) return 0;
          uint32_t val2 = eval(op+1,q,success);
          if (!*success) return 0;
      
          switch (tokens[op].type){
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/': 
              if (val2 == 0){
                *success = false;
                return 0;
              }
              return val1 / val2;
            case EQ: return val1 == val2;          
            case NOTEQ: return val1 != val2;       
            case LEQ: return val1 <= val2;         
            case REQ: return val1 >= val2;         
            case AND: return val1 && val2;         
            case OR: return val1 || val2;          
            default:
              *success = false;
              return 0;
          } 
        } 
      } 
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  preprocess_tokens();
  *success = true;
  return eval(0,nr_token - 1,success);
}


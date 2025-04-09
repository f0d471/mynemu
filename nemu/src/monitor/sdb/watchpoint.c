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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  char expr[256];
  uint32_t last_val;
  struct watchpoint *next;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }
  head = NULL;
  free_ = wp_pool;
}

WP* new_wp(){
  if (free_ == NULL){
    assert(0);
  }
  WP* wp = free_;
  free_ = free_ ->next;
  wp ->next = head;
  head = wp;
  return wp;
}

void free_wp(WP *wp){
  if(head == wp){
    head = wp->next;
  }
  else {
    WP* prev = head;
    while (prev != NULL && prev->next != wp){
      prev = prev->next;
    }
    if (prev != NULL){
      prev->next = wp->next;
    }
  }
  wp->next = free_;
  free_ = wp;
}

void add_watchpoint(char *e){
  WP* wp = new_wp();
  strncpy(wp->expr, e, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';
  bool success;
  wp ->last_val = expr(wp->expr,&success);
  if (!success){
    printf("Invalid expression\n");
    free_wp(wp);
  }
  else {
    printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
  }
}

void check_watchpoints(){
  WP* wp = head;
  while (wp != NULL){
    bool success;
    uint32_t current_value = expr(wp->expr, &success);
    if (success && current_value != wp->last_val) {
      printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
      printf("Old value: %u\n", wp->last_val);
      printf("New value: %u\n", current_value);
      wp->last_val = current_value; 
      if (nemu_state.state!= NEMU_END){
      nemu_state.state = NEMU_STOP;  
      }
    }
    wp = wp->next;
  }
}

void info_watchpoints(){
  WP* wp = head;
  if(wp == NULL){
    printf("No watchpoints\n");
  }
  while (wp != NULL) {
    printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
    wp = wp->next;
  }
}

void delete_watchpoint(int no) {
  WP* wp = head;
  while (wp != NULL) {
    if (wp->NO == no) {
      free_wp(wp);
      printf("Watchpoint %d deleted\n", no);
      return;
    }
    wp = wp->next;
  }
  printf("No watchpoint number %d\n", no);
}
/* TODO: Implement the functionality of watchpoint */



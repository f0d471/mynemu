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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

static int depth = 0;
static const int MAX_DEPTH = 5;

static void gen_num(char *buf, int *pos) {
    int num = rand() % 100; 
    *pos += snprintf(buf + *pos, MAX_EXPR_LEN - *pos, "%d", num);
}

static void gen_op(char *buf, int *pos) {
    char ops[] = {'+', '-', '*', '/'};
    char op = ops[rand() % 4];
    *pos += snprintf(buf + *pos, MAX_EXPR_LEN - *pos, " %c ", op);
}

void gen_rand_expr(char *buf, int *pos) {
    if (depth >= MAX_DEPTH || *pos >= MAX_EXPR_LEN - 1) {
        gen_num(buf, pos);
        return;
    }
    depth++;
    int choice = rand() % 3;
    switch (choice) {
        case 0:
            gen_num(buf, pos);
            break;
        case 1:
            *pos += snprintf(buf + *pos, MAX_EXPR_LEN - *pos, "(");
            gen_rand_expr(buf, pos);
            *pos += snprintf(buf + *pos, MAX_EXPR_LEN - *pos, ")");
            break;
        default:
            gen_rand_expr(buf, pos);
            gen_op(buf, pos);
            gen_rand_expr(buf, pos);
            break;
    }
    depth--;
}
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_x(char *args){
    char* n = strtok(args," ");
    char* baseaddr = strtok(NULL," ");
    int len = 0;
    paddr_t addr = 0;
    sscanf(n, "%d", &len);
    sscanf(baseaddr,"%x", &addr);
    for(int i = 0 ; i < len ; i ++)
    {
        printf("%x\n",paddr_read(addr,4));
        addr = addr + 4;
    }
    return 0;
}

static int cmd_p(char *args) {
	uint32_t num ;
	bool suc;
	num = expr (args,&suc);
	if (suc)
		printf ("0x%x:\t%d\n",num,num);
  else printf("false\n");
	return 0;
}

static int cmd_help(char *args);

static int cmd_si(char *args){
  int step =0;
  if(args == NULL)
    step =1;
  else
    sscanf(args,"%d",&step);
  cpu_exec(step);
  return 0;
}

static int cmd_info(char *args){
  if(args == NULL)
      printf("No args.\n");
  else if(strcmp(args, "r") == 0)
      isa_reg_display();
  else if(strcmp(args, "w") == 0)
      info_watchpoints();
  return 0;
}

static int cmd_d (char *args){
  if(args == NULL)
      printf("No args.\n");
  else{
      delete_watchpoint(atoi(args));
  }
  return 0;
}

static int cmd_w(char* args){
  add_watchpoint(args);
  return 0;
}

static int cmd_t(char *args) {
  int count = 1; 
  if (args != NULL) {
      sscanf(args, "%d", &count); 
      if (count <= 0) {
          printf("Invalid count: %d, using default (1)\n", count);
          count = 1; 
      }
  }

  srand(time(NULL)); 
  for (int i = 0; i < count; i++) {
      char expr_buf[MAX_EXPR_LEN] = {0};
      int pos = 0;
      gen_rand_expr(expr_buf, &pos);
      printf("Expression %d: %s\n", i + 1, expr_buf);
      
      bool success;
      uint32_t result = expr(expr_buf, &success);
      if (success) {
          printf("Result: %u (0x%x)\n", result, result);
      } else {
          printf("Evaluation failed.\n");
      }
  }
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step into the program (step by instruction)", cmd_si }, 
  { "info","Print register status or Prints the monitoring point information",cmd_info},
  { "x","x 10 $esp",cmd_x},
  { "p","calculate the expr",cmd_p},
  { "w","add watchpoint",cmd_w},
  { "d","delete watchpoint",cmd_d},
  { "info","print watchpoint information",cmd_info},
  { "t", "Generate and evaluate a random expression",cmd_t}
  /* TODO: Add more commands */
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

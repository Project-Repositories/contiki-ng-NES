/*
** Relevant files for inspiration: 
** shell-comands.h/c
** serial-shell.h/c
** shell.h/c
*/ 

#include <string.h>


PT_THREAD(cmd_ring(struct pt *pt, shell_output_func output, char *args))
{
  PT_BEGIN(pt);
  
  char *next_args;
  SHELL_ARGS_INIT(args, next_args);

  /* Get argument (remote hostname and url (http://host/url) */
  SHELL_ARGS_NEXT(args, next_args);


  if(args == NULL || (strchr(args,'0') == NULL && strchr(args,'1') == NULL)) {
    SHELL_OUTPUT(output, "role is not specified\n");
    PT_EXIT(pt);
  } else {
    if (strchr(args,'0') != NULL) {
        SHELL_OUTPUT(output, "starting ring topology as root (%s)\n", args);
        // trigger 'ring-as-root' event
        // ...
    }
    else {
        SHELL_OUTPUT(output, "starting ring topology as joiner (%s)\n", args);
        // trigger 'ring-as-joiner' event
        // ...
    }
  }
  PT_END(pt);
};


const struct shell_command_t custom_shell_commands[] = {
  { "ring-start",                 cmd_ring,                 "'> ring-start' 0/1: starts ring topology protocol as either root+coordinator (0) or non-root (1)" }
};


static struct shell_command_set_t custom_shell_command_set = {
  .next = NULL,
  .commands = custom_shell_commands,
};

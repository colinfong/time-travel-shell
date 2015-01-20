// UCLA CS 111 Lab 1 command interface

#include <stdbool.h>

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;
typedef struct command_node *command_node_t;

/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the flag is set.  */
void execute_command (command_t);

/* Return the exit status of a command, which must have previously
   been executed.  Wait for the command, if it is not already finished.  */
int command_status (command_t);

//added functions---------------------------


/*Initialize command node next and command pointers to NULL
Uses a linked list of command_t's*/
void init_cmd_node(command_node_t cmd);

 /*Initialize the head and tail of the cmd stream to NULL
 Points to head and tail of linked list of command nodes*/
void init_cmdStream(command_stream_t cmd);

/*Combine commands with respect to an operator*/
command_t combine(command_t c1, command_t c2, int op);

/*UNUSED*/
/*command_t combine_redirect(token_node_t n1, token_node_t n2, int op);*/

/* Execute Command Types */


typedef struct list_node *list_node_t;
typedef struct graph_node *graph_node_t;
typedef struct dependency_graph *dependency_graph_t;

dependency_graph_t createGraph(command_stream_t cs);

void executeGraph(dependency_graph_t dep);
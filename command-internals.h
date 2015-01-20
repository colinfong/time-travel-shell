// UCLA CS 111 Lab 1 command internals

enum command_type
{
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
};

// Data associated with a command.
struct command
{
    enum command_type type;   //set as one of the above command types

    // Exit status, or -1 if not known (e.g., because it has not exited yet).
    int status;

    // I/O redirections, or null if none.
    char *input;
    char *output;

    union
    {
        // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
        struct command *command[2]; //what to put in command[2]?

        // for SIMPLE_COMMAND:
        char **word;

        // for SUBSHELL_COMMAND:
        struct command *subshell_command;
    } u;
};

struct command_node {
    command_node_t next;
    command_t cmdPtr;
};

struct command_stream { 
    command_node_t head;
    command_node_t tail;
    int length;
};

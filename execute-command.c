// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#define INC_BUFSIZE 5

void execute_switch(command_t c);

int
command_status (command_t c)
{
    return c->status;
}

void executingSimple(command_t c) {
    int pid;
    int outputRedir = 0;
    int inputRedir = 0;
    int status;

    pid = fork();
    if (pid == 0) {
        /*  Create appropriate file desciptors. */
        if (c->input != NULL) { /* there is an input redirect*/
            if (c->output != NULL) { /* There is an input and output redirect. */
                /* Try and open input and output files. Error if open fails. */
                if ((inputRedir = open(c->input, O_RDONLY)) < 0)
                    error(1, errno,"error with opening an input file");

                if ((outputRedir = open(c->output, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0)
                    error(1, errno,"error with opening an output file");

                if (dup2(inputRedir, 0) < 0)
                    error(1, errno,"error with dup2");

                if (dup2(outputRedir, 1) < 0)
                    error(1, errno,"error with dup2");
            }
            else { /* There's no ouput redirect, only input redirect. */
                if ((inputRedir = open(c->input, O_RDONLY)) < 0)
                    error(1, errno,"error with opening an input file");

                if (dup2(inputRedir, 0) < 0)
                    error(1, errno,"error with dup2");
            }
        }
        else { /* There is no input redirect. */
            if (c->output != NULL) { /* No input redirect but there is an output redirect. */
                if ((outputRedir = open(c->output, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0)
                    error(1, errno,"error with opening an ouput file");

                if (dup2(outputRedir, 1) < 0)
                    error(1, errno,"error with dup2");
            }
        }
        /*  If the shell directive 'exec' is the simple argument, then just execute
            it ignoring 'exec'. */
        if (!(strcmp(c->u.word[0], "exec"))) {
            execvp(c->u.word[1], c->u.word+1);
            _exit(-1);
        }

        execvp(c->u.word[0], c->u.word);
        _exit(-1);  // Will only reach this line if execvp failed.
    }

    /* Execute the parent process, waiting for the exit status of the child. */   //close file descriptors here?
    else if (pid > 0) {
        waitpid(pid, &status, 0);
        c->status = WEXITSTATUS(status);
        if (inputRedir != 0)
            close(inputRedir);
        if (outputRedir != 0)
            close(outputRedir);
        return; //think about why must parent return and not exit
    }

    /* Error forking process. */
    else if (pid < 0) {
        error(1, errno, "fork was unsuccessful");
    }
}

void executingSubshell(command_t c) {
    int pid;
    int outputRedir = 0;
    int inputRedir = 0;
    int status;

    pid = fork();
    if (pid == 0) {
        /* Create appropriate file desciptors. */
        if (c->input != NULL) { /* there is an input redirect*/
            if (c->output != NULL) { /* There is an input and output redirect. */
                /* Try and open input and output files. Error if open fails. */
                if ((inputRedir = open(c->input, O_RDONLY)) < 0)
                    error(1, errno,"error with opening an input file");

                if ((outputRedir = open(c->output, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0)
                    error(1, errno,"error with opening an output file");

                if (dup2(inputRedir, 0) < 0)
                    error(1, errno,"error with dup2");

                if (dup2(outputRedir, 1) < 0)
                    error(1, errno,"error with dup2");
            }
            else { /* There's no ouput redirect, only input redirect. */
                if ((inputRedir = open(c->input, O_RDONLY)) < 0)
                    error(1, errno,"error with opening an input file");

                if (dup2(inputRedir, 0) < 0)
                    error(1, errno,"error with dup2");
            }
        }
        else { /* There is no input redirect. */
            if (c->output != NULL) { /* No input redirect but there is an output redirect. */
                if ((outputRedir = open(c->output, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0)
                    error(1, errno,"error with opening an ouput file");

                if (dup2(outputRedir, 1) < 0)
                    error(1, errno,"error with dup2");
            }
        }
        execute_switch(c->u.subshell_command);
        _exit(c->u.subshell_command->status);
    }

    /* Execute the parent process, waiting for the exit status of the child. */   //close file descriptors here?
    else if (pid > 0) {
        waitpid(pid, &status, 0);
        c->status = WEXITSTATUS(status);
        if (inputRedir != 0)
            close(inputRedir);
        if (outputRedir != 0)
            close(outputRedir);
        return;
    }

    /* Error forking process. */
    else if (pid < 0) {
        error(1, errno, "fork was unsuccessful");
    }
}

void executingSequence(command_t c) {
    pid_t firstPid = fork();
    int eStatus;
    int otherStatus;
    if(firstPid < 0) {
        error(1, errno, "fork was unsuccessful");
    }
    else if (firstPid == 0) { //child command will execute the second command
        //printf("sequence fork successful\n");
        execute_switch(c->u.command[0]);
        _exit(c->u.command[0]->status);
    }
    else { //parent will execute second command, execvp will exit a process
        waitpid(firstPid, &otherStatus, 0);
        pid_t secondPid = fork();
        if(secondPid < 0) { //parent fork unsuccessful
          error(1, errno, "fork was unsuccessful");
        }
        else if (secondPid == 0) { //child of parent executes
            //printf("parent executing %c\n", c->u.command[1]->u.word[0][0]);
            execute_switch(c->u.command[1]);
            _exit(c->u.command[1]->status);
        }
        else { //parent tries finishing the processes
            /*returnedPid =*/ waitpid(secondPid, &eStatus, 0);
            c->status = WEXITSTATUS(eStatus);
            return;
        }
    }
}

void executingAnd(command_t c) {
    /*  Execute left command. */
    execute_switch(c->u.command[0]);

    /*  If left command successful, execute right command. */
    if (c->u.command[0]->status == 0) {
        execute_switch(c->u.command[1]);

        /*  If right command successful, return successful AND_COMMAND.
            Else, return unsuccessful status. */
        if (c->u.command[1]->status == 0) {
            c->status = 0;
            return;
        }
        else {
            c->status = c->u.command[1]->status;
            return;
        }
    }
    else {
        /*  Else, return unsuccessful status of AND_COMMAND. */
        c->status = c->u.command[0]->status;
        return;
    }
}

void executingOr(command_t c) {
    /*  Execute left command. */
    execute_switch(c->u.command[0]);

    /*  If left command unsuccessful, execute right command. */
    if (c->u.command[0]->status != 0) {
        execute_switch(c->u.command[1]);

        /*  If right command successful, return successful OR_COMMAND.
            Else, return unsuccessful status. */
        if (c->u.command[1]->status == 0) {
            c->status = 0;
            return;
        }
        else {
            c->status = c->u.command[1]->status;
            return;
        }
    }
    else {
        /*  Else, return successful status of OR_COMMAND. */
        c->status = c->u.command[0]->status;
        return;
    }
}

void executingPipe(command_t c)
{
    pid_t returnedPid;
    pid_t firstPid;
    pid_t secondPid;
    int buffer[2];
    int eStatus;

    if ( pipe(buffer) < 0 )
    {
    error (1, errno, "pipe was not created");
    }

    firstPid = fork();
    if (firstPid < 0)
    {
        error(1, errno, "fork was unsuccessful");
    }
    else if (firstPid == 0) //child executes command on the right of the pipe
    {
        close(buffer[1]); //close unused write end

        //redirect standard input to the read end of the pipe
        //so that input of the command (on the right of the pipe)
        //comes from the pipe
        if ( dup2(buffer[0], 0) < 0 )
        {
            error(1, errno, "error with dup2");
        }
        execute_switch(c->u.command[1]);
        _exit(c->u.command[1]->status);
    }
    else 
    {
        // Parent process
        secondPid = fork(); //fork another child process
                                //have that child process executes command on the left of the pipe
        if (secondPid < 0)
        {
            error(1, 0, "fork was unsuccessful");
        }
        else if (secondPid == 0)
        {
            close(buffer[0]); //close unused read end
            if(dup2(buffer[1], 1) < 0) //redirect standard output to write end of the pipe
            {
                error (1, errno, "error with dup2");
            }
            execute_switch(c->u.command[0]);
            _exit(c->u.command[0]->status);
        }
        else
        {
            // Finishing processes
            returnedPid = waitpid(-1, &eStatus, 0); //this is equivalent to wait(&eStatus);
                        //we now have 2 children. This waitpid will suspend the execution of
                        //the calling process until one of its children terminates
                        //(the other may not terminate yet)

            //Close pipe
            close(buffer[0]);
            close(buffer[1]);

            if (secondPid == returnedPid )
            {
                //wait for the remaining child process to terminate
                waitpid(firstPid, &eStatus, 0); 
                c->status = WEXITSTATUS(eStatus);
                return;
            }
      
            if (firstPid == returnedPid)
            {
                //wait for the remaining child process to terminate
                waitpid(secondPid, &eStatus, 0);
                c->status = WEXITSTATUS(eStatus);
                return;
            }
        }
    }
}

void execute_switch(command_t c)
{
    switch(c->type)
    {
        case SIMPLE_COMMAND:
            executingSimple(c);
            break;
        case SUBSHELL_COMMAND:
            executingSubshell(c);
            break;
        case AND_COMMAND:
            executingAnd(c);
            break;
        case OR_COMMAND:
            executingOr(c);
            break;
        case SEQUENCE_COMMAND:
            executingSequence(c);
            break;
        case PIPE_COMMAND:
            executingPipe(c);
            break;
        default:
            error(1, 0, "Not a valid command");
    }
}

void
execute_command (command_t c)
{
    execute_switch(c);
    return;
    /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    //error (1, 0, "command execution not yet implemented");
}

/******************************************************************************
-------------------------------------------------------------------------------
Graph Nodes
-------------------------------------------------------------------------------
Constructs a new graph none and list node for each command node
of command stream. The graph node is created with the current
command node. The graph node is then added to a new list node.
-------------------------------------------------------------------------------
List Nodes
-------------------------------------------------------------------------------
The list nodes are used in a linked list to allow each new
list node to compare its read and write lists to those of each previous
list node that has been created. This constructs graph node dependencies
within each list node. This continues until each command node is
stored into the linked list with all dependencies created.
The linked list will then be sorted into a dependency graph.
-------------------------------------------------------------------------------
Dependency Graph
-------------------------------------------------------------------------------
A list node will be sorted into the dependent queue if
it's command depends on other commands denoted by a nonnull
before field in its graph_node. Otherwise the list node goes
to the no_dependencies queue.
******************************************************************************/ 

/******************************************************************************
    graph_node implementation
******************************************************************************/

struct graph_node {
    command_t command;      // root of the command tree the graph node holds
    graph_node_t *before;   // knows node(s) it depends on
    int bufsize;
    int before_size;
    pid_t pid;              // initialized as -1 until set after started/finished execution
    int dont_wait;          // to remedy if many trees depend on one tree
};

void initGraphNode(graph_node_t gn, command_t c) {
    gn->command = c;
    gn->pid = -1;
    gn->bufsize = 10;
    gn->before_size = 0;
    gn->before = NULL;
    gn->dont_wait = 0;
    return;
}

void insertGnBefore(graph_node_t gn, graph_node_t add) {
    if (gn->before == NULL) {
        gn->before = (graph_node_t*) malloc(sizeof(graph_node_t)*gn->bufsize);

        if (gn->before == NULL) {
            fprintf(stderr, "Error allocating memory.");
            exit(-1);
        }
    }
    else if (gn->before_size == gn->bufsize) {
        gn->bufsize += INC_BUFSIZE;
        gn->before = (graph_node_t*) realloc(gn->before, sizeof(graph_node_t)*gn->bufsize);

        if (gn->before == NULL) {
            fprintf(stderr, "Error reallocating memory.");
            exit(-1);
        }
    }
    gn->before[gn->before_size++] = add;
    return;
}

/******************************************************************************
    list_node implementation
        -we're not creating a linked list to hold these. rather, we'll just
         manually insert them into a pseudo linked list and to traverse will
         just use a while loop using the 'next' data member until it is NULL.
******************************************************************************/

struct list_node {
    list_node_t next;
    graph_node_t gn;
    char **readlist;
    char **writelist;
    int r_bufsize;        // used to nicely reallocate readlist/writelist memory
    int w_bufsize;
    int readlist_size;
    int writelist_size;
};

void initListNode(list_node_t ln, graph_node_t gn) {
    ln->next = NULL;
    ln->gn = gn;
    ln->readlist_size = 0;
    ln->writelist_size = 0;
    ln->r_bufsize = 10;
    ln->w_bufsize = 10;
    ln->readlist = NULL;
    ln->writelist = NULL;
    return;
}

void addReadlist(list_node_t ln, char *s) {
    if (ln->readlist == NULL) {
        ln->readlist = (char**) malloc(sizeof(char*)*ln->r_bufsize);

        if (ln->readlist == NULL) {
            fprintf(stderr, "Error allocating memory.");
            exit(-1);
        }
    }
    else if (ln->readlist_size == ln->r_bufsize) {
        ln->r_bufsize += INC_BUFSIZE;
        ln->readlist = (char**) realloc(ln->readlist, sizeof(char*)*ln->r_bufsize);
        if (ln->readlist == NULL) {
            fprintf(stderr, "Error reallocating memory.");
            exit(-1);
        }
    }
    ln->readlist[ln->readlist_size++] = s;
    return;
}

void addWritelist(list_node_t ln, char *s) {
    if (ln->writelist == NULL) {
        ln->writelist = (char**) malloc(sizeof(char*)*ln->w_bufsize);

        if (ln->writelist == NULL) {
            fprintf(stderr, "Error allocating memory.");
            exit(-1);
        }
    }
    else if (ln->writelist_size == ln->w_bufsize) {
        ln->w_bufsize += INC_BUFSIZE;
        ln->writelist = (char**) realloc(ln->writelist, sizeof(char*)*ln->w_bufsize);
        if (ln->writelist == NULL) {
            fprintf(stderr, "Error reallocating memory.");
            exit(-1);
        }
    }
    ln->writelist[ln->writelist_size++] = s;
    return;
}

/******************************************************************************
    dependency_graph implementation
******************************************************************************/

struct dependency_graph {
    graph_node_t *no_dependencies;
    graph_node_t *dependencies;
    int dep_size;
    int no_dep_size;
    int dep_bufsize;
    int no_dep_bufsize;
}deo;

void initDependencyGraph(dependency_graph_t dg) {
    dg->no_dependencies = NULL;
    dg->dependencies = NULL;
    dg->dep_size = 0;
    dg->no_dep_size = 0;
    dg->dep_bufsize = 10;
    dg->no_dep_bufsize = 10;
    return;
}

void addDependency(dependency_graph_t dg, graph_node_t gn) {
    if (dg->dependencies == NULL) {
        dg->dependencies = (graph_node_t*) malloc(sizeof(graph_node_t)*dg->dep_bufsize);

        if (dg->dependencies == NULL) {
            fprintf(stderr, "Error allocating memory.");
            exit(-1);
        }
    }
    else if (dg->dep_size == dg->dep_bufsize) {
        dg->dep_bufsize += INC_BUFSIZE;
        dg->dependencies = (graph_node_t*) realloc(dg->dependencies, sizeof(graph_node_t)*dg->dep_bufsize);

        if (dg->dependencies == NULL) {
            fprintf(stderr, "Error reallocating memory.");
            exit(-1);
        }
    }
    dg->dependencies[dg->dep_size++] = gn;
    return;
}

void addNoDependency(dependency_graph_t dg, graph_node_t gn) {
    if (dg->no_dependencies == NULL) {
        dg->no_dependencies = (graph_node_t*) malloc(sizeof(graph_node_t)*dg->no_dep_bufsize);

        if (dg->no_dependencies == NULL) {
            fprintf(stderr, "Error allocating memory.");
            exit(-1);
        }
    }
    else if (dg->no_dep_size == dg->no_dep_bufsize) {
        dg->no_dep_bufsize += INC_BUFSIZE;
        dg->no_dependencies = (graph_node_t*) realloc(dg->no_dependencies, sizeof(graph_node_t)*dg->no_dep_bufsize);

        if (dg->no_dependencies == NULL) {
            fprintf(stderr, "Error reallocating memory.");
            exit(-1);
        }
    }
    dg->no_dependencies[dg->no_dep_size++] = gn;
    return;
}

void processCommand(command_t c, list_node_t ln) {
    /*  If SIMPLE_COMMAND, add all word[1]...word[n-1] as inputs to readlist.
        Add input and output appropriately if they exist. Ignore's options
        (anything beginning with '-'). */
    if (c->type == SIMPLE_COMMAND) {
        for (int k = 1; c->u.word[k] != NULL; k++) {
            if (c->u.word[k][0] != '-') {
                addReadlist(ln, c->u.word[k]);
            }
        }
        
        if (c->input != NULL) {
            addReadlist(ln, c->input);
        }

        if (c->output != NULL) {
            addWritelist(ln, c->output);
        }
    }

    /*  Else if SUBSHELL_COMMAND, add input and output appropriately if they
        exist. */
    else if (c->type == SUBSHELL_COMMAND) {
        if (c->input != NULL) {
            addReadlist(ln, c->input);
        }

        if (c->output != NULL) {
            addWritelist(ln, c->output);
        }
    }

    /*  Else process the left and right commands recursively. */
    else {
        processCommand(c->u.command[0], ln);
        processCommand(c->u.command[1], ln);
    }
    return;
}

bool compareLists(char **a, int a_size, char**b, int b_size) {
    /*  Traverse two arrays of c-strings looking for any matches. */
    for (int i = 0; i < a_size; i++) {
        for (int g = 0; g < b_size; g++) {
            if(!strcmp(a[i],b[g])) {
                return true;
            }
        }
    }
  return false;
}

dependency_graph_t createGraph(command_stream_t cs) {
    dependency_graph_t dg = (dependency_graph_t) malloc(sizeof(struct dependency_graph));
    initDependencyGraph(dg);

    /*  This big while loop processes each command_tree from the 
        command_stream. */
    command_t c;
    list_node_t old = NULL;
    while ((c = read_command_stream(cs))) {
        graph_node_t gn = (graph_node_t) malloc(sizeof(struct graph_node));
        list_node_t ln = (list_node_t) malloc(sizeof(struct list_node));
        initGraphNode(gn, c);
        initListNode(ln, gn);
        processCommand(c, ln);  //creates read/writelists

        /*  Insert the list_node into the head of a linked list to compare. */
        if (old != NULL) {
            ln->next = old;
        }

        /*  Compare current list_node to the rest in the list and check for
            any possible data hazards. If so, add the dependencies into the
            list_node's graph_node's 'list_node_t *before' field. */
        list_node_t temp = ln->next;
        while (temp != NULL) {
            if ((ln->readlist != NULL && temp->writelist != NULL &&
                compareLists(ln->readlist, ln->readlist_size, temp->writelist, temp->writelist_size)) ||    //RAW
                (ln->writelist != NULL && temp->readlist != NULL &&
                 compareLists(ln->writelist, ln->writelist_size, temp->readlist, temp->readlist_size)) ||   //WAR
                (ln->writelist != NULL && temp->writelist != NULL &&
                 compareLists(ln->writelist, ln->writelist_size, temp->writelist, temp->writelist_size))) {   //WAW
                insertGnBefore(ln->gn, temp->gn);
            }
            temp = temp->next;
        }

        /*  If the current list_node's graph_node's before == NULL, add it to
            the dependency_graph's no_dependencies. */
        if (ln->gn->before == NULL) {
            addNoDependency(dg, ln->gn);
        }
        else {
            addDependency(dg, ln->gn);
        }

        old = ln;
    }   //end while
    return dg;
}

void executeNoDependencies(dependency_graph_t dg) {
    //printf("There are %d independent trees to execute.\n", dg->no_dep_size);

    /*  Loop through, executing all the command trees that don't have any
        dependencies and set the tree's respective graph_node's pid to a
        valid number so that dependent trees can know that it started. */
    for (int k = 0; k < dg->no_dep_size; k++) {
        pid_t pid = fork();
        if (pid == 0) {
            execute_command(dg->no_dependencies[k]->command);
            _exit(0);
        }
        else if (pid > 0) {
            dg->no_dependencies[k]->pid = pid;
        }
        else {
            error(1, errno, "fork was unsuccessful");
        }
    }
    return;
}

void executeDependencies (dependency_graph_t dg) {
    //printf("There are %d dependent trees to execute.\n", dg->dep_size);

    for (int k = 0; k < dg->dep_size; k++) {
        graph_node_t temp = dg->dependencies[k];

        /*  Wait for all processes in the current graph_node's before to
            have at least started. */
        for (int i = 0; i < temp->before_size; i++) {
            while (temp->before[i]->pid == -1)
                continue;
        }

        /*  Wait for all processes in the current graph_node's before to
            finish execution. */
        int status;
        for (int i = 0; i < temp->before_size; i++) {
            if (temp->before[i]->dont_wait == 0) {
                waitpid(temp->before[i]->pid, &status, 0);
                temp->before[i]->dont_wait = 1;
            }
        }

        /*  Once dependent command trees have finished, run yourself. */
        pid_t pid = fork();
        if (pid == 0) {
            execute_command(temp->command);
            _exit(0);
        }
        else if (pid > 0) {
            temp->pid = pid;
        }
        else {
            error(1, errno, "fork was unsuccessful");
        }
    }
    return;
}

void executeGraph(dependency_graph_t dep) {
    executeNoDependencies(dep);
    executeDependencies(dep);
    return;
}

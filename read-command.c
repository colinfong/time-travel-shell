// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#define STACK_SIZE 1000

//used for checking if malloc worked
void memcheck(void *ptr) {
    if (ptr == NULL)
    {
        fprintf(stderr, "Error allocating memory.");
        exit(1);
    }
}


/******************************************************************************
Command Node Functions
******************************************************************************/
void init_cmd_node(command_node_t cmd){
    cmd->next = NULL;
    cmd->cmdPtr  = NULL;
}


/******************************************************************************
Command Stream Functions
******************************************************************************/

void init_cmdStream(command_stream_t cmd) {
    cmd->head = NULL;
    cmd->tail = NULL;
}

void addCmd (command_stream_t stream, command_node_t toAdd) { //add node to command_stream
    if (stream->head == NULL) {//empty list
        stream->head = toAdd;
        stream->tail = toAdd;
        stream->head->next = NULL;
    }
    else {
        toAdd->next = NULL;
        stream->tail->next = toAdd;
        stream->tail = toAdd;
        stream->tail->next = NULL;
    }
    return;
}

command_node_t getCmd (command_stream_t this) { //read linked list from the front
    if (this->head == NULL) {
        return NULL;
    }
    else {
        command_node_t temp = this->head;
        this->head = this->head->next;
        return temp;
    	//WHAT THE HECK HAPPENED HERE?! QUARANTINEEEE
        /*if (this->head->next != NULL) {
          this->head = this->head->next;
          return temp;
        }*/
    }
}

command_node_t peekCmdStrm (command_stream_t this) {
    if (this->head == NULL) {
        return NULL;
    }
    else {
        return this->head;
    }
}


/******************************************************************************
Stacks implementation
******************************************************************************/
typedef struct op_stack *op_stack_t;
typedef struct cmd_stack *cmd_stack_t;

struct op_stack {
    int o_stack[STACK_SIZE];
    int size;
};


struct cmd_stack { //holds command_ts
    command_t c_stack[STACK_SIZE];
    int size; //assume -1 first
};


/******************************************************************************
Operation Stack Functions
    -Used for storing operations such as &&, ||, |, and etc.
******************************************************************************/

void init_opStack(op_stack_t os) {
    os->size = 0;
}

//Push an operator onto the stack
void opPush(op_stack_t stack, int insert) {
    if(stack->size <= STACK_SIZE) {
        stack->o_stack[stack->size] = insert;
        stack->size++;
    }
}

int opPop(op_stack_t stack) {
    if(stack->size == 0 ) {
        return -1;
    } 
    else {
        stack->size--;
        char temp = stack->o_stack[stack->size];
        return temp;
    }
}

int opPeek(op_stack_t stack) { //returns character
    if(stack->size == 0) {
        return -1;
    } 
    else {
        return stack->o_stack[(stack->size)-1];
    }
}


/******************************************************************************
Command Stack Functions
    -Stores commands such as "cat a.txt" and etc.
******************************************************************************/

void init_cmdStack(cmd_stack_t cs) {
    cs->size = 0;
}

void cmdPush(cmd_stack_t stack, command_t insert) {
    if(stack->size <= STACK_SIZE) { 
        stack->c_stack[stack->size] = insert;
        stack->size++;
    }
}

command_t cmdPop(cmd_stack_t stack) {
    if(stack->size == 0) {
        return NULL;
    }
    else {
        stack->size--;
        command_t temp = stack->c_stack[stack->size];
        return temp;
    }
}

command_t cmdPeek(cmd_stack_t stack) {
    if(stack->size == 0)
        return NULL;
    else
        return stack->c_stack[(stack->size)-1];
}


/******************************************************************************
Token List implementation and functions
******************************************************************************/

typedef struct token_node *token_node_t;
typedef struct token_list *token_list_t;

enum tokenType {
    //special tokens: ; | && || ( ) < >
    SIMPLE,
    SEMICOLON,
    PIPE,
    AMPERSAND, 
    OR,   //combine pipes into ORs in grammar()
    AND,  //combine ampersand into ANDs in grammar()
    OPEN_PARENTHESIS,
    CLOSE_PARENTHESIS,
    LESS_THAN,
    GREATER_THAN,
    COMMENT,
    NEWLINE,
    END_COMMAND,  //if newline sequence that ends a command
};

struct token_node {
    enum tokenType type;
    char *string;
    int lineNumber;
    int length;
    token_node_t next;
    token_node_t prev;
};

struct token_list {
    token_node_t head;
    token_node_t tail;
    int length;
};

int precedence (int a) {  //pass in token's type to this function
    switch (a)
    {
        case SEMICOLON:
            return 0;
            break;
        case AND:
        case OR:
            return 1;
            break;
        case PIPE:
            return 2;
            break;
        default:
            return -1;
    }
}


void init_tokenList (token_list_t list) {
  list->head = NULL;
  list->tail = NULL;
  list->length = 0;
}

void init_toke_node(token_node_t t) {
    t->length = 0;
    t->next = NULL;
    t->prev = NULL;
}

int addToken(token_list_t list, token_node_t toAdd) {
    if(list->head == NULL) {
        toAdd->prev = NULL;
        toAdd->next = NULL;
        list->head = toAdd;
        list->tail = toAdd;
        list->length = 1;
    } 
    else {
        token_node_t prev_temp = list->tail;
        prev_temp->next = toAdd;
        toAdd->prev = prev_temp;
        toAdd->next = NULL;
        list->tail = toAdd;
        list->length++;
    }
    return 1; //return type
}

token_node_t getToken(token_list_t list, int getNum) {
    if(getNum < 0 || getNum >= list->length) {
        return NULL;
    }
    else if (getNum == 0 && list->length == 0) { //list is empty
        return NULL;
    }
    int b = 0;
    token_node_t temp = list->head;
    while(b != getNum) { //Go to getnum of the linked list
        if(temp->next != NULL) {
            temp = temp->next;
            b++;
        }
    }
    return temp;
}

token_list_t tokenize(char* buffer) {
    /*  Creates a linked list of tokens to be easily parsed. Note that
        the buffer passed in is nullbyte terminated. */

    token_list_t list = (token_list_t) malloc(sizeof(struct token_list));
    memcheck(list);
    init_tokenList(list);

    token_node_t last_token = (token_node_t) malloc (sizeof(struct token_node));
    init_toke_node(last_token);
    int index = 0;
    int size = 0;
    int whichLine = 1; //keep track of line number for error reporting
    int k, start;
    int op = 0;   //# open parens
    int ep = 0;   //# end parens
    int lt = 0;   //# of less thans
    int gt = 0;   //# of greater thans

    //loop to eat up any blank space before we start tokenizing
    while (buffer[index] == ' ' || buffer[index] == '\t' ||
        buffer[index] == '\n' || buffer[index] == '#') {
        if (buffer[index] == '#') {
            char buf = buffer[index];
            while (buf != '\n' && buf != '\0') {
                index++;
            buf = buffer[index];
        }
        whichLine++;
        }
        else if (buffer[index] == '\n')
            whichLine++;
                index++;
    }

    while ((k = (char) buffer[index]) != '\0') {
        token_node_t t = (token_node_t) malloc(sizeof(struct token_node));
        init_toke_node(t);
        switch (k) {
            case '\'':
            case '`':
                fprintf(stderr, "%d: Unexpected token encountered 1.\n", whichLine);
                exit(1);
            case ';':
                t->type = SEMICOLON;
                t->string = buffer + index;
                t->lineNumber = whichLine;
                t->length = 1;
                index++;
                break;
            case '|':
                if(buffer[index + 1] == '\0') {//next is a pipe
                    fprintf(stderr, "%d: Unexpected token encountered 2.\n",whichLine);
                    exit(1);
                } 
                else if(buffer[index + 1] == '|') { //error for 3 or more pipes
                    //third character has to be space, alnum, or newline
                    if(buffer[index + 2] == '\0' || (buffer[index + 2] !=' '
                        && !isalnum(buffer[index + 2]) && buffer[index+2] != '\n')) {
                        fprintf(stderr, "%d: Unexpected token encountered 3.\n",whichLine);
                        exit(1);
                    }
                    t->type = OR;
                    t->string = buffer + index;
                    t->lineNumber = whichLine;
                    t->length = 2;
                    index += 2;
                } 
                else {  
                    t->type = PIPE;
                    t->string = buffer + index;
                    t->lineNumber = whichLine;
                    t->length = 1;
                    index++;
                }
                break;
            case '&':
                if(buffer[index + 1] == '\0' //next can't be null
                    || buffer[index+1] != '&' || buffer[index+2] == '\0'  //next has to be &
                    || (buffer[index+2] != ' ' && !isalnum(buffer[index+2]) 
                    && buffer[index+2] != '\n')) { //after && has to be space or alnum
                    fprintf(stderr, "%d: Unexpected token encountered 4.\n",whichLine);
                    exit(1);
                } 
                else {
                    t->type = AND;
                    t->string = buffer + index;
                    t->lineNumber = whichLine;
                    t->length = 2;
                    index+=2; 
                }
                break;
            case '(':
                t->type = OPEN_PARENTHESIS;
                t->string = buffer + index;
                t->lineNumber = whichLine;
                t->length = 1;
                index++;
                op++; //increment open parens
                break;
            case ')':
                t->type = CLOSE_PARENTHESIS;
                t->string = buffer + index;
                t->lineNumber = whichLine;
                t->length = 1;
                ep++; //increment end parens
                index++;
                if (ep > op) {
                    fprintf(stderr, "%d: Unexpected token encountered 5.\n", whichLine);
                    exit(1);
                }
                break;
            case '<':
                if(gt == 0) {
                    lt++;
                }
                else {
                    fprintf(stderr, "%d: Unexpected token encountered 6.\n",whichLine);
                    exit(1);
                }
                t->type = LESS_THAN;
                t->string = buffer + index;
                t->lineNumber = whichLine;
                t->length = 1;
                index++;
                break;
            case '>':
                if(lt <= 1) {
                  gt++;
                }
                else {
                    fprintf(stderr, "%d: Unexpected token encountered 7.\n",whichLine);
                    exit(1);
                }
                t->type = GREATER_THAN;
                t->string = buffer + index;
                t->lineNumber = whichLine;
                t->length = 1;
                index++;
                break;
            case '\n':
                if(last_token->type == SIMPLE || last_token->type == SEMICOLON ||
                    last_token->type == CLOSE_PARENTHESIS) {//previous is simple
                    if(buffer[index + 1] != '\0' && buffer[index +1] == '\n') { //multiple newlines is new command node
                        index++;
                        whichLine++;
                        int newLines = 2;
                        while(buffer[index] == '\n') {
                          index++;
                          whichLine++;
                          newLines++;
                        }
                        t->type = END_COMMAND;
                        t->string = buffer + start;
                        t->lineNumber = whichLine;
                        t->length = newLines;
                    } 
                    else { //single newline is ;
                        //printf("setting semicolon\n");
                        t->type = SEMICOLON;
                        t->string = buffer + start;
                        t->lineNumber = whichLine;
                        t->length = 1;
                        whichLine++;
                        index++;
                    }
                } 
                else {//previous was an operator
                    if(buffer[index +1] != '\0' && buffer[index +1] == '\n') { //ignore all next newlines
                        index++;
                        whichLine++;
                        int newLines = 2;
                        while(buffer[index] == '\n') {
                            index++;
                            whichLine++;
                            newLines++;
                        }
                    } 
                    else {  //ignore single newline
                        whichLine++;
                        index++;
                    }
                }
                break;

            case '#': //must ignore comments up to the next newline
            {
                char buf = buffer[index];
                while (buf != '\n' && buf != '\0') {
                    //printf("peeking at %d,%c\n", index, buffer[index]);
                    index++;
                    buf = buffer[index];
                }
                index++;
                whichLine++;
            }
            break;

            case ' ':
            case '\t':
                index++;

            default:
            /*  Allow all ASCII letters, digits, and ! % + , - . / : @ ^ _ to be
                apart of simple commands. */
                start = index; 
                while (isalnum(buffer[index]) || buffer[index] == ':'
                       || buffer[index] == '/' || buffer[index] == '.'
                       || buffer[index] == '+' || buffer[index] == '@'
                       || buffer[index] == '!' || buffer[index] == '^'
                       || buffer[index] == '%' || buffer[index] == '+'
                       || buffer[index] == '\"' || buffer[index] == '\\' 
                       || buffer[index] == '\'' || buffer[index] == '-'
                       || buffer[index] == ',') {
                    size++;
                    index++;
                } 
                t->type = SIMPLE;
                t->string = buffer + start;
                t->lineNumber = whichLine;
                t->length = size;
                break;
        } //end switch

        size = 0;
        if(t->type != SIMPLE && t->type != LESS_THAN && t->type != GREATER_THAN) {
            lt = 0;
            gt = 0;
        }
        if(t->length != 0) {
            if(t->type == LESS_THAN || t->type == GREATER_THAN)  {
                //if the current is a redirect and last token wasn't simple or closed parentheses
                if(last_token->type != SIMPLE && last_token->type != CLOSE_PARENTHESIS) {
                    fprintf(stderr, "%d: Unexpected token encountered 8. %d\n",whichLine, t->type);
                    exit(1);
                }
            } 
            else if((t->type != SIMPLE) && (last_token->length == 0 || last_token->type != SIMPLE) &&
                    t->type != OPEN_PARENTHESIS && last_token->type != CLOSE_PARENTHESIS) {// all operators must be preceded by commands
                fprintf(stderr, "%d: Unexpected token encountered 9.\n", whichLine);
                exit(1);
            }
            last_token = t;
            addToken(list, t);
        }
    } //end while
    if(!(last_token->type == SIMPLE || last_token->type == CLOSE_PARENTHESIS
        ||last_token->type == END_COMMAND) || (op != ep)) {
        fprintf(stderr, "%d: Unexpected token encountered 10.\n", whichLine);
        exit(1);
    }
    return list;
}

//converts a token to a command to push onto the cmd_stack
command_t tokenToCommand(token_node_t node) {
    /*  Converts a token to a command to push onto cmd_stack. */
    if(node->type == SIMPLE) {
        command_t c1 = (command_t) malloc(sizeof(struct command));
        c1->u.word = (char**) malloc(sizeof(char*) * 2);
        char *p = (char *) malloc(sizeof(char) * (node->length + 1));
        for(int a = 0; a < node->length ; a++) {
            p[a] = node->string[a];
        }

        p[node->length] = '\0';
        c1->u.word[0] = p;
        c1->u.word[1] = NULL;

        c1->input = NULL;
        c1->output = NULL;
        c1->type = SIMPLE_COMMAND;
        c1->status = -1;

        return c1;
    }
    return NULL;
}

command_t combineSimples(token_node_t node, int count) {
    token_node_t cur = node; //hold each token
    command_t c1 = (command_t) malloc(sizeof(struct command));
    c1->u.word = (char**) malloc(sizeof(char*) * (count+1)); //#words + end null
    char *p; //hold each word
    for (int a = 0;  a < count ; a++) {
        p = (char *) malloc(sizeof(char) * (cur->length + 1));

        for (int a = 0; a < cur->length ; a++)
            p[a] = cur->string[a]; //copy entire string

        p[cur->length] = '\0'; //place null byte at end of word
        c1->u.word[a] = p; //set word
        cur = cur->next; //prepare the next word to be added 
    }

    c1->u.word[count] = NULL; //add null at the end
    c1->type = SIMPLE_COMMAND;
    return c1;
}

command_t combine(command_t c1, command_t c2, int op) {
    /*  Combines two commands with a given operator into a new command. */
    command_t new = (command_t) malloc(sizeof(struct command));
    new->u.command[0] = c1;
    new->u.command[1] = c2;
    new->input = NULL;
    new->output= NULL;
    new->status = -1;
    if (op == AND) {
        new->type = AND_COMMAND;
    }
    else if (op == OR) {
        new->type = OR_COMMAND;
    }
    else if (op == PIPE) {
        new->type = PIPE_COMMAND;
    }
    else if (op == SEMICOLON) {
        new->type = SEQUENCE_COMMAND;
    }
    return new;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{ 
    /* read all of stdin into a buffer to process */
    int bufsize = 50;
    int index = 0;
    char *buffer = buffer = (char*) malloc(sizeof(char) * bufsize);
    memcheck(buffer);

    for (int k = 0; (k = get_next_byte(get_next_byte_argument)) != EOF;) {
        if (index == bufsize) {
            bufsize += 50;
            buffer = (char*) realloc(buffer, sizeof(char) * bufsize);
            memcheck(buffer);
        }
        buffer[index++] = k;
    }

    //terminate the buffer array with a nullbyte
    if (index == bufsize) {
        bufsize += 3;  //increase by 1 for nullbyte
        buffer  = (char*) realloc(buffer, sizeof(char) * bufsize);
        memcheck(buffer);
    }
    buffer[index++] = '\n';
    buffer[index++] = '\n';
    buffer[index++] = '\0';

    op_stack_t  os = (op_stack_t) malloc(sizeof(struct op_stack));
    cmd_stack_t cs = (cmd_stack_t) malloc(sizeof(struct cmd_stack));
    token_list_t tl = (token_list_t) malloc(sizeof(struct token_list));
    command_stream_t result = (command_stream_t) malloc(sizeof(struct command_stream));
    command_node_t n;
    token_node_t temp;

    n = (command_node_t) malloc(sizeof(struct command_node));
    init_cmd_node(n);
    init_opStack(os);
    init_cmdStack(cs);
    init_tokenList(tl);
    init_cmdStream(result);
    tl = tokenize(buffer);

    //parse precedence and then create command_nodes
    index = 0;
    while ((temp = getToken(tl, index)) != NULL) {
        switch (temp->type) 
        {
            case SEMICOLON:
            case PIPE:
            case OR:
            case AND:
            case OPEN_PARENTHESIS:
                if (os->size == 0 || temp->type == OPEN_PARENTHESIS) {
                    opPush(os, temp->type);
                    index++;
                }
                else {
                    if (/*temp->type != CLOSE_PARENTHESIS && */(precedence(temp->type) > precedence(opPeek(os)))) {
                        opPush(os, temp->type);
                        index++;
                    }
                    else {
                        while (opPeek(os) != OPEN_PARENTHESIS &&
                               (precedence(temp->type) <= precedence(opPeek(os)))) {
                            int operator = opPop(os);
                            command_t c2 = cmdPop(cs);
                            command_t c1 = cmdPop(cs);
                            cmdPush(cs, combine(c1, c2, operator));
                            if (os->size == 0)
                              break;
                        } //end while
                        opPush(os, temp->type);
                        index++;
                    } //end else
                } //end else
                break;

            case CLOSE_PARENTHESIS: ;
                int topOp = opPop(os);
                index++;
                while (topOp != OPEN_PARENTHESIS) { //cant pop two off stack is just (A)!!
                    if(cs->size >1) {
                        command_t c2 = cmdPop(cs);
                        command_t c1 = cmdPop(cs);
                        cmdPush(cs, combine(c1, c2, topOp));
                        topOp = opPop(os); 
                    }
                    else {
                        command_t c1 = cmdPop(cs);
                        cmdPush(cs, c1);
                    }
                }
                //now, create a subshell command
                command_t subshell = (command_t) malloc(sizeof(struct command));
                subshell->type = SUBSHELL_COMMAND;
                subshell->status = -1;
                subshell->input = NULL;
                subshell->output = NULL;
                subshell->u.subshell_command = cmdPop(cs);
                cmdPush(cs, subshell);
                break;

            case LESS_THAN: ;
                command_t com = cmdPop(cs);
                token_node_t toke = getToken(tl, index+1);
                char *in = (char*) malloc(sizeof(char) * toke->length+1);
                for (int k = 0; k < toke->length; k++)
                    in[k] = toke->string[k];
                in[toke->length] = '\0';
                com->input = in;
                cmdPush(cs, com);
                index += 2;
                break;

            case GREATER_THAN: ;
                command_t com1 = cmdPop(cs);
                token_node_t toke1 = getToken(tl, index+1);
                char *out = (char*) malloc(sizeof(char) * toke1->length+1);
                for (int k = 0; k < toke1->length; k++)
                    out[k] = toke1->string[k];
                out[toke1->length] = '\0';
                com1->output = out;
                cmdPush(cs, com1);
                index += 2;
                break;

            case SIMPLE:
                if (getToken(tl, index+1) != NULL) {
                    int tempindex = index+1;
                    int simples = 1;
                    while (getToken(tl, tempindex) != NULL && getToken(tl, tempindex)->type == SIMPLE) {
                        tempindex++;
                        simples++;
                    }

                    if (simples > 1) {  //Must create a SIMPLE command composed of many words
                    cmdPush(cs, combineSimples(temp, simples));
                    index += simples;
                    }
                    else {  //encountered a SIMPLE command of one word
                        cmdPush(cs, tokenToCommand(temp));
                        index++;
                    }
                }
                else {  //encountered a SIMPLE command of one word
                    cmdPush(cs, tokenToCommand(temp));
                    index++;
                }
                break;

            case END_COMMAND: //remember to index++
                if (os->size > 0) {
                    //process everything in the stacks;
                    while (os->size > 0) {
                        int operator = opPop(os);
                        command_t c2 = cmdPop(cs);
                        command_t c1 = cmdPop(cs);
                        //printf("combining %d and %d with operator %d\n", c1->type, c2->type,operator);
                        cmdPush(cs, combine(c1, c2, operator));
                    }
                    n->cmdPtr = cmdPop(cs);
                    addCmd(result, n);
                    n = (command_node_t) malloc(sizeof(struct command_node));
                    init_cmd_node(n);
                }
                else if(cs->size > 0) {  //Only a simple command on the command_stack
                    n->cmdPtr = cmdPop(cs);
                    addCmd(result, n);
                    n = (command_node_t) malloc(sizeof(struct command_node));
                    init_cmd_node(n);
                }
                index++;
                break;

            default:
                break;
        } //end switch
    } //end while
    //check if stacks not empty and finish processing them
    if (os->size > 0) {
        //process everything in the stacks;
        while (os->size > 0) {
            int operator = opPop(os);
            command_t c2 = cmdPop(cs);
            command_t c1 = cmdPop(cs);
            cmdPush(cs, combine(c1, c2, operator));
        }
        n->cmdPtr = cmdPop(cs);
        addCmd(result, n);
        n = (command_node_t) malloc(sizeof(struct command_node));
        init_cmd_node(n);
    }
    else if (cs->size > 0) {  //Only a simple command on the command_stack
        n->cmdPtr = cmdPop(cs);
        addCmd(result, n);
        n = (command_node_t) malloc(sizeof(struct command_node));
        init_cmd_node(n);
    }
    return result;
}

command_t read_command_stream (command_stream_t s) {
    command_node_t n = getCmd(s);
    command_t c = NULL;
    if (n != NULL) { 
        c = n->cmdPtr;
        if(c == NULL) {
        }
        return c;
    }
    return NULL;
}
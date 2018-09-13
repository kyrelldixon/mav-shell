// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10     // Mav shell only supports ten arguments

#define EXEC_FILE_INDEX 0

int handle_tokens( char *tokens[], int token_count );
int exec_to_completion( char *tokens[] );
void print_tokens( char* tokens[], int token_count );

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *tokens[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      tokens[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( tokens[token_count] ) == 0 )
      {
        tokens[token_count] = NULL;
      }
        token_count++;
    }

    handle_tokens( tokens, token_count );

    free( working_root );

  }
  return 0;
}

int handle_tokens( char *tokens[], int token_count )
{
  char* exec_file = tokens[EXEC_FILE_INDEX];

  // If the command is cd, I have to use a different handler.
  // I'm checking it first to move past that case.
  if ( strcmp("cd", exec_file) == 0 )
  {
    printf("Changing dir\n");
    return 0;
  }
  else if (strcmp("ls", exec_file) == 0)
  {
    return exec_to_completion( tokens );
  }
  else {
    printf("Have not implemented function %s\n", exec_file);
  }

  printf("Command is %s\n", exec_file);
  return 0;
}

/*
  fork() a child and then use exec to execute ls
*/

int exec_to_completion( char *tokens[] )
{
  pid_t child_pid = fork();
  int status;

  if ( child_pid == 0 )
  {
    execv("/bin/ls", tokens);
    exit(EXIT_SUCCESS);
  }

  waitpid(child_pid, &status, 0);

  return 0;
}

void print_tokens( char* tokens[], int token_count )
{
  for (int i = 0; i < token_count; i++)
  {
    printf("token[%d]: %s\n", i, tokens[i]);
  }
}
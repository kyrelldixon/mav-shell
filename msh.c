/*

  Name: Kyrell Dixon
  ID: 1001084446

*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"          // We want to split our command line up into tokens
                                    // so we need to define what delimits our tokens.
                                    // In this case  white space
                                    // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255        // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10        // Mav shell only supports ten arguments

#define EXEC_FILE_INDEX 0           // The file to execute should always be at index 0
#define FILE_PATH_INDEX 1           // The file path (for "cd") should always be at index 1
#define MAX_HISTORY_CMDS 15
#define MAX_PIDS 15

char *cmd_history[MAX_HISTORY_CMDS];// Used to keep track of the last 15 commands
int num_cmds_in_history = 0;
pid_t pids[MAX_PIDS];
int num_pids_in_pids = 0;

int tokenize_string( char* cmd_str, char *tokens[], int token_count );
int handle_tokens( char *tokens[], int token_count );
int exec_to_completion( char *tokens[], int token_count );
int handle_exec_error( char* cmd );
int savepid( pid_t pid );
int savehist( char *cmd_str );
void showhist();
void showpids();

void print_tokens( char *tokens[], int token_count );
char *concat_strings( char *s1, char *s2 );
char *concat_path( char *path, char *filename );
bool streq( char *s1, char *s2 );

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

    // Before tokenizing, save the input to history
    savehist( cmd_str );

    /* Parse input */
    char *tokens[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    token_count = tokenize_string( cmd_str, tokens, token_count );

    // Checks to make sure there are actual tokens
    // before attempting to handle them. If NULL
    // I want to start taking input again.
    if ( tokens[EXEC_FILE_INDEX] ) {
      handle_tokens( tokens, token_count );
    }
    
  }

  free ( cmd_str );
  for (int i = 0; i < num_cmds_in_history; i++)
  {
    free ( cmd_history[i] );
  }

  free ( cmd_history );

  return 0;
}

/**
 * Decides functions to call based on tokens
 * 
 * @param tokens List of tokens processed from user input
 * @param token_count The total number of tokens in tokens array
 * @return 0 if successful
 */
int handle_tokens( char *tokens[], int token_count )
{
  char *cmd = tokens[EXEC_FILE_INDEX];

  if ( streq("quit", cmd) || streq("exit", cmd) )
  {
    // Exit with 0 status based on requirements
    exit(0);
  }
  // If the command is cd, I have to use a different handler
  // so I check for that separately
  if ( streq("cd", cmd) )
  {
    chdir(tokens[FILE_PATH_INDEX]);
  }

  else if ( streq("history", cmd) )
  {
    showhist();
  }

  else if ( streq("listpids", cmd) )
  {
    showpids();
  }

  else if ( streq("bg", cmd) )
  {
    printf("getting bg\n");
  }

  else {
    // If none of the above checks are true, then the
    // user just wants to execute some command
    exec_to_completion(tokens, token_count);
  }

  return 0;
}

/**
 * Forks a child and uses exec to execute
 * 
 * @param tokens List of tokens processed from user input
 * @param token_count The total number of tokens in tokens array
 * @return 0 if successful
 */
int exec_to_completion( char *tokens[], int token_count )
{
  pid_t child_pid = fork();
  int status;

  // PATH variable constants for checking where file
  // should execute from
  char *PATH_BIN = "/bin";
  char *PATH_USR_BIN = "/usr/bin";
  char *PATH_USR_LOCAL_BIN = "/usr/local/bin";

  char *exec_file = tokens[EXEC_FILE_INDEX];
  char *filepath = concat_path(PATH_BIN, exec_file);

  if ( child_pid == 0 )
  {
    if ( execv(filepath, tokens) < 0)
    {
      handle_exec_error( exec_file );
    }
    free(filepath); // The calling function should free the memory
    exit(EXIT_SUCCESS);
  }
  else
  {
    savepid(child_pid);
  }

  waitpid(child_pid, &status, 0);
  
  return 0;
}

/**
 * Error handler for issues with the exec function
 * Determines steps to take on error from the exec()
 * function
 * 
 * @param exec_file The name of the file that resulted in
 *                  the error
 * @return 0 if successful
 */
int handle_exec_error( char* exec_file )
{
  if ( errno == 2 ) {
    printf("%s: Command not found.\n", exec_file);
  }
  return 0;
}

/**
 * Saves pid for child resulting from fork() command
 * 
 * @param pid The child pid to save
 * @return 0 if successful, 1 if not
 */
int savepid( pid_t pid )
{
  if (num_pids_in_pids >= MAX_PIDS)
  {
    return 1;
  }

  pids[num_pids_in_pids++] = pid;
  return 0;
}

/**
 * Displays the past commands entered by the user
 * up to a max of MAX_PIDS
 * 
 * @return nothing
 */
void showpids()
{
  for (int i = 0; i < num_pids_in_pids; i++)
  {
    // Printing i + 1 since we want a more user friendly
    // display index.
    printf("%d: %d\n", (i + 1), pids[i]);
  }
}

/**
 * Saves a command to the cmd_history array and increments
 * total number of commands
 * 
 * @param cmd_str The initial string entered by the user
 *                before tokenization and whitespace removal
 * @return 0 if successful, 1 if max commands reached
 */
int savehist( char *cmd_str )
{
  if ( num_cmds_in_history >= MAX_HISTORY_CMDS )
  {
    return 1;
  }

  cmd_history[num_cmds_in_history++] = strdup( cmd_str );
  return 0;
}

/**
 * Displays the past commands entered by the user
 * up to a max of MAX_COMMAND_SIZE
 * 
 * @return nothing
 */
void showhist()
{
  for (int i = 0; i < num_cmds_in_history; i++)
  {
    // Printing i + 1 since we want a more user friendly
    // display index. Also needs to match the !n syntax
    // for executing from history
    printf("[%d]: %s", (i + 1), cmd_history[i]);
  }
}

/*
 * < -------------- UTILITIES -------------- >
 * This is a set of function that do nothing
 * but make my life easier.
*/

/**
 * Takes a string delimitted by whitespace and stores
 * each set of characters as a token in tokens array
 * 
 * @param cmd_str The string to be tokenized
 * @param tokens List of tokens processed from user input
 * @param token_count The total number of tokens in tokens array
 * @return the updated token_count
 */
int tokenize_string( char *cmd_str, char *tokens[], int token_count )
{
  // Pointer to point to the token
  // parsed by strsep
  char *arg_ptr;

  char *working_str = strdup(cmd_str);

  // we are going to move the working_str pointer so
  // keep track of its original value so we can deallocate
  // the correct amount at the end
  char *working_root = working_str;

  // Tokenize the input strings with whitespace used as the delimiter
  while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
         (token_count < MAX_NUM_ARGUMENTS))
  {
    tokens[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
    if (strlen(tokens[token_count]) == 0)
    {
      tokens[token_count] = NULL;
    }
    token_count++;
  }

  free( working_root );

  return token_count;
}

/**
 * Prints tokens nicely formatted for debugging
 * 
 * @param tokens List of tokens processed from user input
 * @param token_count The total number of tokens in tokens array
 * @return nothing
 */
void print_tokens( char *tokens[], int token_count )
{
  for (int i = 0; i < token_count; i++)
  {
    printf("token[%d]: %s\n", i, tokens[i]);
  }
}

/**
 * Concatenates second string to end of the first
 * 
 * @param s1 The first string to concatenate
 * @param s2 The second string to concatenate
 * @return The concatenated string
 */
char *concat_strings(char *s1, char *s2)
{
  char *concatted_string = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator

  strcpy(concatted_string, s1);
  strcat(concatted_string, s2);
  return concatted_string;
}

/**
 * Concatenates a file and a path
 * 
 * @param path Some path to be concatenated
 * @param filename Some filename to be appended to path
 * @return The complete filepath
 */
char *concat_path(char *path, char *filename)
{
  // +1 for the null-terminator
  // +1 more for the "/" character
  char *result = malloc(strlen(path) + strlen(filename) + 2);
  char *str = concat_strings(path, "/");
  result = concat_strings(str, filename);

  return result;
}

/**
 * Checks if strings are equal
 * 
 * @param s1 The first string to compare
 * @param s2 The second string to compare
 * @return true if all characters match, otherwise false
 */
bool streq( char *s1, char *s2 )
{
  if ( strcmp( s1, s2 ) == 0 )
  {
    return true;
  }
  return false;
}
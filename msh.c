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
#include <limits.h>

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
int num_cmds_entered = 0;
pid_t pids[MAX_PIDS];
int num_pids_in_pids = 0;

int tokenize_string( char *cmd_str, char *tokens[], int token_count );
int handle_tokens( char *tokens[], int token_count );
int exec_to_completion( char *tokens[], int token_count );
int exec_from_history( int history_val );
int savepid( pid_t pid );
int savehist( char *cmd_str );
void showhist();
void showpids();
static void handle_signal( int sig );

void print_tokens( char *tokens[], int token_count );
char *concat_strings( char *s1, char *s2 );
char *concat_path( char *path, char *filename );
bool streq( char *s1, char *s2 );

int main( void )
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  struct sigaction act;

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    /*
    Zero out the sigaction struct
    */
    memset(&act, '\0', sizeof(act));

    /*
    Set the handler to use the function handle_signal()
    */
    act.sa_handler = &handle_signal;

    /* 
    Install the handler for SIGINT and SIGTSTP and check the 
    return value.
    */
    if (sigaction(SIGINT, &act, NULL) < 0)
    {
      perror("sigaction: ");
      return 1;
    }

    if (sigaction(SIGTSTP, &act, NULL) < 0)
    {
      perror("sigaction: ");
      return 1;
    }

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

  // Gets actual value of commands so it will free accurately
  int num_cmds_in_history = num_cmds_entered % MAX_HISTORY_CMDS;
  int i;
  for (i = 0; i < num_cmds_in_history; i++)
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

  // else if ( streq("bg", cmd) )
  // {
  //   printf("getting bg\n");
  // }

  // an "!" as the first character in a command means
  // the user is attempting to execute a command from
  // history. This handles that use case
  else if ( cmd[0] == '!' )
  {
    int history_val = atoi( cmd + 1 );
    exec_from_history( history_val );
  }

  else {
    // If none of the above checks are true, then the
    // user just wants to execute some command
    exec_to_completion(tokens, token_count);
  }

  return 0;
}

/**
 * Forks a child and uses exec to execute from various filepaths
 * 
 * @param tokens List of tokens processed from user input
 * @param token_count The total number of tokens in tokens array
 * @return 0 if successful
 */
int exec_to_completion( char *tokens[], int token_count )
{
  pid_t child_pid = fork();
  int status;

  // should match the number of paths in paths array
  int NUM_PATHS = 4;

  // gets curr directory to add to paths for requirements.
  // using PATH_MAX for size since user file could be deeply
  // nested in folders
  char cwd[PATH_MAX];
  getcwd(cwd, sizeof(cwd));

  // ORDER MATTERS. Should execute command from path in order presented 
  char *paths[] = {
      "/usr/local/bin",
      "/usr/bin",
      "/bin",
      cwd
  };

  char *exec_file = tokens[EXEC_FILE_INDEX];
  char *filepath;

  if ( child_pid == 0 )
  {
    // Goes through each file and attempts to exec in the order
    // of paths given. Will exit loop prematurely if exec_file
    // exists on that path
    int i;
    for (i = 0; i < NUM_PATHS; i++)
    {
      filepath = concat_path(paths[i], exec_file);
      execv(filepath, tokens);
    }
    
    // If no file doesn't execute and loop reaches this point, then the
    // command doesn't exist so printing command not found
    printf("%s: Command not found.\n", exec_file);

    free(filepath);
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
 * Used to handle "!n" styled commands. Handles commands
 * based on the position in the cmd_history array
 * 
 * @param history_val The "n" part of "!n". Represents
 *                    command from user presented list
 *                    after entering "history" command
 * @return 0 if successful
 */

int exec_from_history( int history_val )
{
  if ( (history_val > 0) && (history_val <= num_cmds_entered) )
  {
    // The actual indices are 1 - value shown to the user
    // since we display them starting at index 1
    int history_index = history_val - 1;
    
    // store a new set of tokens
    char *tokens[MAX_NUM_ARGUMENTS];

    // get a new token count a update tokens based
    // on cmd from history.
    int token_count = 0;
    token_count = tokenize_string(cmd_history[ history_index ], tokens, token_count);
    handle_tokens(tokens, token_count);
  }
  
  else
  {
    printf("Command not in history.\n");
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
  int i;
  for (i = 0; i < num_pids_in_pids; i++)
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
  // using % the max num of cmds so that it will loop
  // and overwrite commands. This is so we have the last
  // 15 commands entered.
  int history_index = num_cmds_entered++ % MAX_HISTORY_CMDS;
  cmd_history[history_index] = strdup( cmd_str );
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
  int num_cmds_to_print = ( num_cmds_entered >= 15 ) ? 15 : num_cmds_entered;
  int i;
  for (i = 0; i < num_cmds_to_print; i++)
  {
    // Printing i + 1 since we want a more user friendly
    // display index. Also needs to match the !n syntax
    // for executing from history
    printf("[%d]: %s", (i + 1), cmd_history[i]);
  }
}

static void handle_signal(int sig)
{

  /*
   Determine which of the two signals were caught and 
   print an appropriate message.
  */

  switch (sig)
  {
  case SIGINT:
    break;

  case SIGTSTP:
    break;

  default:
    break;
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
  int i;
  for (i = 0; i < token_count; i++)
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

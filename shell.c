#include "shell.h"

char **arguments;

// ============================================================================
// Execute a child process.
// Returns -1
// on failure.  On success, does not return to caller.
// ============================================================================
int child(char **args)
{
  int i = 0;
  while (args[i] != NULL)
  {
    if (equal(args[i], ">"))
    {
      // Redirect stdout to the file
      if (freopen(args[i + 1], "w", stdout) == NULL)
      {
        perror("Error redirecting stdout");
        return 1;
      }

      args[i] = NULL;
    }
    else if (equal(args[i], "<"))
    {
      // Redirect stdin to the file
      if (freopen(args[i + 1], "r", stdin) == NULL)
      {
        perror("Error redirecting stdout");
        return 1;
      }

      args[i] = NULL;
    }
    else if (equal(args[i], "|"))
    {
      // do pipe in a separate function
      doPipe(args, i);
    }
    else
    {
      ++i;
    }
  }

  // call execvp on prepared arguments after while loop ends. You can modify
  // arguments as you loop over the arguments above.
  execvp(args[0], args);
  return -1;
}

// ============================================================================
// Execute the shell command that starts at args[start] and ends at args[end].
// For example, with
//   args = {"ls" "-l", ">", "junk", "&", "cat", "hello.c"}
// start = 5 and end = 6, we would perform the command "cat hello.c"
//
// Or, with args = {"ls", "|", "wc"} and start = 0 and end = 2, we would
// execute: ls | wc
// ============================================================================
void doCommand(char **args, int start, int end, bool waitfor)
{
  // you will have your classic fork() like implementation here.
  // always execute your commands in child. so pass in arguments there
  // based on waitfor flag, in parent implement wait or not wait  based on & or
  // ;
  int subarraySize = end - start + 1;

  // Allocate memory for the subarray
  char **subargs =
      (char **)malloc(subarraySize * sizeof(char) * MAX_ARG_LENGTH);
  if (subargs == NULL)
  {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  // Copy elements from the original array to the subarray
  for (int i = start, j = 0; i <= end; i++, j++)
  {
    subargs[j] = args[i];
  }

  int pid = fork();
  if (pid < 0)
  {
    perror("Error during fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0)
  { // Child
    // Execute in the child function
    child(subargs);

    // If execvp returns, an error occurred
    perror("execvp");
    exit(EXIT_FAILURE);
  }
  // Parent
  if (waitfor)
  {
    wait(NULL);
  }
}

// ============================================================================
// Execute the two commands in 'args' connected by a pipe at args[pipei].
// For example, with
//   args = {"ls" "-a", "|", "wc""}
// and pipei = 2, we will perform the command "ls -a | wc", pipei is the index
// of the pipe, so you can split commands between parent and child.
//
// We split off the command for the parent from 'args' into 'parentArgs'.  In
// our example, parentArgs = {"ls", "-a"}
//
// The parent will write, via a pipe, to the child
// ============================================================================
int doPipe(char **args, int pipei)
{
  enum
  {
    READ,
    WRITE
  };
  char *parentArgs[pipei + 1];

  for (int i = 0; i < pipei; i++)
  {
    parentArgs[i] = args[i];
  }
  parentArgs[pipei] = NULL; // NULL terminate parentArgs array

  int pipefd[2];
  if (pipe(pipefd) == -1)
  {
    perror("Error creating pipe");
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  if (pid < 0)
  {
    perror("Error during fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0)
  { // Child
    close(pipefd[WRITE]);
    if (dup2(pipefd[0], STDIN_FILENO) == -1)
    {
      perror("Error redirecting stdin");
      exit(EXIT_FAILURE);
    }

    close(pipefd[READ]);
    execvp(args[pipei + 1], &args[pipei + 1]);

    perror("Child Piping execvp"); // if execvp returns then error occurred
    exit(EXIT_FAILURE);
  }
  else
  { // Parent process
    close(pipefd[READ]);

    if (dup2(pipefd[WRITE], STDOUT_FILENO) == -1)
    {
      perror("Error redirecting stdout");
      exit(EXIT_FAILURE);
    }

    close(pipefd[1]);
    execvp(parentArgs[0], parentArgs);
    perror("Parent Piping execvp"); // If execvp returns, an error occurred
    exit(EXIT_FAILURE);
  }
}

// ============================================================================
// Parse the shell command, starting at args[start].  For example, if
// start = 0 and args holds:
//    {"ls", "-a", ">", "junk.txt", "&", "cat", "hello.c", ";"}
// then parse will find the "&" terminator at index 4, and set end to this
// value.  This tells the caller that the current command runs from index
// 'start' thru index 'end'.
//
// We return true if the command terminates with ";" or end-of-line.  We
// return false if the command terminates with "&"
// ============================================================================
bool parse(char **args, int start, int *end)
{
  int i = start;
  while (args[i] != NULL && !equal(args[i], ";") && !equal(args[i], "&") &&
         !equal(args[i], "\0") && i < MAX_ARGS)
  {
    i++;
  }
  *end = i - 1;
  if (equal(args[i], "&"))
    return false;
  return true;
}

// ============================================================================
// Tokenize 'line'.  Recall that 'strtok' writes NULLs into its string
// argument to delimit the end of each sub-string.
// Eg: line = "ls -a --color" would produce tokens: "ls", "-a" and "--color"
// ============================================================================
char **tokenize(char *line)
{
  // initalize array with NULL values
  char *token;
  token = strtok(line, " ");
  if (token != NULL && equal(token, "!!"))
  {
    return arguments;
  }
  for (int i = 0; i < MAX_ARGS; i++)
  {
    arguments[i] = NULL;
  }
  arguments[0] = token;
  int i = 1;
  while (token != NULL)
  {
    token = strtok(NULL, " ,\0");
    arguments[i] = token;
    i++;
  }
  return arguments;
}

bool processLine(char *line)
{

  int start = 0; // index into args array
  if (equal(line, "exit"))
  {
    return false;
  }

  if (equal(line, ""))
  {
    return true;
  }

  if (equal(line, "!!"))
  {
    // don't have to do anything here, handled by tokenize
  }

  // todo: does this need to match what is in main?
  // processLine(line);
  // process lines
  char **args = tokenize(line); // split string into tokens
  // loop over to find chunk of independent commands and execute
  while (args[start] != NULL)
  {
    int end;
    bool waitfor = parse(
        args, start,
        &end);                            // parse() checks if current command ends with ";" or "&"
                                          // or nothing. if it does not end with anything treat it
                                          // as ; or blocking call. Parse updates "end" to the index
                                          // of the last token before ; or & or simply nothing
    doCommand(args, start, end, waitfor); // execute sub-command
    start = end + 2;                      // next command
  }
  wait(NULL);
  return true;
}

// ============================================================================
// Main loop for our Unix shell interpreter
// ============================================================================
int main()
{
  // bool should_run = false; // loop until false
  bool runTestsBool = false;
  arguments = calloc(MAX_ARGS, sizeof(char *));
  if (!runTestsBool)
  {
    interactiveShell();
  }
  else
  {
    runTests();
  }
  return 0;
}

// interactive shell to process commands
int interactiveShell()
{
  printf("interactive Shell\n");
  bool should_run = true;
  char *line = calloc(1, MAXLINE);

  while (should_run)
  {
    printf(PROMPT); // PROMPT is osh>
    fflush(stdout);
    int n = fetchline(&line);
    printf("read: %s (length = %d)\n", line, n);
    fflush(stdout);
    // ^D results in n == -1
    if (n == -1 || !processLine(line)) // exit ect. causes false process line
    {
      should_run = false;
      continue;
    }
  }
  free(line);
  return 0;
}

int runTests()
{
  printf("*** Running basic tests ***\n");
  char lines[7][MAXLINE] = {"ls", "ls -al", "ls & whoami ;",
                            "!!", "ls > junk.txt", "cat < junk.txt",
                            "ls | wc"};
  for (int i = 0; i < 7; i++)
  {
    printf("* %d. Testing %s *\n", i + 1, lines[i]);
    processLine(lines[i]);
  }

  return 0;
}

// return true if C-strings are equal
bool equal(char *a, char *b)
{
  return a != NULL && b != NULL && (strcmp(a, b) == 0);
}

// read a line from console
// return length of line read or -1 if failed to read
// removes the \n on the line read
int fetchline(char **line)
{
  size_t len = 0;
  size_t n = getline(line, &len, stdin);
  if (n > 0)
  {
    (*line)[n - 1] = '\0';
  }
  return n;
}
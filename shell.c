#include "shell.h"

char **arguments;

// ============================================================================
// Main loop for our Unix shell interpreter
// ============================================================================
int main()
{
  bool runTestsBool = true;
  arguments = calloc(MAX_ARGS, sizeof(char *));
  if (!runTestsBool) { 
    interactiveShell();
  } else {
    runTests();
  }
  return 0;
}

// ============================================================================
// Execute a child process.
// Returns -1
// on failure.  On success, does not return to caller.
// ============================================================================
int child(char **args)
{
  int i = 0;
  /*
  while (args[i] != NULL) {
    if (*args[i] == '`') {
      
    }
    i++;
  }
  i = 0;*/
  while (args[i] != NULL) {
    if (equal(args[i], ">")) {
      // Redirect stdout to the file
      if (freopen(args[i + 1], "w", stdout) == NULL) {
        perror("Error redirecting stdout");
        return 1;
      }
      args[i] = NULL;
    } else if (equal(args[i], "<")) {
      // Redirect stdin to the file
      if (freopen(args[i + 1], "r", stdin) == NULL) {
        perror("Error redirecting stdin");
        return 1;
      }
      args[i] = NULL;
    } else if (equal(args[i], "|")) {
      doPipe(args, i, 0);
    } else { ++i; 
    }
  }

  // ascii command
  if (equal(args[0], "ascii")) { asciiArt();
  } else {
    execvp(args[0], args);
    // call execvp on prepared arguments after while loop ends. You can modify
    // arguments as you loop over the arguments above.
    return -1;
  }
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
  int subarraySize = end - start + 1;

  // Allocate memory for the subarray
  char **subargs = 
    (char **)malloc(subarraySize * sizeof(char) * MAX_ARG_LENGTH);
  if (subargs == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  // Copy elements from the original array to the subarray
  for (int i = start, j = 0; i <= end; i++, j++) {
    subargs[j] = args[i];
  }

  int pid = fork();
  if (pid < 0) {
    perror("Error during fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) { // Child
    // Execute in the child function
    child(subargs);
    // If execvp returns, an error occurred
    exit(EXIT_FAILURE);
  }
  // Parent
  if (waitfor) {
    wait(NULL);
  }
}

// ============================================================================
// Removes the backticks from the command and executes the command inside the
// backticks. Returns the result of the command.
// Note: only works for single-word commands and only returns a single string.
// ============================================================================
char *doBacktick(char **args, int index)
{
  enum { READ, WRITE };
  char command[strlen(args[index]) - 2];
  for (int j = 1; j < strlen(args[index]) - 1; j++) {
      command[j - 1] = args[index][j];
  }
  command[strlen(command)] = '\0';

  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("Error creating pipe");
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  if (pid < 0) {
    perror("Error during fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) { // Child
    close(pipefd[READ]);
    if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
      perror("Error redirecting stdout");
      exit(EXIT_FAILURE);
    }
    execlp(command, command, NULL);
    perror("Child Piping execvp"); // if execvp returns then error occurred
    exit(EXIT_FAILURE);
  } else { // Parent process
  
    close(pipefd[WRITE]);
    int original_stdin = dup(STDIN_FILENO);
    if (dup2(pipefd[0], STDIN_FILENO) == -1) {
      perror("Error redirecting stdin");
      exit(EXIT_FAILURE);
    }
    dup2(original_stdin, STDIN_FILENO); // Restore the original stdin
    close(original_stdin);
    char *result = (char *)malloc(BUF_SIZE);
    read(pipefd[0], result, BUF_SIZE);
    result[strlen(result) - 1] = '\0'; // remove newline character from result
    printf("Result: %s\n", result);
    close(pipefd[0]);
    return result;
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
int doPipe(char **args, int pipei, int start)
{
  enum { READ, WRITE };
  char *parentArgs[pipei + 1];

  for (int i = 0; i < pipei; i++) {
    parentArgs[i] = args[i];
  }
  parentArgs[pipei] = NULL; // NULL terminate parentArgs array

  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("Error creating pipe");
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  if (pid < 0) {
    perror("Error during fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) { // Child
    close(pipefd[WRITE]);
    if (dup2(pipefd[0], STDIN_FILENO) == -1) {
      perror("Error redirecting stdin");
      exit(EXIT_FAILURE);
    }

    // Multi-Piping
    for (int i = pipei + 1; args[i] != NULL; i++) {
      if (equal(args[i], "|")) {
        args[i] = NULL;
        doPipe(args, i, pipei + 1);
        return 0;
      }
      if (*args[i] == '`') {
        args[i] = doBacktick(args, i);
      }
    }

    //    close(pipefd[READ]);
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
    execvp(parentArgs[start], &parentArgs[start]);
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

// ============================================================================
// this method processes a line and executes the commands in the line
// ============================================================================
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
    // most of this is handled by tokenize
    if (arguments[0] == NULL)
    {
      printf("No commands in history.\n");
      fflush(stdout);
    }
  }

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
// interactive shell to process commands
// ============================================================================
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

// ============================================================================
//This is a test funtion created to run some of the functions we have implemented
// ============================================================================
int runTests()
{
  printf("*** Running basic tests ***\n");
  char lines[9][MAXLINE] = {"ls", "ls -al", "ls & whoami ;",
                            "!!", "ls > junk.txt", "cat < junk.txt",
                            "ls | wc", "ascii",
                            "ps auxf | cat | tac | cat | tac | grep `whoami`"};
  for (int i = 0; i < 9; i++)
  {
    printf("* %d. Testing %s *\n", i + 1, lines[i]);
    processLine(lines[i]);
  }

  return 0;
}

// ============================================================================
// return true if C-strings are equal
// ============================================================================
bool equal(char *a, char *b)
{
  return a != NULL && b != NULL && (strcmp(a, b) == 0);
}

// ============================================================================
// read a line from console
// return length of line read or -1 if failed to read
// removes the \n on the line read
// ============================================================================
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

//=============================================================================
// A method that prints out a rabbit to the terminal
//=============================================================================
void asciiArt()
{
  printf("         ,\n");
  printf("        /|      __\n");
  printf("       / |   ,-~ /\n");
  printf("      Y :|  //  /\n");
  printf("      | jj /( .^\n");
  printf("      >-\"~\"-v\"\n");
  printf("     /       Y\n");
  printf("    jo  o    |\n");
  printf("   ( ~T~     j\n");
  printf("    >._-' _./\n");
  printf("   /   \"~\"  |\n");
  printf("  Y     _,  |\n");
  printf(" /| ;-\"~ _  l\n");
  printf("/ l/ ,-\"~    \\\n");
  printf("\\//\\/      .- \\\n");
  printf(" Y        /    Y    -Row\n");
  printf(" l       I     !\\\n");
  printf(" ]\\      _\\    /\"\\\n");
  printf("(\" ~----( ~   Y.  )\n");
  execlp("sleep", "sleep", "0");
}
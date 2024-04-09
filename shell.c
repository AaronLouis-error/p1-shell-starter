#include "shell.h"

char **arguments;

// ============================================================================
// Execute a child process.
// Returns -1
// on failure.  On success, does not return to caller.
// ============================================================================
int child(char **args) {
  printf("here\n");
  fflush(stdout);
  int i = 0;
  while (args[i] != NULL) {
    if (equal(args[i], ">")) {
      int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);

      if (fd == -1) {
        perror("Error opening file");
        return 1;
      }

      // Redirect stdout to the file
      if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("Error redirecting stdout");
        close(fd);
        return 1;
      }

      args[i] = NULL;
    } else if (equal(args[i], "<")) {
      int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);

      if (fd == -1) {
        perror("Error opening file");
        return 1;
      }

      // Redirect stdout to the file
      if (dup2(fd, STDIN_FILENO) == -1) {
        perror("Error redirecting stdin");
        close(fd);
        return 1;
      }

      args[i] = NULL;
    } else if (equal(args[i], "|")) {
      // do pipe in a separate function
      doPipe(args, i);
    } else {
      ++i;
    }
  }
  printf("here2");
  fflush(stdout);

  // call execvp on prepared arguments after while loop ends. You can modify
  // arguments as you loop over the arguments above.
  printf(args[0]);
  fflush(stdout);
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
void doCommand(char **args, int start, int end, bool waitfor) {
  // you will have your classic fork() like implementation here.
  // always execute your commands in child. so pass in arguments there
  // based on waitfor flag, in parent implement wait or not wait  based on & or
  // ;
  int subarraySize = end - start + 1;

  // Allocate memory for the subarray
  int *subargs = (int *)malloc(subarraySize * sizeof(int));
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
    perror("execvp");
    exit(EXIT_FAILURE);
  }
  // Parent
  if (waitfor) {
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
int doPipe(char **args, int pipei) {
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

    close(pipefd[READ]);
    execvp(args[pipei + 1], &args[pipei + 1]);

    perror("Child Piping execvp"); // if execvp returns then error occurred
    exit(EXIT_FAILURE);
  } else { // Parent process
    close(pipefd[READ]);

    if (dup2(pipefd[WRITE], STDOUT_FILENO) == -1) {
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
bool parse(char **args, int start, int *end) {
  int i = start;
  // printf("4.1\n");
  while (args[i] != NULL && !equal(args[i], ";") && !equal(args[i], "&") &&
         !equal(args[i], "\0") && i < MAX_ARGS) {
    // printf("4.2\n");
    // printf("i: %i\n", i);
    // printf(args[i]);
    fflush(stdout);
    i++;
    // printf("4.21\n");
    // printf("args[i]: %s", args[i]);
    fflush(stdout);
    if (args[i] == NULL) {
      // printf("Is null");
      fflush(stdout);
    } else {

      // printf("Is not null");
      fflush(stdout);
    }
    // printf("\n4.22\n");
    fflush(stdout);
  }
  // printf("4.28\n");
  fflush(stdout);
  *end = i - 1;
  // printf("4.3\n");
  fflush(stdout);
  if (equal(args[i], "&"))
    return false;
  // printf("4.4\n");
  return true;
}

// ============================================================================
// Tokenize 'line'.  Recall that 'strtok' writes NULLs into its string
// argument to delimit the end of each sub-string.
// Eg: line = "ls -a --color" would produce tokens: "ls", "-a" and "--color"
// ============================================================================
char **tokenize(char *line) {
  // initalize array with NULL values
  char *token;
  // printf("2.0\n");
  fflush(stdout);
  token = strtok(line, " ");
  // printf("2.1\n");
  fflush(stdout);
  if (token != NULL && equal(token, "!!")) {
    return arguments;
  }
  // printf("2.2\n");
  fflush(stdout);
  for (int i = 0; i < MAX_ARGS; i++) {
    arguments[i] = NULL;
  }
  // printf("2.3\n");
  fflush(stdout);
  arguments[0] = token;
  int i = 1;
  while (token != NULL) {
    // printf("2.4\n");
    fflush(stdout);
    token = strtok(NULL, " ,\0");
    // printf("2.5\n");
    fflush(stdout);
    arguments[i] = token;
    // printf("2.6\n");
    fflush(stdout);
    i++;
  }
  // printf("2.7\n");
  fflush(stdout);
  return arguments;
}

// ============================================================================
// Main loop for our Unix shell interpreter
// ============================================================================
int main() {
  // bool should_run = false; // loop until false
  bool runTestsBool = false;
  if (!runTestsBool) {
    interactiveShell();
  } else {
    runTests();
  }
  // arguments = calloc(MAX_ARGS, sizeof(char *));
  // // printf("0\n");
  // fflush(stdout);
  // char *line = calloc(1, MAXLINE); // holds user input

  // int start = 0; // index into args array
  // // printf("0.5\n");
  // // todo: should we transfer this code to interactive terminal??
  // while (should_run) {
  //   printf(PROMPT);   // osh>
  //   fflush(stdout);   // because no "\n"
  //   fetchline(&line); // fetch NUL-terminated line

  //   // printf("1");
  //   fflush(stdout);
  //   if (equal(line, ""))
  //     continue; // blank line

  //   // printf("2\n");
  //   fflush(stdout);
  //   if (equal(line, "exit")) { // cheerio
  //     should_run = false;
  //     continue;
  //   }
  //   // printf("2.01\n");
  //   fflush(stdout);

  //   if (equal(line, "!!")) {
  //     // don't have to do anything here, handled by tokenize
  //   }
  //   // printf("2.02\n");
  //   fflush(stdout);

  //   // process lines
  //   char **args = tokenize(line); // split string into tokens
  //   // printf("3\n");
  //   fflush(stdout);
  //   // loop over to find chunk of independent commands and execute
  //   while (args[start] != NULL) {
  //     int end;
  //     // // printf("4\n");
  //     fflush(stdout);
  //     bool waitfor = parse(
  //         args, start,
  //         &end); // parse() checks if current command ends with ";" or "&"
  //                // or nothing. if it does not end with anything treat it
  //                // as ; or blocking call. Parse updates "end" to the index
  //                // of the last token before ; or & or simply nothing
  //     // printf("5\n");
  //     fflush(stdout);
  //     doCommand(args, start, end, waitfor); // execute sub-command
  //     start = end + 2;                      // next command
  //   }
  //   start = 0; // next line
  //   // remember current command into history
  // }
  return 0;
}

/*int main(int argc, char **argv) {
  if (argc == 2 && equal(argv[1], "--interactive")) {
    return interactiveShell();
  } else {
    return runTests();
  }
}*/

// interactive shell to process commands
int interactiveShell() {
  printf("interactive Shell");
  bool should_run = true;
  arguments = calloc(MAX_ARGS, sizeof(char *));
  fflush(stdout);
  char *line = calloc(1, MAXLINE);

  int start = 0; // index into args array

  while (should_run) {
    printf(PROMPT); // PROMPT is osh>
    fflush(stdout);
    int n = fetchline(&line);
    printf("read: %s (length = %d)\n", line, n);
    fflush(stdout);

    // ^D results in n == -1
    if (n == -1 || equal(line, "exit")) {
      should_run = false;
      continue;
    }
    fflush(stdout);

    if (equal(line, "")) {
      continue;
    }
    fflush(stdout);

    if (equal(line, "!!")) {
      // don't have to do anything here, handled by tokenize
    }
    fflush(stdout);
    // printf("2.02\n");

    // todo: does this need to match what is in main?
    processLine(line);
  }
  free(line);
  return 0;
}

void processLine(char *line) {
  printf("processing line: %s\n", line);
  if (arguments == NULL) {
    arguments = calloc(MAX_ARGS, sizeof(char *));
  }

  // Tokonize
  // tokenize(line, arguments);

  runProcess(arguments);

  // check if tokenizing is working
  // todo: change forking behavior according to this
  for (int i = 0; i < 3; i++) {
    if (arguments[i] != NULL) {
      char *line = arguments[i];
      printf("\t %s\n", line);
      if (equal(line, "&")) {
        printf("\tasynchronus\n");
      }
      if (equal(line, ";") || equal(line, "|")) {
        printf("\tblocking call\n");
      }
    }
  }
}

/*
void tokenize(char *line, char **arguments) {
  // initalize array with NULL values
  char *token;
  token = strtok(line, " \n");
  if (token != NULL && equal(token, "!!")) {
    return;
  }
  for (int i = 0; i < MAX_ARGS; i++) {
    arguments[i] = NULL;
  }
  arguments[0] = token;
  int i = 1;
  while (token != NULL) {
    token = strtok(NULL, " ,\0\n");
    if (token != NULL &&
        (equal(token, "&") || equal(token, ";") || equal(token, "|"))) {
      if (arguments[2] == NULL) {
        i = 2;
      } else {
        continue;
      }
    }
    arguments[i] = token;
    i++;
  }
}*/

void runProcess(char **arguments) {
  enum { READ, WRITE };
  pid_t pid;
  int pipeFD[2];

  if (pipe(pipeFD) < 0) {
    perror("Error in creating pipe");
    exit(EXIT_FAILURE);
  }

  // printf("pipe[read] %d\n", pipeFD[READ]);
  // printf("pipe[write] %d\n", pipeFD[WRITE]);

  pid = fork();
  if (pid < 0) {
    perror("Error during fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) // Child
  {

    // close(pipeFD[READ]);
    // dup2(pipeFD[WRITE], 1); // stdout is now child's read pipe
    if (arguments[2] != NULL) { // double fork!
      pid = fork();
      if (pid < 0) {
        perror("Error during fork");
        exit(EXIT_FAILURE);
      }
      if (pid == 0) { // Child of Child
        // close(pipeFD[WRITE]);
        // dup2(pipeFD[READ], 0); // stdin is now child's write pipe
        close(pipeFD[READ]);
        if (equal(arguments[2], "|")) {
          // 1= stdout, anything going to stdout goes to pipeFD[Write]
          dup2(pipeFD[WRITE], 1);
          // stdout is now child's write pipe
          // the results of execlp is sent to stdout
        }
        execlp(arguments[0], arguments[0], arguments[1], NULL);
        return;
      }
      // if bool(!oneProcess){
      if (equal(arguments[2], ";") || equal(arguments[2], "|")) {
        wait(NULL);
        if (equal(arguments[2], "|")) {
          // close(pipeFD[WRITE]);
          char newArgsChars[BUF_SIZE];
          read(pipeFD[READ], newArgsChars, BUF_SIZE);
          char *newArgs[BUF_SIZE];
          // tokenize(newArgsChars, newArgs);
          printf("new args: %s\n", newArgs[0]);
          // todo: have tokenize method that works for input that is
          // delimited by \n
          execvp(arguments[3], *"a.out"); //, newArgsChars, NULL);
        } else {
          execlp(arguments[3], arguments[3], arguments[4], NULL);
        }
      } else {
        execlp(arguments[3], arguments[3], arguments[4], NULL);
        wait(NULL);
      }
    } else {
      execlp(arguments[0], arguments[0], arguments[1], NULL);
    }
    // process is overlayed so does not execut past here...
    //}
    // else{
    // fork again
    return;
  }
  // Parent
  wait(NULL);
  char buf[BUF_SIZE];
  close(pipeFD[WRITE]);
  int n = read(pipeFD[READ], buf, BUF_SIZE);
  buf[n] = '\0';
  for (int i = 0; i < n; ++i)
    printf("%c", buf[i]);
  // cout << buf;
}

int runTests() {
  printf("*** Running basic tests ***\n");
  char lines[7][MAXLINE] = {"ls",     "ls -al",        "ls & whoami ;",
                            "!!",     "ls > junk.txt", "cat < junk.txt",
                            "ls | wc"};
  for (int i = 0; i < 7; i++) {
    printf("* %d. Testing %s *\n", i + 1, lines[i]);
    processLine(lines[i]);
  }

  return 0;
}

// return true if C-strings are equal
bool equal(char *a, char *b) {
  return a != NULL && b != NULL && (strcmp(a, b) == 0);
}

// read a line from console
// return length of line read or -1 if failed to read
// removes the \n on the line read
int fetchline(char **line) {
  size_t len = 0;
  size_t n = getline(line, &len, stdin);
  if (n > 0) {
    (*line)[n - 1] = '\0';
  }
  return n;
}

int ls() { return 0; }

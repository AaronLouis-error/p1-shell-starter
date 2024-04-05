#include "shell.h"

char **arguments;

int main(int argc, char **argv) {
  if (argc == 2 && equal(argv[1], "--interactive")) {
    return interactiveShell();
  } else {
    return runTests();
  }
}

// interactive shell to process commands
int interactiveShell() {
  printf("interactive Shell");
  bool should_run = true;
  char *line = calloc(1, MAXLINE);
  while (should_run) {
    printf(PROMPT);
    fflush(stdout);
    int n = fetchline(&line);
    printf("read: %s (length = %d)\n", line, n);
    // ^D results in n == -1
    if (n == -1 || equal(line, "exit")) {
      should_run = false;
      continue;
    }
    if (equal(line, "")) {
      continue;
    }
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
  tokenize(line, arguments);

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

void tokenize(char *line, char **arguments) {
  // initalize array with NULL values
  char *token;
  token = strtok(line, " ");
  if (token != NULL && equal(token, "!!")) {
    return;
  }
  for (int i = 0; i < MAX_ARGS; i++) {
    arguments[i] = NULL;
  }
  arguments[0] = token;
  int i = 1;
  while (token != NULL) {
    token = strtok(NULL, " ,\0");
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
}

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
        printf("t1\n");
        if (equal(arguments[2], "|")) {
          // 1= stdout, anything going to stdout goes to pipeFD[Write]
          dup2(pipeFD[WRITE], 1);
          // stdout is now child's write pipe
          // the results of execlp is sent to stdout
        }
        printf("t2\n");
        execlp(arguments[0], arguments[0], arguments[1], NULL);
        return;
      }
      // if bool(!oneProcess){
      if (equal(arguments[2], ";") || equal(arguments[2], "|")) {
        wait(NULL);
        printf("Test\n");
        if (equal(arguments[2], "|")) {
          close(pipeFD[WRITE]);
          char newArgsChars[BUF_SIZE];
          read(pipeFD[READ], newArgsChars, BUF_SIZE);
          char *newArgs[BUF_SIZE];
          tokenize(newArgsChars, newArgs);
          execlp(arguments[3], newArgs, NULL);
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

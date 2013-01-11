#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern int errno;

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: realpath <file>\n");
    exit(1);
  }

  char *file = *++argv;
  char result[255];

  if ((realpath(file, result)) != NULL) {
    printf("%s\n", result);
    return 0;
  } else {
    printf("%s\n", strerror(errno));
    exit(1);
  }
}

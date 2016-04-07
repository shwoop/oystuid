#include <linux/limits.h>
#include <errno.h>
#include <error.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char **argv) {
  bool found_path = false;
  char script_path[PATH_MAX],
       *env_path,
       *path;
  int status;
  pid_t pid;
  struct stat st;

  if (geteuid() != 0)
    error(1, errno, "must be run as root\r\n");

  if (argc < 2)
    error(1, errno, "requires script as argument\r\n");

  env_path = getenv("PATH");
  if (!env_path)
    error(1, errno, "PATH env not set\r\n");

  path = strtok(env_path, ":");
  while (path) {
    snprintf(script_path, sizeof(script_path), "%s/%s", path, argv[1]);
    if (access(script_path, F_OK) == 0) {
      found_path = true;
      break;
    }
    path = strtok(NULL, ":");
  }

  if (!found_path)
    error(1, errno, "unable to find script in PATH\r\n");

  if (access(script_path, X_OK) < 0)
    error(1, errno, "script should have execute permission\r\n");

  if (stat(script_path, &st) < 0)
    error(1, errno, "unable to get stat of script\r\n");

  if (!(st.st_mode & S_ISUID))
    error(1, errno, "setuid not set on provided script\r\n");

  if (chmod(script_path, S_IRWXU) != 0)
    error(1, errno, "unable to remove setuid bit\r\n");

  if ((pid = fork()) < 0)
     error(1, errno, "unable to fork\r\n");

  if (pid == 0) {
    if ((st.st_uid != 0) && (setuid(st.st_uid) != 0))
        error(1, errno, "unable to set uid to %d\r\n", st.st_uid);
    execvp(script_path, &argv[1]);
  } else {
    wait(&status);
    if (chmod(script_path, st.st_mode) != 0)
      error(1, errno, "unable to restore setuid bit\r\n");
    return status;
  }
}

#include <string.h>

#include "gui_main.h"
#include "compiler_main.h"

char	*argv0 = NULL;

int main(int argc, char **argv)
{
  argv0 = strdup(argv[0]);

  if (argc < 2) {
    return gui_main(argc, argv);
  } else {
    return compiler_main(argc, argv);
  }
  
  return 0;
}

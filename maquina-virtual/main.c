/*
 * Simple Pascal Compiler
 * Virtual Machine (Main Program)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include "ui.h"
#include "vm.h"

int main(int argc, char **argv)
{
  UI *ui;
  
  gtk_init(&argc, &argv);

  if (argc < 2) {
    ui = ui_new(NULL);
  } else {
    ui = ui_new(argv[1]);
  }
  
  gtk_main();
  ui_destroy(ui);
  
  return 0;
}

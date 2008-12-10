/*
 * Simple Pascal Compiler
 * GUI (Main Program)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include "conf.h"
#include "ui.h"

int gui_main(int argc, char **argv)
{
  UI *ui;
    
  gtk_init(&argc, &argv);
  conf_init("csd");
  
  ui = ui_new();
  gtk_main();
  ui_destroy(ui);
  
  return 0;
}

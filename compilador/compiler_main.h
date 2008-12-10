#ifndef __COMPILER_MAIN_H__
#define __COMPILER_MAIN_H__

#include <glib.h>

typedef struct _CompilerParams	CompilerParams;
struct _CompilerParams {
	gboolean test_parser,
		 test_ast,
		 test_st,
		 show_time,
		 viagem_do_freitas;
	gint	 optimization_level;
	gchar	*input_file,
		*output_format;
};

int	compiler_main(int argc, char **argv);

extern 	char		*argv0;
extern	CompilerParams	 params;

#endif	/* __COMPILER_MAIN_H__ */



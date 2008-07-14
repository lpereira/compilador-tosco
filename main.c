#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/time.h>
#include <time.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"
#include "codegen.h"
#include "symbol-table.h"
#include "stack.h"

typedef struct _Params	Params;
struct _Params {
	gboolean test_parser,
		 test_ast,
		 test_codegen,
		 show_time;
	gint	 optimization_level;
	gchar	*input_file,
		*output_file,
		*output_format;
};

static Params params;
static GOptionEntry cmdline_options[] = {
	{
		.long_name = "test-parser",
		.short_name = 'P',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_parser,
		.description = "Tests the Parser"
	},
	{
		.long_name = "test-ast",
		.short_name = 'A',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_ast,
		.description = "Tests the AST"
	},
	{
		.long_name = "test-codegen",
		.short_name = 'C',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_codegen,
		.description = "Tests the Code Generation (TAC)"
	},
	{
		.long_name = "optimization-level",
		.short_name = 'O',
		.arg = G_OPTION_ARG_INT,
		.arg_data = &params.optimization_level,
		.description = "Sets the Optimization level (0-3)"
	},
	{
		.long_name = "input-file",
		.short_name = 'i',
		.arg = G_OPTION_ARG_STRING,
		.arg_data = &params.input_file,
		.description = "Sets the input file"
	},
	{
		.long_name = "output-file",
		.short_name = 'o',
		.arg = G_OPTION_ARG_STRING,
		.arg_data = &params.output_file,
		.description = "Sets the ouput file"
	},
	{
		.long_name = "output-format",
		.short_name = 'f',
		.arg = G_OPTION_ARG_STRING,
		.arg_data = &params.output_format,
		.description = "Specify the output format (risclie/llvm)"
	},
	{
		.long_name = "show-time",
		.short_name = 't',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.show_time,
		.description = "Show time taken by all steps"
	},
	{ NULL }
};

static void
show_usage(gchar *program_name)
{
	g_print("Usage: %s -i input-file.pas\n\n"
	        "See ``%s --help'' for more.\n\n", program_name, program_name);
}

int
main(int argc, char **argv)
{
	GNode          *root;
	TokenList      *token_list;
	struct timeval	tv_start, tv_lex, tv_ast, tv_codegen;
	gdouble		time_lex, time_ast, time_codegen, time_total;
	gdouble		p_lex, p_ast, p_codegen, p_total;
	GOptionContext *ctx;
	
	ctx = g_option_context_new("- Toy Pascal-ish Compiler");
	g_option_context_set_help_enabled(ctx, TRUE);
	g_option_context_add_main_entries(ctx, cmdline_options, NULL);
	g_option_context_parse(ctx, &argc, &argv, NULL);
	g_option_context_free(ctx);
	
	if (params.test_parser) {
		return lex_test_main(argc, argv);
	}
	
	if (params.test_ast) {
		return ast_test_main(argc, argv);
	}
	
	if (params.test_codegen) {
		return codegen_test_main(argc, argv);
	}

	if (!params.input_file) {
		show_usage(argv[0]);
		return 0;
	}

	gettimeofday(&tv_start, NULL);
	
	token_list = lex();
	gettimeofday(&tv_lex, NULL);

	root = ast(token_list);
	gettimeofday(&tv_ast, NULL);
	
	if (params.optimization_level & 1) {
		/* do the first-level optimization thing */
	}

	codegen(root);
	gettimeofday(&tv_codegen, NULL);
	
	if (params.optimization_level & 2) {
		/* do the second-level optimization thing */
	}

	tl_destroy(token_list);

	
	if (params.show_time) {
#define CALCTIME(start,end)  \
	((end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1e6))
		time_lex = CALCTIME(tv_start, tv_lex);
		time_ast = CALCTIME(tv_lex, tv_ast);
		time_codegen = CALCTIME(tv_ast, tv_codegen);
#undef CALCTIME

		time_total = time_lex + time_ast + time_codegen;

		p_lex = (100.0f * time_lex) / time_total;
		p_ast = (100.0f * time_ast) / time_total;
		p_codegen = (100.0f * time_codegen) / time_total;
		
		p_total = p_lex + p_ast + p_codegen;
		
		printf("\n--------------------------------------------------\n");
		printf("     Time to Lex & Parse: %fs (%f%%)\n", time_lex, p_lex);
		printf("      Time to create AST: %fs (%f%%)\n", time_ast, p_ast);
		printf("   Time to generate code: %fs (%f%%)\n", time_codegen, p_codegen);
		printf("                   Total: %fs (%f%%)\n", time_total, p_total);
		printf("--------------------------------------------------\n");
	}
		
	return 0;
}

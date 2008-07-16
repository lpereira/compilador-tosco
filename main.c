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

#include "optimization-l1.h"

#define CALCTIME(start,end) 	((end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1e6))
#define CALCPERC(t)		(100.0f * t) / time_total

typedef struct _Params	Params;
struct _Params {
	gboolean test_parser,
		 test_ast,
		 show_time;
	gint	 optimization_level;
	gchar	*input_file,
		*output_file,
		*output_format;
};

static Params params;
static GOptionEntry cmdline_options[] = {
	{
		.long_name = "pretty-print",
		.short_name = 'P',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_parser,
		.description = "Pretty-print the input file (requires ANSI terminal)"
	},
	{
		.long_name = "show-ast",
		.short_name = 'A',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_ast,
		.description = "Outputs a DOT-file with the AST"
	},
	{
		.long_name = "optimization-level",
		.short_name = 'O',
		.arg = G_OPTION_ARG_INT,
		.arg_data = &params.optimization_level,
		.description = "Sets the Optimization level (0-3)"
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

int
main(int argc, char **argv)
{
	GNode          *root;
	TokenList      *token_list;
	struct timeval	tv_start, tv_lex, tv_ast, tv_codegen, tv_opt1, tv_opt2;
	gdouble		time_lex, time_ast, time_codegen, time_total, time_opt1, time_opt2;
	gdouble		p_lex, p_ast, p_codegen, p_total, p_opt1, p_opt2;
	GOptionContext *ctx;
	FILE	       *input_file;
	
	ctx = g_option_context_new("input-file.pas ...");
	g_option_context_set_help_enabled(ctx, TRUE);
	g_option_context_add_main_entries(ctx, cmdline_options, NULL);
	g_option_context_parse(ctx, &argc, &argv, NULL);
	g_option_context_free(ctx);
	
	if (argv[1]) {
		params.input_file = argv[1];
		
		if (!(input_file = fopen(params.input_file, "r"))) {
			g_print("%s: can't open input file ``%s''\n", argv[0], params.input_file);
			return 1;
		} else {
			char_buf_set_file(input_file);
		}
	} else {
		g_print("%s: no input file\n", argv[0]);
		return 1;
	}
	
	if (params.test_parser) {
		return lex_test_main(argc, argv);
	}
	
	if (params.test_ast) {
		return ast_test_main(argc, argv);
	}
	
	gettimeofday(&tv_start, NULL);
	
	token_list = lex();
	gettimeofday(&tv_lex, NULL);

	root = ast(token_list);
	gettimeofday(&tv_ast, NULL);
	
	if (params.optimization_level & 1) {
		optimization_l1(root);
		gettimeofday(&tv_opt1, NULL);
	}

	codegen(root);
	gettimeofday(&tv_codegen, NULL);
	
	if (params.optimization_level & 2) {
		/* do the second-level optimization thing */
		gettimeofday(&tv_opt2, NULL);
	}

	tl_destroy(token_list);
	fclose(input_file);
	
	if (params.show_time) {
		time_lex = CALCTIME(tv_start, tv_lex);
		time_ast = CALCTIME(tv_lex, tv_ast);
		
		if (params.optimization_level & 1) {
			time_opt1 = CALCTIME(tv_ast, tv_opt1);
			time_codegen = CALCTIME(tv_opt1, tv_codegen);
		} else {
			time_opt1 = 0.0f;
			time_codegen = CALCTIME(tv_ast, tv_codegen);
		}
		
		if (params.optimization_level & 2) {
			time_opt2 = CALCTIME(tv_codegen, tv_opt2);
		} else {
			time_opt2 = 0.0f;
		}

		time_total = time_lex + time_ast + time_codegen + time_opt1 + time_opt2;

		p_lex = CALCPERC(time_lex);
		p_ast = CALCPERC(time_ast);
		p_codegen = CALCPERC(time_codegen);

		p_total = p_lex + p_ast + p_codegen;
		
		if (params.optimization_level & 1) {
			p_opt1 = CALCPERC(time_opt1);
			p_total += p_opt1;
		}
		
		if (params.optimization_level & 2) {
			p_opt2 = CALCPERC(time_opt2);
			p_total += p_opt2;
		}		
		
		printf("\n----- Step ----------- Time ----- Percentage -----\n");
		printf("     Lex & Parse    %fs    %0f%%\n", time_lex, p_lex);
		
		if (params.optimization_level & 1)
			printf("Optimization (1)    %fs    %0f%%\n", time_opt1, p_opt1);

		printf("      Create AST    %fs    %f%%\n", time_ast, p_ast);
	
		if (params.optimization_level & 2)
			printf("Optimization (2)    %fs    %f%%\n", time_opt2, p_opt2);
	
		printf("   Generate code    %fs    %f%%\n", time_codegen, p_codegen);
		printf("           Total    %fs    %f%%\n", time_total, p_total);
		printf("--------------------------------------------------\n");
	}
		
	return 0;
}

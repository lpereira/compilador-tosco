/*
 * Simple Pascal Compiler
 * Main program
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

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

#include "optimization.h"

#include "compiler_main.h"

#define CALCTIME(start,end) 	((end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1e6))
#define CALCPERC(t)		(t * 100.0f) / time_total

CompilerParams params;
static GOptionEntry cmdline_options[] = {
	{
		.long_name = "pretty-print",
		.short_name = 'P',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_parser,
		.description = "Pretty-print the input file (requires ANSI term.)"
	},
	{
		.long_name = "generate-dot-ast",
		.short_name = 'A',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_ast,
		.description = "Outputs a DOT-file with the AST"
	},
	{
		.long_name = "print-symbol-table",
		.short_name = 'S',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.test_st,
		.description = "Prints the symbol table"
	},
	{
		.long_name = "optimization-level",
		.short_name = 'O',
		.arg = G_OPTION_ARG_INT,
		.arg_data = &params.optimization_level,
		.description = "Sets the Optimization level (0-3)"
	},
	{
		.long_name = "show-time",
		.short_name = 't',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.show_time,
		.description = "Show time taken by all steps"
	},
	{
		.long_name = "viagem-do-freitas",
		.short_name = 'v',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = &params.viagem_do_freitas,
		.description = "Habilita as viagens do Freitas"
	},
	{ NULL }
};

static int compiler_do(void)
{
	GNode          *root;
	TokenList      *token_list;
	struct timeval	tv_start, tv_lex, tv_ast, tv_codegen, tv_opt;
	gdouble		time_lex, time_ast, time_codegen, time_total, time_opt;
	gdouble		p_lex, p_ast, p_codegen, p_total, p_opt = 0.0;
	
	gettimeofday(&tv_start, NULL);
	
	token_list = lex();
	gettimeofday(&tv_lex, NULL);

	root = ast(token_list);
	gettimeofday(&tv_ast, NULL);
	
	if (params.optimization_level & 1) {
		optimize(root);
		gettimeofday(&tv_opt, NULL);
	}

	codegen(root);
	gettimeofday(&tv_codegen, NULL);
	
	tl_destroy(token_list);
	
	if (params.show_time) {
		time_lex = CALCTIME(tv_start, tv_lex);
		time_ast = CALCTIME(tv_lex, tv_ast);
		
		if (params.optimization_level & 1) {
			time_opt = CALCTIME(tv_ast, tv_opt);
			time_codegen = CALCTIME(tv_opt, tv_codegen);
		} else {
			time_opt = 0.0f;
			time_codegen = CALCTIME(tv_ast, tv_codegen);
		}
		
		time_total = time_lex + time_ast + time_codegen + time_opt;

		p_lex = CALCPERC(time_lex);
		p_ast = CALCPERC(time_ast);
		p_codegen = CALCPERC(time_codegen);

		p_total = p_lex + p_ast + p_codegen;
		
		if (params.optimization_level & 1) {
			p_opt = CALCPERC(time_opt);
			p_total += p_opt;
		}
		
		fprintf(stderr, "Análise Léxica e Sintática|%fs|%0f\n", time_lex, p_lex);
		fprintf(stderr, "Análise Semântica|%fs|%f\n", time_ast, p_ast);

		if (params.optimization_level & 1)
			fprintf(stderr, "Otimização|%fs|%0f\n", time_opt, p_opt);
	
		fprintf(stderr, "Geração de Código|%fs|%f\n", time_codegen, p_codegen);

		fprintf(stderr, "Total|%fs|%f\n", time_total, p_total);
	}
		
	return 0;
}

int
compiler_compile_with_parameters(CompilerParams	*p)
{
	FILE *input_file;
	
	if (p->input_file) {
		if (g_str_equal(p->input_file, "-")) {
			char_buf_set_file(stdin);
		} else if (!(input_file = fopen(p->input_file, "r"))) {
			g_print("can't open input file ``%s''\n", params.input_file);
			return 1;
		} else {
			char_buf_set_file(input_file);
		}
	} else {
		g_print("no input file\n");
		return 1;
	}

	params = *p;
	return compiler_do();
}

int
compiler_main(int argc, char **argv)
{
	GOptionContext *ctx;
	FILE	       *input_file;
	
	ctx = g_option_context_new("input-file.lpd ...");
	g_option_context_set_help_enabled(ctx, TRUE);
	g_option_context_add_main_entries(ctx, cmdline_options, NULL);
	g_option_context_parse(ctx, &argc, &argv, NULL);
	g_option_context_free(ctx);
	
	if (argv[1]) {
		params.input_file = argv[1];
		
		if (g_str_equal(params.input_file, "-")) {
			char_buf_set_file(stdin);
		} else if (!(input_file = fopen(params.input_file, "r"))) {
			g_print("can't open input file ``%s''\n", params.input_file);
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

	if (params.test_st) {
		return symbol_table_test_main(argc, argv);
	}
	
	return compiler_do();
}

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

int
main(int argc, char **argv)
{
	GNode          *root;
	TokenList      *token_list;
	struct timeval	tv_start, tv_lex, tv_ast, tv_codegen;
	gdouble		time_lex, time_ast, time_codegen, time_total;
	gdouble		p_lex, p_ast, p_codegen, p_total;
	
	if (argc >= 2) {
		fclose(stdin);
		stdin = fopen(argv[1], "r");		
	}
	
	gettimeofday(&tv_start, NULL);
	
	token_list = lex();
	gettimeofday(&tv_lex, NULL);

	root = ast(token_list);
	gettimeofday(&tv_ast, NULL);

	codegen(root);
	gettimeofday(&tv_codegen, NULL);

	tl_destroy(token_list);

#define CALCTIME(start,end)  \
	((end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1e6))
	
	time_lex = CALCTIME(tv_start, tv_lex);
	time_ast = CALCTIME(tv_lex, tv_ast);
	time_codegen = CALCTIME(tv_ast, tv_codegen);

	time_total = time_lex + time_ast + time_codegen;

	p_lex = (100.0f * time_lex) / time_total;
	p_ast = (100.0f * time_ast) / time_total;
	p_codegen = (100.0f * time_codegen) / time_total;
	
	p_total = p_lex + p_ast + p_codegen;
#undef CALCTIME
	
	printf("\n--------------------------------------------------\n");
	printf("     Time to Lex & Parse: %fs (%f%%)\n", time_lex, p_lex);
	printf("      Time to create AST: %fs (%f%%)\n", time_ast, p_ast);
	printf("   Time to generate code: %fs (%f%%)\n", time_codegen, p_codegen);
	printf("                   Total: %fs (%f%%)\n", time_total, p_total);
	printf("--------------------------------------------------\n");
	
	return 0;
}
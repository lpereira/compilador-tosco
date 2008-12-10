/*
 * Simple Pascal Compiler
 * Lexical and Syntatical Analysis
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"

#ifndef LPD
const char     *literals[] = {
	"(nil)", ",", "then", ":=", "boolean", ")",
	":", "/", "do", "else", "false", "end",
	"function", "if", "begin", "integer", "-",
	"*", "not", "<>", "(", "=", ">",
	">=", "<", "<=", "or", ".", "+",
	"procedure", "program", "read", ";", "and",
	"true", "var", "while", "write",
	"id", "number",
	"return", "function call", "procedure call", "-",
	"for", "step", "+", "begin"
};
#else
const char     *literals[] = {
	"(nil)", ",", "entao", ":=", "booleano", ")",
	":", "div", "faca", "senao", "falso", "fim",
	"funcao", "se", "inicio", "inteiro", "-",
	"*", "nao", "<>", "(", "=", ">",
	">=", "<", "<=", "ou", ".", "+",
	"procedimento", "programa", "leia", ";", "e",
	"verdadeiro", "var", "enquanto", "escreva",
	"id", "numero",
	"return", "cham. funcao", "cham. procedimento", "-",
	"para", "passo", "+", "inicio principal"
};
#endif

static int      line = 1, column = 1;

static TokenList *match_program(void);
static TokenList *match_block(void);
static TokenList *match_variable_declare_step(void);
static TokenList *match_variable_declare(void);
static TokenList *match_type(void);
static TokenList *match_subroutine_declare_step(void);
static TokenList *match_procedure_declare(void);
static TokenList *match_function_declare(void);
static TokenList *match_statements(void);
static TokenList *match_statement(void);
static TokenList *match_attrib_call(void);
static TokenList *match_attrib(void);
static TokenList *match_procedure_call(void);
static TokenList *match_conditional(void);
static TokenList *match_while(void);
static TokenList *match_read(void);
static TokenList *match_write(void);
static TokenList *match_for(void);
static TokenList *match_expression(void);
static TokenList *match_relational_op(void);
static TokenList *match_simple_expression(void);
static TokenList *match_term(void);
static TokenList *match_factor(void);
static TokenList *match_variable(void);
static TokenList *match_function_call(void);
static TokenList *match_identifier(void);
static TokenList *match_number(void);

static void     lex_error(char *message, ...);

static void
lex_error(char *message, ...)
{
	gchar *buffer;
	va_list args;
	
	va_start(args, message);
	buffer = g_strdup_vprintf(message, args);
	va_end(args);
	
	printf("Erro: ln <b>%d</b>, col <b>%d</b> \342\206\222 %s\n", line, column, buffer);
	
	g_free(buffer);
	exit(1);
}

static int
get_character(void)
{
	int ch = char_buf_get();

	if (ch == '\n') {
		line++;
		column = 1;
	} else {
		column++;
	}
	
	return ch;
}

static void
unget_character(char ch)
{
	if (ch == -1)
		return;

	if(--column <= 0)
		column = 1;

	if(ch == '\n')
		line--;		/*
		             * FIXME: still needs to calculate the column correctly;
					 *		  it's not column=1, since it will have a certain
					 *		  length. anyway, since tokens cannot currently
					 *		  span multiple lines, this won't cause major
					 *		  problems, I guess :)
					 */

	char_buf_put_char(ch);
}

static void
unget_string(char *string)
{
	char_buf_put_string(string);
	
	for (; *string; string++) {
		if (--column <= 0)
			column = 1;
		
		if (*string == '\n')
			line--;
	}
}

static int
is_desired_char(int ch, gpointer user_data)
{
	return ch == GPOINTER_TO_INT(user_data);
}

static int
is_alpha(int ch, gpointer user_data)
{
	return isalpha(ch);
}

static int
is_digit(int ch, gpointer user_data)
{
	return isdigit(ch);
}

static int
eat_whitespace_until(int (*condition_func)(int, gpointer), gpointer user_data)
{
	int ch;
	gboolean last_was_space = FALSE;

	ch = get_character();

	while (1) {
		if (ch == '{') {		/* eat comments */
			last_was_space = FALSE;
			
			while (1) {
				ch = get_character();
				if (ch == '}')
					break;
				else if (ch == EOF)
					lex_error("esperando: <u>}</u>");
			}

			ch = get_character();
		} else if (ch == '/') {		/* eat line comments */
			if ((ch = get_character()) == '/') {
				while (1) {
					ch = get_character();
					
					if (ch == '\n')
						break;
					else if (ch == EOF)
						lex_error("esperando: fim de linha");
				}
			} else {
				unget_character(ch);
				ch = '/';
				goto try_condition;
			}
		} else if (isspace(ch)) {	/* eat spaces */
			last_was_space = TRUE;
			
			ch = get_character();
		} else {			/* might be what we're looking for */
try_condition:
			if (condition_func && !condition_func(ch, user_data)) {
				unget_character(ch);
				
				if (last_was_space)
					unget_character(' ');
				return -1;
			}
			
			return ch;
		}
	}
}

static void
unget_tokenlist(TokenList * tl)
{
	GList          *token;

	for (token = tl->tokens; token; token = token->next) {
		Token          *t = (Token *) token->data;
		
		unget_string(t->id);
	}
}

static TokenList      *
match_token(TokenType token_type)
{
	Token          *token;
	TokenList      *token_list;
	char            buffer[128], ch;
	int             index = 0;
	const char     *literal = literals[token_type];
	
	ch = eat_whitespace_until(is_desired_char,
				  GINT_TO_POINTER((gint)*literal));
	if (ch == -1)
		return NULL;
	

	buffer[index++] = ch;
	literal++;

	while (*literal) {
		ch = get_character();
		
		if (ch == *literal) {
			buffer[index++] = ch;
			literal++;
		} else {
			unget_character(ch);
			break;
		}
	}
	
	/*
	 * FIXME
	 *
	 * Isso aqui tenta arrumar o problema de ambiguidade quando temos duas palavras
	 * no vocabulario comecadas pela mesma cadeia (como eh o caso das palavras "e" e
	 * "entao", que comecam pela cadeia "e"). A ideia eh simples: se o literal comeca
	 * com uma letra, assumimos que ele tambem termina com uma letra. Por conta disso,
	 * depois de procurar todos os caracteres do literal, lemos mais um -- se este
	 * for uma letra, quer dizer que na verdade pegamos o literal errado.
	 *
	 * Nao eh uma solucao elegante, embora funcione.
	 */
	if (isalpha(literals[token_type][0])) {
		unget_character(ch = get_character());

		if (isalpha(ch)) {
			buffer[index] = '\0';
			unget_string(buffer);
			
			return NULL;
		}
	}
	
	if (*literal == '\0') {
		token = g_new0(Token, 1);
		token->type = token_type;
		token->id = (gchar*) literals[token_type];
		token->line = line;
		token->column = column - index - 1;
		
		token_list = g_new0(TokenList, 1);
		token_list->tokens = g_list_append(token_list->tokens, token);
		
		return token_list;		
	}

	buffer[index] = '\0';
	unget_string(buffer);
	
	return NULL;
}

static TokenList      *
match_token_req(TokenType token_type)
{
	TokenList      *token_list;

	if ((token_list = match_token(token_type))) {
		return token_list;
	}
	
	lex_error("esperando <u>%s</u>", (char *) literals[token_type]);
	
	/* make compiler happy */
	return NULL;
}

#define REQ(matcher,err)				\
  static TokenList * matcher##_req(void) {		\
    TokenList *tl = matcher();				\
    if (tl == NULL) {					\
      lex_error("esperando %s", err);			\
    }							\
    return tl;						\
  }

#define SUPPRESS(match_function)			\
  tl_unref(match_function)


/*
 * Since REQ macro expands to a static function, this function might not be used
 * in actual code; thus, the macro call is commented out, so gcc stops whinning.
 */

/* REQ(match_program, "program") */
REQ(match_block, "bloco")
/* REQ(match_variable_declare_step, "variable declare step") */
REQ(match_variable_declare, "declaração de variável")
REQ(match_type, "tipo")
/* REQ(match_subroutine_declare_step, "subroutine declare step") */
/* REQ(match_procedure_declare, "procedure declare") */
/* REQ(match_function_declare, "function declare") */
REQ(match_statements, "bloco de comandos (inicio/fim)")
REQ(match_statement, "comando")
/* REQ(match_attrib_call, "attrib call") */
/* REQ(match_attrib, "attrib") */
/* REQ(match_procedure_call, "procedure call") */
/* REQ(match_conditional, "conditional") */
/* REQ(match_while, "while") */
/* REQ(match_for, "for") */
/* REQ(match_read, "read") */
/* REQ(match_write, "write") */
REQ(match_expression, "expressão")
/* REQ(match_relational_op, "relational op") */
REQ(match_simple_expression, "expressão simples")
REQ(match_term, "termo de expressão")
REQ(match_factor, "fator de expressão")
/*REQ(match_variable, "variable")*/
/* REQ(match_function_call, "function call") */
REQ(match_identifier, "identificador")
/* REQ(match_number, "number") */

/* <programa> ::= programa <identificador> ; <bloco> . */
static TokenList      *
match_program(void)
{
	TokenList      *tl;

	tl = tl_new();

	tl_add_token(&tl, match_token_req(T_PROGRAM));
	tl_add_token(&tl, match_identifier_req());
	SUPPRESS(match_token_req(T_SEMICOLON));
	tl_add_token(&tl, match_block_req());
	SUPPRESS(match_token_req(T_PERIOD));

	return tl;
}

/*
 * <bloco> ::= [<etapa_declaracao_variaveis>] [<etapa_declaracao_subrotinas>]
 * <comandos>
 */
static TokenList      *
match_block(void)
{
	TokenList      *tl, *t;
	static gboolean _first_time = TRUE;
	gboolean first_time = _first_time;
	_first_time = FALSE;

	tl = tl_new();

	if ((t = match_variable_declare_step())) {
		tl_add_token(&tl, t);
	}
	if ((t = match_subroutine_declare_step())) {
		tl_add_token(&tl, t);
	}
	
	t = match_statements_req();
	
	if (first_time) {
		Token		*token;

		token = g_new0(Token, 1);
		token->type = T_MAIN_BEGIN;
		token->id = (char *)literals[T_MAIN_BEGIN];
		token->line = line;
		token->column = column;
		
		t->tokens = g_list_prepend(t->tokens, token);
	}

	tl_add_token(&tl, t);

	return tl;
}

/*
 * <etapa_declaracao_variaveis> ::= var <declaracao_variaveis> ;
 * {<declaracao_variaveis>;}
 */
static TokenList      *
match_variable_declare_step(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_VAR))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		tl_add_token(&tl, match_variable_declare_req());
		SUPPRESS(match_token_req(T_SEMICOLON));

		while ((t = match_variable_declare())) {
			tl_add_token(&tl, t);
			SUPPRESS(match_token_req(T_SEMICOLON));
		}

		return tl;
	} else {
		return NULL;
	}
}

/* <declaracao_variaveis> ::= <identificador> {, <identificador>} : <tipo> */
static TokenList      *
match_variable_declare(void)
{
	TokenList      *tl, *t;

	tl = tl_new();

	if ((t = match_identifier())) {
		tl_add_token(&tl, t);

		while ((t = match_token(T_COMMA))) {
			tl_unref(t);
			tl_add_token(&tl, match_identifier_req());
		}

		SUPPRESS(match_token_req(T_COLON));
		tl_add_token(&tl, match_type_req());

		return tl;
	}
	return NULL;
}

/* <tipo> ::= (inteiro|booleano) */
static TokenList      *
match_type(void)
{
	TokenList      *t;

	if ((t = match_token(T_INTEGER)) ||
	    (t = match_token(T_BOOLEAN))) {
		return t;
	}
	return NULL;
}

/*
 * <etapa_declaracao_subrotinas> ::= (<declaracao_procedimento>; |
 * <declaracao_funcao>;) {<declaracao_procedimento>; | <declaracao_funcao>;}
 */
static TokenList      *
match_subroutine_declare_step(void)
{
	TokenList      *tl, *t;

	if ((t = match_procedure_declare()) || (t = match_function_declare())) {
		tl = tl_new();

		tl_add_token(&tl, t);
		SUPPRESS(match_token_req(T_SEMICOLON));

		while ((t = match_procedure_declare()) ||
		       (t = match_function_declare())) {
			tl_add_token(&tl, t);
			SUPPRESS(match_token_req(T_SEMICOLON));
		}

		return tl;
	}
	return NULL;
}

/* <declaracao_procedimento> ::= procedimento <identificador> ; <bloco> */
static TokenList      *
match_procedure_declare(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_PROCEDURE))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		tl_add_token(&tl, match_identifier_req());
		SUPPRESS(match_token_req(T_SEMICOLON));
		tl_add_token(&tl, match_block_req());
		return tl;
	}
	return NULL;
}

/* <declaracao_funcao> ::= funcao <identificador> : <tipo>; <bloco> */
static TokenList      *
match_function_declare(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_FUNCTION))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		tl_add_token(&tl, match_identifier_req());
		SUPPRESS(match_token_req(T_COLON));
		tl_add_token(&tl, match_type_req());
		SUPPRESS(match_token_req(T_SEMICOLON));
		tl_add_token(&tl, match_block_req());

		return tl;
	}
	return NULL;
}


/* <comandos> ::= inicio <comando> {; <comando>}[;] fim */
static TokenList      *
match_statements(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_BEGIN))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		tl_add_token(&tl, match_statement_req());

		while ((t = match_token(T_SEMICOLON))) {
			tl_add_token(&tl, t);
			tl_add_token(&tl, match_statement());
		}

		tl_add_token(&tl, match_token(T_SEMICOLON));
		tl_add_token(&tl, match_token_req(T_END));
		
		return tl;
	}
	return NULL;
}

/*
 * <comando> ::=
 * (<atribuicao_chprocedimento>|<cmd_condicional>|<cmd_enquanto>|<cmd_leitura>
 * |<cmd_escrita>|<comandos>)
 */
static TokenList      *
match_statement(void)
{
	TokenList      *t;

	if ((t = match_attrib_call()) ||
	    (t = match_conditional()) ||
	    (t = match_while()) ||
	    (t = match_for()) ||
	    (t = match_read()) ||
	    (t = match_write()) ||
	    (t = match_statements()) ||
	    (t = match_function_declare()) ||
	    (t = match_procedure_declare()))
	    return t;	/* FIXME: mover esses dois pra match_statements()? */

	return NULL;
}

/* <atribuicao_chprocedimento> ::= (<cmd_atribuicao>|<chamada_procedimento>) */
static TokenList      *
match_attrib_call(void)
{
	TokenList      *t;

	if ((t = match_attrib()) ||
	    (t = match_procedure_call())) {
	    	tl_add_semicolon(t, line, column);
		return t;
	}
	return NULL;
}

/* <cmd_atribuicao> ::= <identificador> := <expressao> */
static TokenList      *
match_attrib(void)
{
	TokenList      *tl, *t1, *t2;

	if ((t1 = match_identifier())) {
		tl = tl_new();

		tl_ref(t1);
		tl_add_token(&tl, t1);

		if ((t2 = match_token(T_ATTRIB))) {
			tl_add_token(&tl, t2);
			tl_add_token(&tl, match_expression_req());

			tl_unref(t1);
			return tl;
		} else {
			unget_tokenlist(t1);
			tl_unref(t1);
			tl_destroy(tl);
		}
	}
	return NULL;
}

/* <chamada_procedimento> ::= <identificador> */
static TokenList      *
match_procedure_call(void)
{
	return match_identifier();
}

/* <cmd_condicional> ::= se <expressao> entao <comando> [senao <comando>] */
static TokenList      *
match_conditional(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_IF))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		tl_add_token(&tl, match_expression_req());
		tl_add_token(&tl, match_token_req(T_THEN));
		
		tl_add_token(&tl, match_statement_req());
		
		if ((t = match_token(T_ELSE))) {
			tl_add_token(&tl, t);
			tl_add_token(&tl, match_statement_req());
		}
		
		tl_add_semicolon(tl, line, column);

		return tl;
	}
	return NULL;
}

/* <cmd_para> ::= para <variavel> := <expressao> enquanto <expressao> [passo <expressao>] faca <comando> */
static TokenList      *
match_for(void)
{
	TokenList	*tl, *t;
	
	if ((t = match_token(T_FOR))) {
		tl = tl_new();
		
		tl_add_token(&tl, t);
		tl_add_token(&tl, match_identifier_req());
		SUPPRESS(match_token_req(T_ATTRIB));
		tl_add_token(&tl, match_expression_req());
		tl_add_token(&tl, match_token_req(T_WHILE));
		tl_add_token(&tl, match_expression_req());
		
		tl_add_semicolon(tl, line, column);

		if ((t = match_token(T_STEP))) {
			tl_add_token(&tl, t);
			tl_add_token(&tl, match_expression_req());

			tl_add_semicolon(tl, line, column);
		}
		
		SUPPRESS(match_token_req(T_DO));
		tl_add_token(&tl, match_statement_req());

		tl_add_semicolon(tl, line, column);
		
		return tl;
	}

	return NULL;
}

/* <cmd_enquanto> ::= enquanto <expressao> faca <comando> */
static TokenList      *
match_while(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_WHILE))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		tl_add_token(&tl, match_expression_req());
		tl_add_token(&tl, match_token_req(T_DO));
		tl_add_token(&tl, match_statement_req());
		
		tl_add_semicolon(tl, line, column);

		return tl;
	}
	return NULL;
}

/* <cmd_leitura> ::= leia(<identificador>) */
static TokenList      *
match_read(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_READ))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		SUPPRESS(match_token_req(T_OPENPAREN));
		tl_add_token(&tl, match_identifier_req());
		SUPPRESS(match_token_req(T_CLOSEPAREN));

		tl_add_semicolon(tl, line, column);

		return tl;
	}
	return NULL;
}

/* <cmd_escrita> ::= escreva(<identificador>) */
static TokenList      *
match_write(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_WRITE))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		SUPPRESS(match_token_req(T_OPENPAREN));
		tl_add_token(&tl, match_identifier_req());
		SUPPRESS(match_token_req(T_CLOSEPAREN));

		tl_add_semicolon(tl, line, column);

		return tl;
	}
	return NULL;
}

/*
 * <expressao> ::= <expressao_simples> [<operador_relacional><expressao_simples>]
 */
static TokenList      *
match_expression(void)
{
	TokenList      *tl, *t;

	tl = tl_new();
	
	tl_add_token(&tl, match_simple_expression_req());

	if ((t = match_relational_op())) {
		tl_add_token(&tl, t);
		tl_add_token(&tl, match_simple_expression_req());
	}
	
	return tl;
}

/* <operador_relacional> ::= (<> | = | < | <= | > | >= ) */
static TokenList      *
match_relational_op(void)
{
	TokenList      *t;

	if ((t = match_token(T_OP_DIFFERENT)) ||
	    (t = match_token(T_OP_EQUAL)) ||
	    (t = match_token(T_OP_LEQ)) ||
	    (t = match_token(T_OP_LT)) ||
	    (t = match_token(T_OP_GEQ)) ||
	    (t = match_token(T_OP_GT))) {
		return t;
	}
	return NULL;
}

/* <expressao_simples> ::= [(+|-)] <termo> {(+|-|ou) <termo> } */
static TokenList      *
match_simple_expression(void)
{
	TokenList      *tl, *t;
	
	tl = tl_new();
	
	if ((t = match_token(T_UNARY_MINUS)) ||
	    (t = match_token(T_UNARY_PLUS))) {
		tl_add_token(&tl, t);
	}
	tl_add_token(&tl, match_term_req());

	while ((t = match_token(T_PLUS)) ||
	       (t = match_token(T_MINUS)) ||
	       (t = match_token(T_OR))) {
		tl_add_token(&tl, t);
		tl_add_token(&tl, match_term_req());
	}

	return tl;
}

/* <termo> ::= <fator> {(*|div|e) <fator>} */
static TokenList      *
match_term(void)
{
	TokenList      *tl, *t;

	if ((t = match_factor())) {
		tl = tl_new();

		tl_add_token(&tl, t);

		while ((t = match_token(T_MULTIPLY)) ||
		       (t = match_token(T_DIVIDE)) ||
		       (t = match_token(T_AND))) {
			tl_add_token(&tl, t);
			tl_add_token(&tl, match_factor_req());
		}

		return tl;
	}
	return NULL;
}


/*
 * <fator> ::= (<variavel>|<numero>|<chamada_funcao>|( <expressao>
 * )|verdadeiro|falso|nao <fator>)
 */
static TokenList      *
match_factor(void)
{
	TokenList      *tl, *t;

	tl = tl_new();

	if ((t = match_token(T_TRUE)) ||
	    (t = match_token(T_FALSE)) ||
	    (t = match_variable()) ||
	    (t = match_number()) ||
	    (t = match_function_call())) {
		tl_add_token(&tl, t);
	} else if ((t = match_token(T_OPENPAREN))) {
		tl_add_token(&tl, t);
		tl_add_token(&tl, match_expression_req());
		tl_add_token(&tl, match_token_req(T_CLOSEPAREN));
	} else if ((t = match_token(T_NOT))) {
		tl_add_token(&tl, t);
		tl_add_token(&tl, match_factor_req());
	} else {
		tl_destroy(tl);
		
		return NULL;
	}

	return tl;
}

/* <variavel> ::= <identificador> */
static TokenList      *
match_variable(void)
{
	return match_identifier();
}

/* <chamada_funcao> ::= <identificador> */
static TokenList      *
match_function_call(void)
{
	return match_identifier();
}

int
reserved_token(char *token)
{
	unsigned int    i;

	for (i = 0; i < G_N_ELEMENTS(literals); i++) {
		if (!strcmp(literals[i], token)) {
			return TRUE;
		}
	}

	return FALSE;
}

/* <identificador> ::= <letra> {<letra> | <digito> | _ } */
static TokenList      *
match_identifier(void)
{
	Token          *t;
	TokenList      *tl;
	char            buffer[128], ch;
	int             index = 0;

	ch = eat_whitespace_until(is_alpha, NULL);
	if (ch == -1) {
		return NULL;
	} else {
		buffer[index++] = ch;
		
		while (index < 128) {
			ch = get_character();
			if (isalpha(ch) || isdigit(ch) || ch == '_') {
				buffer[index++] = ch;
			} else {
				unget_character(ch);
				
				break;
			}
		}
		
		buffer[index] = '\0';
		
		if (reserved_token(buffer)) {
			unget_string(buffer);
			return NULL;
		}

		t = g_new0(Token, 1);
		t->type = T_IDENTIFIER;
		t->id = strdup(buffer);
		t->line = line;
		t->column = column - index - 1; /* point to start of token */

		tl = tl_new();
		tl->tokens = g_list_append(tl->tokens, t);

		return tl;		
	}
}

/* <numero> ::= <digito> {<digito>} */
static TokenList      *
match_number(void)
{
	Token          *t;
	TokenList      *tl;
	char            buffer[128], ch;
	int             index = 0;
	
	ch = eat_whitespace_until(is_digit, NULL);
	if (ch == -1) {
		return NULL;
	} else {
		buffer[index++] = ch;
		
		while (index < 128) {
			ch = get_character();
			if (isdigit(ch)) {
				buffer[index++] = ch;
			} else {
				unget_character(ch);
				
				break;
			}
		}
		buffer[index] = '\0';
		
		tl = tl_new();

		t = g_new0(Token, 1);
		t->type = T_NUMBER;
		t->id = strdup(buffer);
		t->line = line;
		t->column = column;

		tl->tokens = g_list_append(tl->tokens, t);

		return tl;			
	}
}

TokenList      *
lex(void)
{
	return match_program();
}

static int
qual_cor(TokenType t)
{
	switch (t) {
	case T_AND:
	case T_OR:
		return 3;

	case T_ATTRIB:
		return 2;

	case T_BOOLEAN:
	case T_INTEGER:
		return 1;

	case T_DO:
	case T_ELSE:
	case T_FALSE:
	case T_TRUE:
	case T_IF:
	case T_THEN:
	case T_VAR:
	case T_WHILE:
	case T_PROGRAM:
	case T_WRITE:
	case T_READ:
	case T_FOR:
	case T_STEP:
		return 0;

	case T_FUNCTION:
	case T_PROCEDURE:
	case T_BEGIN:
	case T_END:
		return 5;

	case T_MINUS:
	case T_MULTIPLY:
	case T_PLUS:
	case T_DIVIDE:
	case T_NOT:
	case T_UNARY_MINUS:
		return 6;

	case T_IDENTIFIER:
	case T_NUMBER:
		return 4;

	default:
		return 7;
	}

	return 0;		/* make compiler happy! */
}

int
lex_test_main(int argc, char **argv)
{
	Token          *token;
	TokenType	last = T_PROGRAM;
	TokenList      *tl;
	GList          *t;
	char           *cores[] = {"37;1", "32;1", "32", "36;1", "33;1", "34;1", "31", "33"};
	gint		nivel = 0;
	
	if ((tl = match_program())) {
		for (t = tl->tokens; t; t = t->next) {
			token = (Token *) t->data;

			if (token->type == T_BEGIN ||
			    token->type == T_VAR ||
			    token->type == T_FUNCTION ||
			    token->type == T_PROCEDURE) {
				putchar('\n');
			}

			if (token->type == T_BEGIN) {
				nivel++;
			} else if (token->type == T_END) {
				nivel--;
			}

			if (!(token->type == T_SEMICOLON && last == T_SEMICOLON)) {
				printf("\033[40;%sm%s\033[m ",
				       cores[qual_cor(token->type)],
				       token->id);

				if (token->type == T_SEMICOLON ||
				    token->type == T_BEGIN) {
					putchar('\n');
				}
			}
			
			
			last = token->type;
		}
	}
	
	puts("");
	
	tl_unref(tl);
	return 0;
}

/*
 * Simple Pascal Compiler
 * Lexical and Syntatical Analysis
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@linuxmag.com.br>
 */

/*
 * FIXME: programx fica com dois tokens: program x
 * FIXME: beginx:=10end usa 5 tokens: begin, x, :=, 10, end
 * FIXME: (expressao) nao tah funcionando. pq? :~
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"

const char     *literals[] = {
	"(nil)", "literal", ",", "and", ":=", "boolean", ")",
	":", "/", "do", "else", "false", "end",
	"function", "if", "begin", "integer", "-",
	"*", "not", "<>", "(", "=", ">",
	">=", "<", "<=", "or", ".", "+",
	"procedure", "program", "read", ";", "then",
	"true", "var", "while", "write",
	"id", "number",
	"return", "function call", "procedure call"
};

static int      line = 1, column = 1;
static char     char_buf[32];
static int      char_buf_rp = 0, char_buf_wp = 0;

static TokenList *match_program(void);
static TokenList *match_block(void);
static TokenList *match_variable_declare_step(void);
static TokenList *match_variable_declare(void);
static TokenList *match_type(void);
static TokenList *match_subroutine_declare_step(void);
static TokenList *match_procedure_declare(void);
static TokenList *match_function_declare(void);
static TokenList *match_commands(void);
static TokenList *match_command(void);
static TokenList *match_attrib_call(void);
static TokenList *match_attrib(void);
static TokenList *match_procedure_call(void);
static TokenList *match_conditional(void);
static TokenList *match_while(void);
static TokenList *match_read(void);
static TokenList *match_write(void);
static TokenList *match_expression(void);
static TokenList *match_relational_op(void);
static TokenList *match_simple_expression(void);
static TokenList *match_term(void);
static TokenList *match_factor(void);
static TokenList *match_variable(void);
static TokenList *match_function_call(void);
static TokenList *match_identifier(void);
static TokenList *match_number(void);

static void     lex_error(char *message);
static void     lex_error2(char *message, char *arg);

static void
char_buf_put(char ch)
{
	int             wp = (char_buf_wp + 1) % 32;

	if (wp != char_buf_rp) {
		char_buf[char_buf_wp] = ch;
		char_buf_wp = wp;
	} else {
		lex_error("buffer full");
	}
}

static int
char_buf_get(void)
{
	int             ch = -1;

	if (char_buf_rp != char_buf_wp) {
		ch = char_buf[char_buf_rp];
		char_buf_rp = (char_buf_rp + 1) % 32;
	}
	return ch;
}

static void
lex_error(char *message)
{
	printf("Error: Line %d, column %d: %s\n", line, column, message);
	exit(1);
}

static void
lex_error2(char *message, char *arg)
{
	printf("Error: Line %d, column %d: %s%s\n", line, column, message, arg);
	exit(1);
}

static int
get_character(void)
{
	int             ch = char_buf_get();

	if (ch == -1)
		ch = getchar();
	
	if (ch == '\n') {
		line++;
		column = 1;
	} else {
		column++;
	}
	
	return ch;
}

static int
get_character_notblank(void)
{
	char            ch;

	do {
		ch = get_character();

		/* eat comments */
		if (ch == '{') {
			while (1) {
				ch = get_character();
				if (ch == '}')
					break;
				else if (ch == EOF)
					lex_error("Expecting: }");
			}

			ch = get_character();
		}
	} while (isspace(ch));

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

	char_buf_put(ch);
}

static void
unget_string(char *string)
{
	while (*string) {
		unget_character(*string++);
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

TokenList      *
tl_new(void)
{
	TokenList      *tl;

	tl = g_new0(TokenList, 1);
	tl->tokens = NULL;

	return tl;
}

void
tl_unref(TokenList * t)
{
	if (t && --t->ref_count < 0) {
		tl_destroy(t);
	}
}

void
tl_ref(TokenList * t)
{
	t->ref_count++;
}

void
tl_add_token(TokenList ** token_list, TokenList * t)
{
	GList          *token;

	if (t) {
		for (token = t->tokens; token; token = token->next) {
			(*token_list)->tokens = g_list_append((*token_list)->tokens, token->data);
		}

		tl_unref(t);
	}
}

void
tl_destroy(TokenList * tl)
{
	g_list_free(tl->tokens);	
	g_free(tl);
}

static TokenList      *
match_token(TokenType token_type)
{
	Token          *token;
	TokenList      *token_list;
	char            buffer[128], ch;
	int             index = 0;
	const char     *literal = literals[token_type];

	buffer[0] = '\0';

	for (ch = get_character_notblank(); !strchr("\t\n\r ", ch); ch = get_character()) {
		buffer[index++] = ch;
		buffer[index] = '\0';

		if (strcmp(literal, buffer) == 0) {
			token = g_new0(Token, 1);
			token->type = token_type;
			token->id = (char *) literal;
			token->line = line;
			token->column = column - index - 1; /* point to start of token */

			token_list = g_new0(TokenList, 1);
			token_list->tokens = g_list_append(token_list->tokens, token);

			return token_list;
		} else if (strncmp(literal, buffer, index - 1) != 0) {
			for (ch = char_buf_get(); ch != -1; ch = char_buf_get()) {
				buffer[index++] = ch;
				buffer[index] = '\0';
			}

			unget_string(buffer);

			return NULL;
		}
	}

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
	
	lex_error2("expecting: ", (char *) literals[token_type]);

	/* make compiler happy */
	return NULL;
}

#define REQ(matcher,err)						\
  static TokenList * matcher##_req(void) {		\
    TokenList *tl = matcher();					\
    if (tl == NULL) {							\
      lex_error("expecting: " err);				\
    }											\
    return tl;									\
  }

#define SUPPRESS(match_function)				\
  tl_unref(match_function)


/*
 * Since REQ macro expands to a static function, this function might not be used
 * in actual code; thus, the macro call is commented out, so gcc stops whinning.
 */

/* REQ(match_program, "program") */
REQ(match_block, "block")
/* REQ(match_variable_declare_step, "variable declare step") */
REQ(match_variable_declare, "variable declaration")
REQ(match_type, "type")
/* REQ(match_subroutine_declare_step, "subroutine declare step") */
/* REQ(match_procedure_declare, "procedure declare") */
/* REQ(match_function_declare, "function declare") */
REQ(match_commands, "commands")
REQ(match_command, "command")
/* REQ(match_attrib_call, "attrib call") */
/* REQ(match_attrib, "attrib") */
/* REQ(match_procedure_call, "procedure call") */
/* REQ(match_conditional, "conditional") */
/* REQ(match_while, "while") */
/* REQ(match_read, "read") */
/* REQ(match_write, "write") */
REQ(match_expression, "expression")
/* REQ(match_relational_op, "relational op") */
REQ(match_simple_expression, "simple expression")
REQ(match_term, "expression term")
REQ(match_factor, "expression factor")
/* REQ(match_variable, "variable") */
/* REQ(match_function_call, "function call") */
REQ(match_identifier, "identifier")
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

	tl = tl_new();

	if ((t = match_variable_declare_step())) {
		tl_add_token(&tl, t);
	}
	if ((t = match_subroutine_declare_step())) {
		tl_add_token(&tl, t);
	}
	tl_add_token(&tl, match_commands_req());

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
match_commands(void)
{
	TokenList      *tl, *t;

	if ((t = match_token(T_BEGIN))) {
		tl = tl_new();

		tl_add_token(&tl, t);
		tl_add_token(&tl, match_command_req());

		while ((t = match_token(T_SEMICOLON))) {
			tl_add_token(&tl, t);
			tl_add_token(&tl, match_command());
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
match_command(void)
{
	TokenList      *t;

	if ((t = match_attrib_call()) ||
	    (t = match_conditional()) ||
	    (t = match_while()) ||
	    (t = match_read()) ||
	    (t = match_write()) ||
	    (t = match_commands()) ||
	    (t = match_function_declare()) ||
	    (t = match_procedure_declare()))	/* XXX: mover esses dois pra
						 * match_commands()? */
		return t;

	return NULL;
}

/* <atribuicao_chprocedimento> ::= (<cmd_atribuicao>|<chamada_procedimento>) */
static TokenList      *
match_attrib_call(void)
{
	TokenList      *t;

	if ((t = match_attrib()) ||
	    (t = match_procedure_call())) {
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
			char            buffer[128];
			int             index = 0, ch;

			for (ch = char_buf_get(); ch != -1; ch = char_buf_get()) {
				buffer[index++] = ch;
				buffer[index] = '\0';
			}

			unget_tokenlist(t1);
			unget_string(buffer);

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
		tl_add_token(&tl, match_command_req());

		if ((t = match_token(T_ELSE))) {
			tl_add_token(&tl, t);
			tl_add_token(&tl, match_command_req());
		}
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
		tl_add_token(&tl, match_command_req());

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
	int				started_with_paren = 0;

	tl = tl_new();
	
	if ((t = match_token(T_OPENPAREN))) {
		tl_add_token(&tl, t);
		started_with_paren = 1;
	}
	
	tl_add_token(&tl, match_simple_expression_req());

	if ((t = match_relational_op())) {
		tl_add_token(&tl, t);
		tl_add_token(&tl, match_simple_expression_req());
	}
	
	if (started_with_paren) {
		tl_add_token(&tl, match_token_req(T_CLOSEPAREN));
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
	    (t = match_token(T_OP_LT)) ||
	    (t = match_token(T_OP_LEQ)) ||
	    (t = match_token(T_OP_GT)) ||
	    (t = match_token(T_OP_GEQ))) {
		return t;
	}
	return NULL;
}

/* <expressao_simples> ::= [(+|-)] <termo> {(+|-|ou) <termo> } */
static TokenList      *
match_simple_expression(void)
{
	TokenList      *tl, *t;
	//int started_with_paren = 0;
	
	tl = tl_new();
	
	if ((t = match_token(T_PLUS)) ||
	    (t = match_token(T_MINUS))) {	/* FIXME T_PLUS eu posso
						 * ignorar; T_MINUS_UNARY!@# */
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

	if ((t = match_variable()) ||
	    (t = match_number()) ||
	    (t = match_function_call()) ||
	    (t = match_token(T_TRUE)) ||
	    (t = match_token(T_FALSE))) {
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

	if ((ch = get_character_notblank()) && isalpha(ch)) {
		buffer[index++] = ch;

		do {
			ch = get_character();
			if (isalnum(ch) || ch == '_') {
				buffer[index++] = ch;
			} else {
				unget_character(ch);

				buffer[index] = '\0';
				break;
			}
		} while (1);

		if (reserved_token(buffer)) {
			goto unget;
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
	buffer[index++] = ch;
	buffer[index] = '\0';

unget:
	for (ch = char_buf_get(); ch != -1; ch = char_buf_get()) {
		buffer[index++] = ch;
		buffer[index] = '\0';
	}

	unget_string(buffer);

	return NULL;
}

/* <numero> ::= <digito> {<digito>} */
static TokenList      *
match_number(void)
{
	Token          *t;
	TokenList      *tl;
	char            buffer[128], ch;
	int             index = 0;

	for (ch = get_character_notblank();; ch = get_character()) {
		if (isdigit(ch)) {
			buffer[index++] = ch;
		} else {
			unget_character(ch);
			buffer[index] = '\0';
			break;
		}
	}

	if (index) {
		tl = tl_new();

		t = g_new0(Token, 1);
		t->type = T_NUMBER;
		t->id = strdup(buffer);
		t->line = line;
		t->column = column;

		tl->tokens = g_list_append(tl->tokens, t);

		return tl;
	}
	return NULL;
}

TokenList      *
lex(void)
{
	return match_program();
}

#ifdef LEX_TEST
int
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
	case T_PROCEDURE:
	case T_IF:
	case T_THEN:
	case T_VAR:
	case T_WHILE:
	case T_PROGRAM:
	case T_WRITE:
	case T_READ:
		return 0;

	case T_FUNCTION:
	case T_BEGIN:
	case T_END:
		return 5;

	case T_MINUS:
	case T_MULTIPLY:
	case T_PLUS:
	case T_DIVIDE:
		return 6;

	case T_IDENTIFIER:
	case T_NUMBER:
		return 4;

	default:
		return 7;
	}

	return 0;		/* make compiler happy, again! */
}

int
main(int argc, char **argv)
{
	Token          *token;
	TokenList      *tl;
	GList          *t;
	char           *cores[] = {"37;1", "32;1", "32", "36;1", "33;1", "34;1", "31", "33"};
	
	if (argc >= 2) {
		fclose(stdin);
		stdin = fopen(argv[1], "r");		
	}

	if ((tl = match_program())) {
		for (t = tl->tokens; t; t = t->next) {
			token = (Token *) t->data;

			if (token->type == T_BEGIN || token->type == T_VAR || token->type == T_FUNCTION || token->type == T_PROCEDURE) {
				putchar('\n');
			}

			printf("\033[40;%sm%s\033[m ", cores[qual_cor(token->type)], token->id);

			if (token->type == T_SEMICOLON || token->type == T_BEGIN || token->type == T_END) {
				putchar('\n');
			}
		}
	}
	
	tl_unref(tl);
	return 0;
}
#endif				/* LEX_TEST */

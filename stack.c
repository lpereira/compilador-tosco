/*
 * Simple Pascal Compiler
 * Stack
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#include "stack.h"

Stack*
stack_new(void)
{
	Stack *stack;
	
	stack = g_new0(Stack, 1);
	stack->_stack = NULL;
	
	return stack;
}

void
stack_free(Stack *stack)
{
	g_return_if_fail(stack);
	
	g_slist_free(stack->_stack);
	g_free(stack);
}

gboolean
stack_is_empty(Stack *stack)
{
	g_return_val_if_fail(stack, TRUE);
	
	return stack->_stack == NULL;
}

void
stack_push(Stack *stack, gpointer data)
{
	g_return_if_fail(stack);
	
	stack->_stack = g_slist_prepend(stack->_stack, data);
}

gpointer
stack_pop(Stack *stack)
{
	GSList *element;
	gpointer data = NULL;
	
	if (stack && stack->_stack) {
		element = stack->_stack;
		stack->_stack = element->next;

		data = element->data;
		g_slist_free_1(element);
	}
	
	return data;
}

gpointer
stack_peek(Stack *stack)
{
	return (stack && stack->_stack) ? stack->_stack->data : NULL;
}

/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

/*
 * changelog: add background, 2010, Grégory Mounié
 */

#pragma once

/* If GNU Readline is not available, internal readline will be used*/
#include "variante.h"

#if USE_GNU_READLINE == 0
/* Read a line from standard input and put it in a char[] */
char *readline(const char *prompt);

#else
#include <readline/readline.h>
#include <readline/history.h>

#endif


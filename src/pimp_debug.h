/* pimp_debug.h -- Debugging helpers
 * Copyright (C) 2005-2006 J�rn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_DEBUG_H
#define PIMP_DEBUG_H

#ifdef DEBUG_PRINTF_ENABLED
 #define DEBUG_PRINT(X) iprintf X
#else
 #define DEBUG_PRINT(X)
#endif

#ifdef ASSERT_ENABLED
 #include <stdlib.h>
 #include <stdio.h>
 #define ASSERT(expr) \
	do { \
		if (!(expr)) iprintf("*** ASSERT FAILED %s AT (%s:%i)\n", #expr, __FILE__, __LINE__); \
	} while(0)
#else
 #define ASSERT(expr)
#endif

#endif /* PIMP_DEBUG_H */

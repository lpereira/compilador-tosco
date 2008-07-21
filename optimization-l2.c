#include "optimization-l2.h"
#include "codeemitter.h"

/*
 * Possible optimizations
 *
 * Easy:
 * - Remove "label n: goto label m;" 
 * - Change "tx := not tx; if_not tx (...)" to "if tx (...)"
 *
 * Difficult:
 * - Common subexpression elimination
 */

void
optimization_l2(Emitter *emitter)
{

}

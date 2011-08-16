#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "prb.h"
#include "prb-tree.h"
//#include <stdarg.h>
//#include <stdio.h>

int
main (int argc, char *argv[])
{
  int seedlen=5;
  int success=1;                  /* Everything okay so far? */

  /* Initialize |pgm_name|, using |argv[0]| if sensible. */
  pgm_name = argv[0] != NULL && argv[0][0] != '\0' ? argv[0] : "readsdedump";

  struct prb_table *tree;
  tree = prb_create (compare_fixed_strings, &seedlen, NULL);
  if (tree == NULL)
    {
      printf ("  Out of memory creating tree.\n");
      return 1;
    }
  //pgm_fail("%s","test");
  prb_destroy (tree, NULL);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

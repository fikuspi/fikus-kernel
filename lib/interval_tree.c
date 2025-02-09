#include <fikus/init.h>
#include <fikus/interval_tree.h>
#include <fikus/interval_tree_generic.h>

#define START(node) ((node)->start)
#define LAST(node)  ((node)->last)

INTERVAL_TREE_DEFINE(struct interval_tree_node, rb,
		     unsigned long, __subtree_last,
		     START, LAST,, interval_tree)

/*
 *  Profiling infrastructure declarations.
 *
 *  This file is based on gcc-internal definitions. Data structures are
 *  defined to be compatible with gcc counterparts. For a better
 *  understanding, refer to gcc source: gcc/gcov-io.h.
 *
 *    Copyright IBM Corp. 2009
 *    Author(s): Peter Oberparleiter <oberpar@fikus.vnet.ibm.com>
 *
 *    Uses gcc-internal data definitions.
 */

#ifndef GCOV_H
#define GCOV_H GCOV_H

#include <fikus/types.h>

/*
 * Profiling data types used for gcc 3.4 and above - these are defined by
 * gcc and need to be kept as close to the original definition as possible to
 * remain compatible.
 */
#define GCOV_COUNTERS		5
#define GCOV_DATA_MAGIC		((unsigned int) 0x67636461)
#define GCOV_TAG_FUNCTION	((unsigned int) 0x01000000)
#define GCOV_TAG_COUNTER_BASE	((unsigned int) 0x01a10000)
#define GCOV_TAG_FOR_COUNTER(count)					\
	(GCOV_TAG_COUNTER_BASE + ((unsigned int) (count) << 17))

#if BITS_PER_LONG >= 64
typedef long gcov_type;
#else
typedef long long gcov_type;
#endif

/**
 * struct gcov_fn_info - profiling meta data per function
 * @ident: object file-unique function identifier
 * @checksum: function checksum
 * @n_ctrs: number of values per counter type belonging to this function
 *
 * This data is generated by gcc during compilation and doesn't change
 * at run-time.
 */
struct gcov_fn_info {
	unsigned int ident;
	unsigned int checksum;
	unsigned int n_ctrs[0];
};

/**
 * struct gcov_ctr_info - profiling data per counter type
 * @num: number of counter values for this type
 * @values: array of counter values for this type
 * @merge: merge function for counter values of this type (unused)
 *
 * This data is generated by gcc during compilation and doesn't change
 * at run-time with the exception of the values array.
 */
struct gcov_ctr_info {
	unsigned int	num;
	gcov_type	*values;
	void		(*merge)(gcov_type *, unsigned int);
};

/**
 * struct gcov_info - profiling data per object file
 * @version: gcov version magic indicating the gcc version used for compilation
 * @next: list head for a singly-linked list
 * @stamp: time stamp
 * @filename: name of the associated gcov data file
 * @n_functions: number of instrumented functions
 * @functions: function data
 * @ctr_mask: mask specifying which counter types are active
 * @counts: counter data per counter type
 *
 * This data is generated by gcc during compilation and doesn't change
 * at run-time with the exception of the next pointer.
 */
struct gcov_info {
	unsigned int			version;
	struct gcov_info		*next;
	unsigned int			stamp;
	const char			*filename;
	unsigned int			n_functions;
	const struct gcov_fn_info	*functions;
	unsigned int			ctr_mask;
	struct gcov_ctr_info		counts[0];
};

/* Base interface. */
enum gcov_action {
	GCOV_ADD,
	GCOV_REMOVE,
};

void gcov_event(enum gcov_action action, struct gcov_info *info);
void gcov_enable_events(void);

/* Iterator control. */
struct seq_file;
struct gcov_iterator;

struct gcov_iterator *gcov_iter_new(struct gcov_info *info);
void gcov_iter_free(struct gcov_iterator *iter);
void gcov_iter_start(struct gcov_iterator *iter);
int gcov_iter_next(struct gcov_iterator *iter);
int gcov_iter_write(struct gcov_iterator *iter, struct seq_file *seq);
struct gcov_info *gcov_iter_get_info(struct gcov_iterator *iter);

/* gcov_info control. */
void gcov_info_reset(struct gcov_info *info);
int gcov_info_is_compatible(struct gcov_info *info1, struct gcov_info *info2);
void gcov_info_add(struct gcov_info *dest, struct gcov_info *source);
struct gcov_info *gcov_info_dup(struct gcov_info *info);
void gcov_info_free(struct gcov_info *info);

struct gcov_link {
	enum {
		OBJ_TREE,
		SRC_TREE,
	} dir;
	const char *ext;
};
extern const struct gcov_link gcov_link[];

#endif /* GCOV_H */

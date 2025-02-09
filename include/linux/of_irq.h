#ifndef __OF_IRQ_H
#define __OF_IRQ_H

#include <fikus/types.h>
#include <fikus/errno.h>
#include <fikus/irq.h>
#include <fikus/irqdomain.h>
#include <fikus/ioport.h>
#include <fikus/of.h>

/**
 * of_irq - container for device_node/irq_specifier pair for an irq controller
 * @controller: pointer to interrupt controller device tree node
 * @size: size of interrupt specifier
 * @specifier: array of cells @size long specifing the specific interrupt
 *
 * This structure is returned when an interrupt is mapped. The controller
 * field needs to be put() after use
 */
#define OF_MAX_IRQ_SPEC		4 /* We handle specifiers of at most 4 cells */
struct of_irq {
	struct device_node *controller; /* Interrupt controller node */
	u32 size; /* Specifier size */
	u32 specifier[OF_MAX_IRQ_SPEC]; /* Specifier copy */
};

typedef int (*of_irq_init_cb_t)(struct device_node *, struct device_node *);

/*
 * Workarounds only applied to 32bit powermac machines
 */
#define OF_IMAP_OLDWORLD_MAC	0x00000001
#define OF_IMAP_NO_PHANDLE	0x00000002

#if defined(CONFIG_PPC32) && defined(CONFIG_PPC_PMAC)
extern unsigned int of_irq_workarounds;
extern struct device_node *of_irq_dflt_pic;
extern int of_irq_map_oldworld(struct device_node *device, int index,
			       struct of_irq *out_irq);
#else /* CONFIG_PPC32 && CONFIG_PPC_PMAC */
#define of_irq_workarounds (0)
#define of_irq_dflt_pic (NULL)
static inline int of_irq_map_oldworld(struct device_node *device, int index,
				      struct of_irq *out_irq)
{
	return -EINVAL;
}
#endif /* CONFIG_PPC32 && CONFIG_PPC_PMAC */


extern int of_irq_map_raw(struct device_node *parent, const __be32 *intspec,
			  u32 ointsize, const __be32 *addr,
			  struct of_irq *out_irq);
extern int of_irq_map_one(struct device_node *device, int index,
			  struct of_irq *out_irq);
extern unsigned int irq_create_of_mapping(struct device_node *controller,
					  const u32 *intspec,
					  unsigned int intsize);
extern int of_irq_to_resource(struct device_node *dev, int index,
			      struct resource *r);
extern int of_irq_count(struct device_node *dev);
extern int of_irq_to_resource_table(struct device_node *dev,
		struct resource *res, int nr_irqs);

extern void of_irq_init(const struct of_device_id *matches);

#if defined(CONFIG_OF)
/*
 * irq_of_parse_and_map() is used by all OF enabled platforms; but SPARC
 * implements it differently.  However, the prototype is the same for all,
 * so declare it here regardless of the CONFIG_OF_IRQ setting.
 */
extern unsigned int irq_of_parse_and_map(struct device_node *node, int index);
extern struct device_node *of_irq_find_parent(struct device_node *child);

#else /* !CONFIG_OF */
static inline unsigned int irq_of_parse_and_map(struct device_node *dev,
						int index)
{
	return 0;
}

static inline void *of_irq_find_parent(struct device_node *child)
{
	return NULL;
}
#endif /* !CONFIG_OF */

#endif /* __OF_IRQ_H */

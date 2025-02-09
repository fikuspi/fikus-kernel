#include <fikus/types.h>
#include <net/net_namespace.h>
#include <fikus/netfilter/nf_conntrack_common.h>
#include <fikus/netfilter/nf_conntrack_tuple_common.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_extend.h>

#include <uapi/fikus/netfilter/xt_connlabel.h>

struct nf_conn_labels {
	u8 words;
	unsigned long bits[];
};

static inline struct nf_conn_labels *nf_ct_labels_find(const struct nf_conn *ct)
{
#ifdef CONFIG_NF_CONNTRACK_LABELS
	return nf_ct_ext_find(ct, NF_CT_EXT_LABELS);
#else
	return NULL;
#endif
}

static inline struct nf_conn_labels *nf_ct_labels_ext_add(struct nf_conn *ct)
{
#ifdef CONFIG_NF_CONNTRACK_LABELS
	struct nf_conn_labels *cl_ext;
	struct net *net = nf_ct_net(ct);
	u8 words;

	words = ACCESS_ONCE(net->ct.label_words);
	if (words == 0 || WARN_ON_ONCE(words > 8))
		return NULL;

	cl_ext = nf_ct_ext_add_length(ct, NF_CT_EXT_LABELS,
				      words * sizeof(long), GFP_ATOMIC);
	if (cl_ext != NULL)
		cl_ext->words = words;

	return cl_ext;
#else
	return NULL;
#endif
}

bool nf_connlabel_match(const struct nf_conn *ct, u16 bit);
int nf_connlabel_set(struct nf_conn *ct, u16 bit);

int nf_connlabels_replace(struct nf_conn *ct,
			  const u32 *data, const u32 *mask, unsigned int words);

#ifdef CONFIG_NF_CONNTRACK_LABELS
int nf_conntrack_labels_init(void);
void nf_conntrack_labels_fini(void);
#else
static inline int nf_conntrack_labels_init(void) { return 0; }
static inline void nf_conntrack_labels_fini(void) {}
#endif

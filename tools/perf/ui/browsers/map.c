#include "../libslang.h"
#include <elf.h>
#include <inttypes.h>
#include <sys/ttydefaults.h>
#include <string.h>
#include <fikus/bitops.h>
#include "../../util/util.h"
#include "../../util/debug.h"
#include "../../util/symbol.h"
#include "../browser.h"
#include "../helpline.h"
#include "../keysyms.h"
#include "map.h"

struct map_browser {
	struct ui_browser b;
	struct map	  *map;
	u8		  addrlen;
};

static void map_browser__write(struct ui_browser *self, void *nd, int row)
{
	struct symbol *sym = rb_entry(nd, struct symbol, rb_node);
	struct map_browser *mb = container_of(self, struct map_browser, b);
	bool current_entry = ui_browser__is_current_entry(self, row);
	int width;

	ui_browser__set_percent_color(self, 0, current_entry);
	slsmg_printf("%*" PRIx64 " %*" PRIx64 " %c ",
		     mb->addrlen, sym->start, mb->addrlen, sym->end,
		     sym->binding == STB_GLOBAL ? 'g' :
		     sym->binding == STB_LOCAL  ? 'l' : 'w');
	width = self->width - ((mb->addrlen * 2) + 4);
	if (width > 0)
		slsmg_write_nstring(sym->name, width);
}

/* FIXME uber-kludgy, see comment on cmd_report... */
static u32 *symbol__browser_index(struct symbol *self)
{
	return ((void *)self) - sizeof(struct rb_node) - sizeof(u32);
}

static int map_browser__search(struct map_browser *self)
{
	char target[512];
	struct symbol *sym;
	int err = ui_browser__input_window("Search by name/addr",
					   "Prefix with 0x to search by address",
					   target, "ENTER: OK, ESC: Cancel", 0);
	if (err != K_ENTER)
		return -1;

	if (target[0] == '0' && tolower(target[1]) == 'x') {
		u64 addr = strtoull(target, NULL, 16);
		sym = map__find_symbol(self->map, addr, NULL);
	} else
		sym = map__find_symbol_by_name(self->map, target, NULL);

	if (sym != NULL) {
		u32 *idx = symbol__browser_index(sym);

		self->b.top = &sym->rb_node;
		self->b.index = self->b.top_idx = *idx;
	} else
		ui_helpline__fpush("%s not found!", target);

	return 0;
}

static int map_browser__run(struct map_browser *self)
{
	int key;

	if (ui_browser__show(&self->b, self->map->dso->long_name,
			     "Press <- or ESC to exit, %s / to search",
			     verbose ? "" : "restart with -v to use") < 0)
		return -1;

	while (1) {
		key = ui_browser__run(&self->b, 0);

		switch (key) {
		case '/':
			if (verbose)
				map_browser__search(self);
		default:
			break;
                case K_LEFT:
                case K_ESC:
                case 'q':
                case CTRL('c'):
                        goto out;
		}
	}
out:
	ui_browser__hide(&self->b);
	return key;
}

int map__browse(struct map *self)
{
	struct map_browser mb = {
		.b = {
			.entries = &self->dso->symbols[self->type],
			.refresh = ui_browser__rb_tree_refresh,
			.seek	 = ui_browser__rb_tree_seek,
			.write	 = map_browser__write,
		},
		.map = self,
	};
	struct rb_node *nd;
	char tmp[BITS_PER_LONG / 4];
	u64 maxaddr = 0;

	for (nd = rb_first(mb.b.entries); nd; nd = rb_next(nd)) {
		struct symbol *pos = rb_entry(nd, struct symbol, rb_node);

		if (maxaddr < pos->end)
			maxaddr = pos->end;
		if (verbose) {
			u32 *idx = symbol__browser_index(pos);
			*idx = mb.b.nr_entries;
		}
		++mb.b.nr_entries;
	}

	mb.addrlen = snprintf(tmp, sizeof(tmp), "%" PRIx64, maxaddr);
	return map_browser__run(&mb);
}

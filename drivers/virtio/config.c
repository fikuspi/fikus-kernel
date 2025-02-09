/* Configuration space parsing helpers for virtio.
 *
 * The configuration is [type][len][... len bytes ...] fields.
 *
 * Copyright 2007 Rusty Russell, IBM Corporation.
 * GPL v2 or later.
 */
#include <fikus/err.h>
#include <fikus/virtio.h>
#include <fikus/virtio_config.h>
#include <fikus/bug.h>


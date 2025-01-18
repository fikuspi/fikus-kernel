/*
 * Power trace points
 *
 * Copyright (C) 2009 Arjan van de Ven <arjan@fikus.intel.com>
 */

#include <fikus/string.h>
#include <fikus/types.h>
#include <fikus/workqueue.h>
#include <fikus/sched.h>
#include <fikus/module.h>

#define CREATE_TRACE_POINTS
#include <trace/events/power.h>

EXPORT_TRACEPOINT_SYMBOL_GPL(cpu_idle);


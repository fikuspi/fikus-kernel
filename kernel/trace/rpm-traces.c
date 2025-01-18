/*
 * Power trace points
 *
 * Copyright (C) 2009 Ming Lei <ming.lei@canonical.com>
 */

#include <fikus/string.h>
#include <fikus/types.h>
#include <fikus/workqueue.h>
#include <fikus/sched.h>
#include <fikus/module.h>
#include <fikus/usb.h>

#define CREATE_TRACE_POINTS
#include <trace/events/rpm.h>

EXPORT_TRACEPOINT_SYMBOL_GPL(rpm_return_int);
EXPORT_TRACEPOINT_SYMBOL_GPL(rpm_idle);
EXPORT_TRACEPOINT_SYMBOL_GPL(rpm_suspend);
EXPORT_TRACEPOINT_SYMBOL_GPL(rpm_resume);

/*
 *  S390 version
 *    Copyright IBM Corp. 1999
 *
 *  Derived from "include/asm-i386/cache.h"
 *    Copyright (C) 1992, John Torvalds
 */

#ifndef __ARCH_S390_CACHE_H
#define __ARCH_S390_CACHE_H

#define L1_CACHE_BYTES     256
#define L1_CACHE_SHIFT     8
#define NET_SKB_PAD	   32

#define __read_mostly __attribute__((__section__(".data..read_mostly")))

#endif

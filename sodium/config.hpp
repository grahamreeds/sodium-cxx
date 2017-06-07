/**
 * Copyright (c) 2012-2014, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM_CONFIG_HPP_
#define _SODIUM_CONFIG_HPP_

#include <limits.h>  // for __WORDSIZE
#if defined(SODIUM_EXTRA_INCLUDE)
#include SODIUM_EXTRA_INCLUDE
#endif

#if __WORDSIZE == 32
#define SODIUM_STRONG_BITS 1
#define SODIUM_STREAM_BITS  14
#define SODIUM_NODE_BITS   14
#define SODIUM_CONSERVE_MEMORY
#elif __WORDSIZE == 64
#define SODIUM_STRONG_BITS 1
#define SODIUM_STREAM_BITS  31
#define SODIUM_NODE_BITS   31
#define SODIUM_CONSERVE_MEMORY
#endif

#endif

/**
 * Copyright (c) 2012-2014, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM_COUNT_SET_HPP_
#define _SODIUM_COUNT_SET_HPP_

#include <sodium/config.hpp>
#include <limits.h>
#include <assert.h>

namespace sodium {
    namespace impl {

        struct large_count_set {
            large_count_set(
                    unsigned strong_count_,
                    unsigned stream_count_,
                    unsigned node_count_
                ) : strong_count(strong_count_),
                    stream_count(stream_count_),
                    node_count(node_count_)
            {
            }
            unsigned strong_count;
            unsigned stream_count;
            unsigned node_count;
        };

#if defined(SODIUM_CONSERVE_MEMORY)
        struct small_count_set {
            unsigned is_small:1;
            unsigned strong_count:SODIUM_STRONG_BITS;
            unsigned stream_count:SODIUM_STREAM_BITS;
            unsigned node_count:SODIUM_NODE_BITS;
        };
        
        #define SODIUM_STRONG_MAX ((1u << SODIUM_STRONG_BITS) - 1u)
        #define SODIUM_STREAM_MAX  ((1u << SODIUM_STREAM_BITS) - 1u)
        #define SODIUM_NODE_MAX   ((1u << SODIUM_NODE_BITS) - 1u)

        union count_set_impl {
            small_count_set small;
            large_count_set* large;
        };
#endif

        /*!
         * Three counters implemented so as to fit into one machine word in the common case.
         */
        class count_set {
            private:
                // disable copy constructor and assignment
                count_set(const count_set&)
#if !defined(SODIUM_CONSERVE_MEMORY)
                : impl(0,0,0)
#endif
                {}
                count_set& operator = (const count_set&) { return *this; }
#if defined(SODIUM_CONSERVE_MEMORY)
                count_set_impl impl;
                void to_large() {
                    impl.large = new large_count_set(impl.small.strong_count, impl.small.stream_count, impl.small.node_count);
                    assert(!impl.small.is_small);
                }
#else
                large_count_set impl;
#endif
            public:
#if defined(SODIUM_CONSERVE_MEMORY)
                count_set() {
                    impl.small.strong_count = 0;
                    impl.small.stream_count = 0;
                    impl.small.node_count = 0;
                    impl.small.is_small = 1;
                }
#else
                count_set() : impl(0,0,0) {}
#endif
                ~count_set() {
#if defined(SODIUM_CONSERVE_MEMORY)
                    if (!impl.small.is_small)
                        delete impl.large;
#endif
                }
                bool active() const {
#if defined(SODIUM_CONSERVE_MEMORY)
                    return impl.small.is_small ? impl.small.strong_count || (impl.small.node_count && impl.small.stream_count) :
                                               impl.large->strong_count || (impl.large->node_count && impl.large->stream_count);
#else
                    return impl.strong_count || (impl.node_count && impl.stream_count);
#endif
                }
                bool alive() const {
#if defined(SODIUM_CONSERVE_MEMORY)
                    return impl.small.is_small ? impl.small.strong_count || impl.small.node_count || impl.small.stream_count
                                         : impl.large->strong_count || impl.large->node_count || impl.large->stream_count;
#else
                    return impl.strong_count || impl.node_count || impl.stream_count;
#endif
                }
                void inc_strong() {
#if defined(SODIUM_CONSERVE_MEMORY)
                    if (impl.small.is_small) {
                        if (impl.small.strong_count == SODIUM_STRONG_MAX)
                            to_large();
                        else {
                            impl.small.strong_count++;
                            return;
                        }
                    }
                    impl.large->strong_count++;
#else
                    impl.strong_count++;
#endif
                }
                void dec_strong() {
#if defined(SODIUM_CONSERVE_MEMORY)
                    if (impl.small.is_small)
                        impl.small.strong_count--;
                    else
                        impl.large->strong_count--;
#else
                    impl.strong_count--;
#endif
                }
                void inc_stream() {
#if defined(SODIUM_CONSERVE_MEMORY)
                    if (impl.small.is_small) {
                        if (impl.small.stream_count == SODIUM_STREAM_MAX)
                            to_large();
                        else {
                            impl.small.stream_count++;
                            return;
                        }
                    }
                    impl.large->stream_count++;
#else
                    impl.stream_count++;
#endif
                }
                void dec_stream() {
#if defined(SODIUM_CONSERVE_MEMORY)
                    if (impl.small.is_small)
                        impl.small.stream_count--;
                    else
                        impl.large->stream_count--;
#else
                    impl.stream_count--;
#endif
                }
                void inc_node() {
#if defined(SODIUM_CONSERVE_MEMORY)
                    if (impl.small.is_small) {
                        if (impl.small.node_count == SODIUM_NODE_MAX)
                            to_large();
                        else {
                            impl.small.node_count++;
                            return;
                        }
                    }
                    impl.large->node_count++;
#else
                    impl.node_count++;
#endif
                }
                void dec_node() {
#if defined(SODIUM_CONSERVE_MEMORY)
                    if (impl.small.is_small)
                        impl.small.node_count--;
                    else
                        impl.large->node_count--;
#else
                    impl.node_count--;
#endif
                }
        };
    }
}
#endif


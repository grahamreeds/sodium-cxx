/**
 * Copyright (c) 2012-2014, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM_LIGHTPTR_HPP_
#define _SODIUM_LIGHTPTR_HPP_

#include <utility>

namespace sodium {
    template <typename A>
    void deleter(void* a0)
    {
        delete (A*)a0;
    }

    namespace impl {
        typedef void (*deleter)(void*);
        struct count {
            count(
                int c_,
                deleter del_
            ) : c(c_), del(del_) {}
            int c;
            deleter del;
        };
    };

    /*!
     * An untyped reference-counting smart pointer, thread-safe variant.
     */
    #define SODIUM_DECLARE_LIGHTPTR(name) \
        struct name { \
            static name DUMMY;  /* A null value that does not work, but can be used to */ \
                                /* satisfy the compiler for unusable private constructors */ \
            name(); \
            name(const name& other); \
            name(name&& other) : value(other.value), count(other.count) \
            { \
                other.value = nullptr; \
                other.count = nullptr; \
            } \
            template <typename A> static inline name create(const A& a) { \
                return name(new A(a), deleter<A>); \
            } \
            template <typename A> static inline name create(A&& a) { \
                return name(new A(std::move(a)), deleter<A>); \
            } \
            name(void* value, impl::deleter del); \
            ~name(); \
            name& operator = (const name& other); \
            void* value; \
            impl::count* count; \
         \
            template <typename A> inline A* cast_ptr(A*) {return (A*)value;} \
            template <typename A> inline const A* cast_ptr(A*) const {return (A*)value;} \
        };

    SODIUM_DECLARE_LIGHTPTR(light_ptr)        // Thread-safe variant
    SODIUM_DECLARE_LIGHTPTR(unsafe_light_ptr)  // Non-thread-safe variant
}

#endif


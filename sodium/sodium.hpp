/**
 * Copyright (c) 2012-2014, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM_SODIUM_HPP_
#define _SODIUM_SODIUM_HPP_

#include <sodium/light_ptr.hpp>
#include <sodium/transaction.hpp>
#include <functional>
#include <boost/optional.hpp>
#include <memory>
#include <list>
#include <set>
#if defined(SODIUM_NO_EXCEPTIONS)
#include <stdlib.h>
#else
#include <stdexcept>
#endif
#include <vector>

#define SODIUM_CONSTANT_OPTIMIZATION

#ifdef WIN32
//maybe replace this with c++14 attributes..
#define __attribute__(x)
#endif

// TO DO:
// the sample_lazy() mechanism is not correct yet. The lazy value needs to be
// fixed at the end of the transaction.

namespace sodium {

    template <typename A> class stream;
    template <typename A> class cell;
    template <typename A> class cell_sink;
    template <typename A> class cell_loop;
    template <typename A> class stream_loop;
    template <typename A> class stream_sink;
    template <typename A, typename Selector> class router;
    template <typename A, typename B>
    cell<B> apply(const cell<std::function<B(const A&)>>& bf, const cell<A>& ba);
    template <typename A>
    stream<A> filter_optional(const stream<boost::optional<A>>& input);
    template <typename A>
    stream<A> split(const stream<std::list<A>>& e);
    template <typename A>
    stream<A> switch_s(const cell<stream<A>>& bea);
    template <typename T>
    cell<typename T::time> clock(const T& t);

    template <typename A>
    class lazy {
    private:
        std::function<A()> f;
    public:
        lazy(const A& a) : f([a] () -> A { return a; }) {}
        lazy(const std::function<A()>& f_) : f(f_) {}
        A operator () () const { return f(); }

        template <typename Fn>
        lazy<typename std::result_of<Fn(A)>::type> map(const Fn& fn) const {
            typedef typename std::result_of<Fn(A)>::type B;
            const auto& a(*this);
            return lazy<B>([fn, a] () { return fn(a()); });
        }

        template <typename B, typename Fn>
        lazy<typename std::result_of<Fn(A,B)>::type> lift(const lazy<B>& b, const Fn& fn) const {
            typedef typename std::result_of<Fn(A,B)>::type C;
            const auto& a(*this);
            return lazy<C>([fn, a, b] () { return fn(a(), b()); });
        }

        template <typename B, typename C, typename Fn>
        lazy<typename std::result_of<Fn(A,B,C)>::type> lift(const lazy<B>& b, const lazy<C>& c, const Fn& fn) const {
            typedef typename std::result_of<Fn(A,B,C)>::type D;
            const auto& a(*this);
            return lazy<D>([fn, a, b, c] () { return fn(a(), b(), c()); });
        }

        template <typename B, typename C, typename D, typename Fn>
        lazy<typename std::result_of<Fn(A,B,C,D)>::type> lift(const lazy<B>& b, const lazy<C>& c, const lazy<D>& d, const Fn& fn) const {
            typedef typename std::result_of<Fn(A,B,C,D)>::type E;
            const auto& a(*this);
            return lazy<E>([fn, a, b, c, d] () { return fn(a(), b(), c(), d()); });
        }
    };

    namespace impl {

        class cell_;
        struct cell_impl;

        class stream_ {
        friend class cell_;
        template <typename A> friend class sodium::stream;
        template <typename A> friend class sodium::stream_loop;
        template <typename A> friend class sodium::cell;
        friend cell_ switch_c(transaction_impl* trans, const cell_& bba);
        friend std::shared_ptr<cell_impl> hold(transaction_impl* trans0, const light_ptr& initValue, const stream_& input);
        friend std::shared_ptr<cell_impl> hold_lazy(transaction_impl* trans0, const std::function<light_ptr()>& initValue, const stream_& input);
        template <typename A, typename B>
        friend cell<B> sodium::apply(const cell<std::function<B(const A&)>>& bf, const cell<A>& ba);
        friend cell_ apply(transaction_impl* trans0, const cell_& bf, const cell_& ba);
        friend stream_ map_(transaction_impl* trans, const std::function<light_ptr(const light_ptr&)>& f, const stream_& ev);
        friend cell_ map_(transaction_impl* trans,
            const std::function<light_ptr(const light_ptr&)>& f,
            const cell_& beh);
        friend stream_ switch_s(transaction_impl* trans, const cell_& bea);
        template <typename A>
        friend stream<A> sodium::split(const stream<std::list<A>>& e);
        friend stream_ filter_optional_(transaction_impl* trans, const stream_& input,
            const std::function<boost::optional<light_ptr>(const light_ptr&)>& f);
        template <typename A, typename Selector> friend class sodium::router;

        protected:
            boost::intrusive_ptr<listen_impl_func<H_STREAM> > p_listen_impl;

        public:
            stream_();
            stream_(boost::intrusive_ptr<listen_impl_func<H_STREAM>> p_listen_impl_)
                : p_listen_impl(std::move(p_listen_impl_)) {}

#if defined(SODIUM_CONSTANT_OPTIMIZATION)
            bool is_never() const { return !impl::alive(p_listen_impl); }
#endif

        protected:

            /*!
             * listen to streams.
             */
            std::function<void()>* listen_raw(
                        transaction_impl* trans0,
                        const std::shared_ptr<impl::node>& target,
                        std::function<void(const std::shared_ptr<impl::node>&, transaction_impl*, const light_ptr&)>* handle,
                        bool suppressEarlierFirings) const;

            /*!
             * This is far more efficient than add_cleanup because it modifies the stream
             * in place.
             */
            stream_ unsafe_add_cleanup(std::function<void()>* cleanup)
            {
                boost::intrusive_ptr<listen_impl_func<H_STRONG> > li(
                    reinterpret_cast<listen_impl_func<H_STRONG>*>(p_listen_impl.get()));
                if (cleanup != NULL) {
                    if (alive(li))
                        li->cleanups.push_front(cleanup);
                    else {
                        (*cleanup)();
                        delete cleanup;
                    }
                }
                return *this;
            }

            /*!
             * This is far more efficient than add_cleanup because it modifies the stream
             * in place.
             */
            stream_ unsafe_add_cleanup(std::function<void()>* cleanup1, std::function<void()>* cleanup2)
            {
                boost::intrusive_ptr<listen_impl_func<H_STRONG> > li(
                    reinterpret_cast<listen_impl_func<H_STRONG>*>(p_listen_impl.get()));
                if (cleanup1 != NULL) {
                    if (alive(li))
                        li->cleanups.push_front(cleanup1);
                    else {
                        (*cleanup1)();
                        delete cleanup1;
                    }
                }
                if (cleanup2 != NULL) {
                    if (alive(li))
                        li->cleanups.push_front(cleanup2);
                    else {
                        (*cleanup2)();
                        delete cleanup2;
                    }
                }
                return *this;
            }

            /*!
             * This is far more efficient than add_cleanup because it modifies the stream
             * in place.
             */
            stream_ unsafe_add_cleanup(std::function<void()>* cleanup1, std::function<void()>* cleanup2, std::function<void()>* cleanup3)
            {
                boost::intrusive_ptr<listen_impl_func<H_STRONG> > li(
                    reinterpret_cast<listen_impl_func<H_STRONG>*>(p_listen_impl.get()));
                if (cleanup1 != NULL) {
                    if (alive(li))
                        li->cleanups.push_front(cleanup1);
                    else {
                        (*cleanup1)();
                        delete cleanup1;
                    }
                }
                if (cleanup2 != NULL) {
                    if (alive(li))
                        li->cleanups.push_front(cleanup2);
                    else {
                        (*cleanup2)();
                        delete cleanup2;
                    }
                }
                if (cleanup3 != NULL) {
                    if (alive(li))
                        li->cleanups.push_front(cleanup3);
                    else {
                        (*cleanup3)();
                        delete cleanup3;
                    }
                }
                return *this;
            }

            /*!
             * Create a new stream that is like this stream but has an extra cleanup.
             */
            stream_ add_cleanup_(transaction_impl* trans, std::function<void()>* cleanup) const;
            cell_ hold_(transaction_impl* trans, const light_ptr& initA) const;
            cell_ hold_lazy_(transaction_impl* trans, const std::function<light_ptr()>& initA) const;
            stream_ once_(transaction_impl* trans) const;
            stream_ merge_(transaction_impl* trans, const stream_& other) const;
            stream_ coalesce_(transaction_impl* trans, const std::function<light_ptr(const light_ptr&, const light_ptr&)>& combine) const;
            stream_ last_firing_only_(transaction_impl* trans) const;
            stream_ snapshot_(transaction_impl* trans, const cell_& beh, const std::function<light_ptr(const light_ptr&, const light_ptr&)>& combine) const;
            stream_ filter_(transaction_impl* trans, const std::function<bool(const light_ptr&)>& pred) const;

            std::function<void()>* listen_impl(
                transaction_impl* trans,
                const std::shared_ptr<impl::node>& target,
                std::shared_ptr<holder> h,
                bool suppressEarlierFirings) const
            {
                boost::intrusive_ptr<listen_impl_func<H_STRONG> > li(
                    reinterpret_cast<listen_impl_func<H_STRONG>*>(p_listen_impl.get()));
                if (alive(li))
                    return (*li->func)(trans, target, h, suppressEarlierFirings);
                else
                    return NULL;
            }
        };
        #define SODIUM_DETYPE_FUNCTION1(A,B,f) \
                   [f] (const light_ptr& a) -> light_ptr { \
                        return light_ptr::create<B>(f(*a.cast_ptr<A>(NULL))); \
                   }
        stream_ map_(transaction_impl* trans, const std::function<light_ptr(const light_ptr&)>& f, const stream_& ca);

        /*!
         * Function to push a value into an stream
         */
        void send(const std::shared_ptr<node>& n, transaction_impl* trans, const light_ptr& ptr);

        /*!
         * Creates an stream, that values can be pushed into using impl::send(). 
         */
        std::tuple<
                stream_,
                std::shared_ptr<node>
            > unsafe_new_stream();

        struct cell_impl {
            cell_impl();
            cell_impl(
                const stream_& updates,
                const std::shared_ptr<cell_impl>& parent);
            virtual ~cell_impl();

            virtual const light_ptr& sample() const = 0;
            virtual const light_ptr& newValue() const = 0;

            stream_ updates;  // Having this here allows references to cell to keep the
                             // underlying stream's cleanups alive, and provides access to the
                             // underlying stream, for certain primitives.

            std::function<void()>* kill;
            std::shared_ptr<cell_impl> parent;

            std::function<std::function<void()>(transaction_impl*, const std::shared_ptr<node>&,
                             const std::function<void(transaction_impl*, const light_ptr&)>&)> listen_value_raw() const;
        };

        std::shared_ptr<cell_impl> hold(transaction_impl* trans0,
                            const light_ptr& initValue,
                            const stream_& input);
        std::shared_ptr<cell_impl> hold_lazy(transaction_impl* trans0,
                            const std::function<light_ptr()>& initValue,
                            const stream_& input);

        struct cell_impl_constant : cell_impl {
            cell_impl_constant(light_ptr k_) : k(std::move(k_)) {}
            light_ptr k;
            virtual const light_ptr& sample() const { return k; }
            virtual const light_ptr& newValue() const { return k; }
        };

        template <typename state_t>
        struct cell_impl_concrete : cell_impl {
            cell_impl_concrete(
                const stream_& updates_,
                const state_t& state_,
                const std::shared_ptr<cell_impl>& parent_)
            : cell_impl(updates_, parent_),
              state(state_)
            {
            }
            state_t state;

            virtual const light_ptr& sample() const { return state.sample(); }
            virtual const light_ptr& newValue() const { return state.newValue(); }
        };

        struct cell_impl_loop : cell_impl {
            cell_impl_loop(
                const stream_& updates_,
                const std::shared_ptr<std::shared_ptr<cell_impl> >& pLooped_,
                const std::shared_ptr<cell_impl>& parent_)
            : cell_impl(updates_, parent_),
              pLooped(pLooped_)
            {
            }
            std::shared_ptr<std::shared_ptr<cell_impl> > pLooped;

            void assertLooped() const {
                if (!*pLooped)
                    throw std::runtime_error("cell_loop sampled before it was looped");
            }

            virtual const light_ptr& sample() const { assertLooped(); return (*pLooped)->sample(); }
            virtual const light_ptr& newValue() const { assertLooped(); return (*pLooped)->newValue(); }
        };

        struct cell_state {
            cell_state(const light_ptr& initA) : current(initA) {}
            light_ptr current;
            boost::optional<light_ptr> update;
            const light_ptr& sample() const { return current; }
            const light_ptr& newValue() const { return update ? update.get() : current; }
            void finalize() {
                current = update.get();
                update = boost::optional<light_ptr>();
            }
        };

        struct cell_state_lazy {
            cell_state_lazy(const std::function<light_ptr()>& initA)
            : pInitA(new std::function<light_ptr()>(initA)) {}
            std::function<light_ptr()>* pInitA;
            boost::optional<light_ptr> current;
            boost::optional<light_ptr> update;
            const light_ptr& sample() const {
                if (!current) {
                    const_cast<cell_state_lazy*>(this)->current = boost::optional<light_ptr>((*pInitA)());
                    delete pInitA;
                    const_cast<cell_state_lazy*>(this)->pInitA = NULL;
                }
                return current.get();
            }
            const light_ptr& newValue() const { return update ? update.get() : sample(); }
            void finalize() {
                current = update;
                update = boost::optional<light_ptr>();
            }
        };

        class cell_ {
            friend impl::stream_ underlying_stream(const cell_& beh);
            public:
                cell_();
                cell_(cell_impl* impl);
                cell_(std::shared_ptr<cell_impl> impl);
                cell_(light_ptr a);
                std::shared_ptr<impl::cell_impl> impl;

#if defined(SODIUM_CONSTANT_OPTIMIZATION)
                /*!
                 * For optimization, if this cell is a constant, then return its value.
                 */
                boost::optional<light_ptr> get_constant_value() const;
#endif

                stream_ value_(transaction_impl* trans) const;
                const stream_& updates_() const { return impl->updates; }
        };

        cell_ map_(transaction_impl* trans, const std::function<light_ptr(const light_ptr&)>& f,
            const cell_& beh);
    }  // end namespace impl

    template <typename A>
    class stream;

    /*!
     * A like an stream, but it tracks the input stream's current value and causes it
     * always to be output once at the beginning for each listener.
     */
    template <typename A>
    class cell : protected impl::cell_ {
        template <typename AA> friend class stream;
        template <typename AA> friend class cell;
        template <typename AA> friend class cell_loop;
        template <typename AA, typename BB>
        friend cell<BB> apply(const cell<std::function<BB(const AA&)>>& bf, const cell<AA>& ba);
        template <typename AA>
        friend cell<AA> switch_c(const cell<cell<AA>>& bba);
        template <typename AA>
        friend stream<AA> switch_s(const cell<stream<AA>>& bea);
        template <typename TT>
        friend cell<typename TT::time> clock(const TT& t);
        private:
            cell(std::shared_ptr<impl::cell_impl> impl_)
                : impl::cell_(std::move(impl_))
            {
            }

        protected:
            cell() {}
            cell(impl::cell_ c) : impl::cell_(std::move(c)) {}

        public:
            /*!
             * Constant value.
             */
            cell(const A& a)
                : impl::cell_(light_ptr::create<A>(a))
            {
            }

            cell(A&& a)
                : impl::cell_(light_ptr::create<A>(std::move(a)))
            {
            }

            /*!
             * Sample the value of this cell.
             */
            A sample() const {
                transaction trans;
                A a = *impl->sample().template cast_ptr<A>(NULL);
                trans.close();
                return a;
            }

            lazy<A> sample_lazy() const {
                const std::shared_ptr<impl::cell_impl>& impl_(this->impl);
                return lazy<A>([impl_] () -> A {
                    transaction trans;
                    A a = *impl_->sample().template cast_ptr<A>(NULL);
                    trans.close();
                    return a;
                });
            }

            /*!
             * Returns a new cell with the specified cleanup added to it, such that
             * it will be executed when no copies of the new cell are referenced.
             */
            cell<A> add_cleanup(const std::function<void()>& cleanup) const {
                transaction trans;
                cell<A> ca = updates().add_cleanup(cleanup).hold(sample());
                trans.close();
                return ca;
            }

            /*!
             * Map a function over this cell to modify the output value.
             */
            template <typename Fn>
            cell<typename std::result_of<Fn(A)>::type> map(const Fn& f) const {
                typedef typename std::result_of<Fn(A)>::type B;
                transaction trans;
                auto ca = cell<B>(impl::map_(trans.impl(), SODIUM_DETYPE_FUNCTION1(A,B,f), *this));
                trans.close();
                return ca;
            }

            /*!
             * Lift a binary function into cells.
             */
            template <typename B, typename Fn>
            cell<typename std::result_of<Fn(A,B)>::type> lift(const cell<B>& bb, const Fn& f) const
            {
                typedef typename std::result_of<Fn(A,B)>::type C;
                std::function<std::function<C(const B&)>(const A&)> fa(
                    [f] (const A& a) -> std::function<C(const B&)> {
                        return [f, a] (const B& b) -> C { return f(a, b); };
                    }
                );
                transaction trans;
                auto ca = apply<B, C>(map(fa), bb);
                trans.close();
                return ca;
            }

            /*!
             * Lift a ternary function into cells.
             */
            template <typename B, typename C, typename Fn>
            cell<typename std::result_of<Fn(A,B,C)>::type> lift(
                const cell<B>& bb,
                const cell<C>& bc,
                const Fn& f
            ) const
            {
                typedef typename std::result_of<Fn(A,B,C)>::type D;
                std::function<std::function<std::function<D(const C&)>(const B&)>(const A&)> fa(
                    [f] (const A& a) -> std::function<std::function<D(const C&)>(const B&)> {
                        return [f, a] (const B& b) -> std::function<D(const C&)> {
                            return [f, a, b] (const C& c) -> D {
                                return f(a,b,c);
                            };
                        };
                    }
                );
                return apply(apply(map(fa), bb), bc);
            }

            /*!
             * Lift a quaternary function into cells.
             */
            template <typename B, typename C, typename D, typename Fn>
            cell<typename std::result_of<Fn(A,B,C,D)>::type> lift(
                const cell<B>& bb,
                const cell<C>& bc,
                const cell<D>& bd,
                const Fn& f
            ) const
            {
                typedef typename std::result_of<Fn(A,B,C,D)>::type E;
                std::function<std::function<std::function<std::function<E(const D&)>(const C&)>(const B&)>(const A&)> fa(
                    [f] (const A& a) -> std::function<std::function<std::function<E(const D&)>(const C&)>(const B&)> {
                        return [f, a] (const B& b) -> std::function<std::function<E(const D&)>(const C&)> {
                            return [f, a, b] (const C& c) -> std::function<E(const D&)> {
                                return [f, a, b, c] (const D& d) -> E {
                                    return f(a,b,c,d);
                                };
                            };
                        };
                    }
                );
                return apply(apply(apply(map(fa), bb), bc), bd);
            }

            /*!
             * Lift a 5-argument function into cells.
             */
            template <typename B, typename C, typename D, typename E, typename Fn>
            cell<typename std::result_of<Fn(A,B,C,D,E)>::type> lift(
                const cell<B>& bb,
                const cell<C>& bc,
                const cell<D>& bd,
                const cell<E>& be,
                const Fn& f
            ) const
            {
                typedef typename std::result_of<Fn(A,B,C,D,E)>::type F;
                std::function<std::function<std::function<std::function<std::function<F(const E&)>(const D&)>(const C&)>(const B&)>(const A&)> fa(
                    [f] (const A& a) -> std::function<std::function<std::function<std::function<F(const E&)>(const D&)>(const C&)>(const B&)> {
                        return [f, a] (const B& b) -> std::function<std::function<std::function<F(const E&)>(const D&)>(const C&)> {
                            return [f, a, b] (const C& c) -> std::function<std::function<F(const E&)>(const D&)> {
                                return [f, a, b, c] (const D& d) -> std::function<F(const E&)> {
                                    return [f, a, b, c, d] (const E& e) -> F {
                                        return f(a,b,c,d,e);
                                    };
                                };
                            };
                        };
                    }
                );
                return apply(apply(apply(apply(map(fa), bb), bc), bd), be);
            }
        
            /*!
             * Lift a 6-argument function into cells.
             */
            template <typename B, typename C, typename D, typename E, typename F, typename Fn>
            cell<typename std::result_of<Fn(A,B,C,D,E,F)>::type> lift(
                const cell<A>& ba,
                const cell<B>& bb,
                const cell<C>& bc,
                const cell<D>& bd,
                const cell<E>& be,
                const cell<F>& bf,
                const Fn& fn
            ) const
            {
                typedef typename std::result_of<Fn(A,B,C,D,E,F)>::type G;
                std::function<std::function<std::function<std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)>(const C&)>(const B&)>(const A&)> fa(
                    [fn] (const A& a) -> std::function<std::function<std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)>(const C&)>(const B&)> {
                        return [fn, a] (const B& b) -> std::function<std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)>(const C&)> {
                            return [fn, a, b] (const C& c) -> std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)> {
                                return [fn, a, b, c] (const D& d) -> std::function<std::function<G(const F&)>(const E&)> {
                                    return [fn, a, b, c, d] (const E& e) -> std::function<G(const F&)> {
                                        return [fn, a, b, c, d, e] (const F& f) -> G {
                                            return fn(a,b,c,d,e,f);
                                        };
                                    };
                                };
                            };
                        };
                    }
                );
                return apply(apply(apply(apply(apply(map(fa), bb), bc), bd), be), bf);
            }

            /*!
             * Lift a 7-argument function into cells.
             */
            template <typename B, typename C, typename D, typename E, typename F, typename G, typename Fn>
            cell<typename std::result_of<Fn(A,B,C,D,E,F,G)>::type> lift(
                const cell<A>& ba,
                const cell<B>& bb,
                const cell<C>& bc,
                const cell<D>& bd,
                const cell<E>& be,
                const cell<F>& bf,
                const cell<G>& bg,
                const Fn& fn
            ) const
            {
                typedef typename std::result_of<Fn(A,B,C,D,E,F,G)>::type H;
                std::function<std::function<std::function<std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)>(const C&)>(const B&)>(const A&)> fa(
                    [fn] (const A& a) -> std::function<std::function<std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)>(const C&)>(const B&)> {
                        return [fn, a] (const B& b) -> std::function<std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)>(const C&)> {
                            return [fn, a, b] (const C& c) -> std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)> {
                                return [fn, a, b, c] (const D& d) -> std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)> {
                                    return [fn, a, b, c, d] (const E& e) -> std::function<std::function<H(const G&)>(const F&)> {
                                        return [fn, a, b, c, d, e] (const F& f) -> std::function<H(const G&)> {
                                            return [fn, a, b, c, d, e, f] (const G& g) {
                                                return fn(a,b,c,d,e,f,g);
                                            };
                                        };
                                    };
                                };
                            };
                        };
                    }
                );
                return apply(apply(apply(apply(apply(apply(map(fa), bb), bc), bd), be), bf), bg);
            }

            /*!
             * Returns an stream giving the updates to a cell. If this cell was created
             * by a hold, then this gives you back an stream equivalent to the one that was held.
             */
            stream<A> updates() const {
                return stream<A>(impl->updates);
            }

            /*!
             * Returns an stream describing the value of a cell, where there's an initial stream
             * giving the current value.
             */
            stream<A> value() const {
                transaction trans;
                stream<A> sa = stream<A>(value_(trans.impl())).coalesce([] (const A&, const A& b) { return b; });
                trans.close();
                return sa;
            }

            /*!
             * Listen for updates to the value of this cell. This is the observer pattern. The
             * returned {@link Listener} has a {@link Listener#unlisten()} method to cause the
             * listener to be removed. This is an OPERATIONAL mechanism is for interfacing between
             * the world of I/O and for FRP.
             * @param action The handler to execute when there's a new value.
             *   You should make no assumptions about what thread you are called on, and the
             *   handler should not block. You are not allowed to use {@link CellSink#send(Object)}
             *   or {@link StreamSink#send(Object)} in the handler.
             *   An exception will be thrown, because you are not meant to use this to create
             *   your own primitives.
             */
            std::function<void()> listen(const std::function<void(const A&)>& handle) const {
                transaction trans;
                auto kill = stream<A>(value_(trans.impl())).coalesce([] (const A&, const A& b) { return b; }).listen(handle);
                trans.close();
                return kill;
            }

            /**
             * Transform a cell with a generalized state loop (a mealy machine). The function
             * is passed the input and the old state and returns the new state and output value.
             *
             * The supplied function should have the signature std::tuple<B, S>(A, S), where B
             * is the return cell's type, and S is the state type.
             */
            template <typename S, typename Fn>
            cell<typename std::tuple_element<0,typename std::result_of<Fn(A,S)>::type>::type> collect_lazy(
                const lazy<S>& initS,
                const Fn& f
            ) const
            {
                typedef typename std::tuple_element<0,typename std::result_of<Fn(A,S)>::type>::type B;
                transaction trans1;
                auto ea = updates().coalesce([] (const A&, const A& snd) -> A { return snd; });
                lazy<A> za_lazy = sample_lazy();
                std::function<std::tuple<B,S>()> zbs = [za_lazy, initS, f] () -> std::tuple<B,S> {
                    return f(za_lazy(), initS());
                };
                std::shared_ptr<lazy<S> > pState(new lazy<S>([zbs] () -> S {
                    return std::get<1>(zbs());
                }));
                std::tuple<impl::stream_,std::shared_ptr<impl::node> > p = impl::unsafe_new_stream();
                auto kill = updates().listen_raw(trans1.impl(), std::get<1>(p),
                    new std::function<void(const std::shared_ptr<impl::node>&, impl::transaction_impl*, const light_ptr&)>(
                        [pState, f] (const std::shared_ptr<impl::node>& target, impl::transaction_impl* trans2, const light_ptr& ptr) {
                            std::tuple<B,S> outsSt = f(*ptr.cast_ptr<A>(NULL), (*pState)());
                            const S& new_s = std::get<1>(outsSt);
                            *pState = lazy<S>(new_s);
                            send(target, trans2, light_ptr::create<B>(std::get<0>(outsSt)));
                        }), false);
                auto ca = stream<B>(std::get<0>(p).unsafe_add_cleanup(kill)).hold_lazy(
                    lazy<B>([zbs] () -> B {
                        return std::get<0>(zbs());
                    }
                ));
                trans1.close();
                return ca;
            }

            /**
             * Transform a cell with a generalized state loop (a mealy machine). The function
             * is passed the input and the old state and returns the new state and output value.
             *
             * The supplied function should have the signature std::tuple<B, S>(A, S), where B
             * is the return cell's type, and S is the state type.
             */
            template <typename S, typename Fn>
            cell<typename std::tuple_element<0,typename std::result_of<Fn(A,S)>::type>::type> collect(
                const S& initS,
                const Fn& f
            ) const
            {
                return collect_lazy<S, Fn>(lazy<S>(initS), f);
            }

    };  // end class cell

    template <typename A>
    class stream : protected impl::stream_ {
        template <typename AA> friend class stream;
        template <typename AA> friend class sodium::stream_sink;
        template <typename AA> friend class cell;
        template <typename AA> friend class cell_sink;
        template <typename AA> friend class cell_loop;
        template <typename AA> friend stream<AA> filter_optional(const stream<boost::optional<AA>>& input);
        template <typename AA> friend stream<AA> switch_s(const cell<stream<AA>>& bea);
        template <typename AA> friend stream<AA> split(const stream<std::list<AA>>& e);
        template <typename AA> friend class sodium::stream_loop;
        template <typename AA, typename Selector> friend class sodium::router;
        public:
            /*!
             * The 'never' stream (that never fires).
             */
            stream() {}
        protected:
            stream(const impl::stream_& ev) : impl::stream_(ev) {}
        public:
            /*!
             * High-level interface to obtain an stream's value.
             */
            std::function<void()> listen(const std::function<void(const A&)>& handle) const {
                transaction trans1;
                std::function<void()>* pKill = listen_raw(trans1.impl(),
                    std::shared_ptr<impl::node>(new impl::node(SODIUM_IMPL_RANK_T_MAX)),
                    new std::function<void(const std::shared_ptr<impl::node>&, impl::transaction_impl*, const light_ptr&)>(
                        [handle] (const std::shared_ptr<impl::node>&, impl::transaction_impl*, const light_ptr& ptr) {
                            handle(*ptr.cast_ptr<A>(NULL));
                        }), false);
                trans1.close();
                if (pKill != NULL) {
                    std::function<void()> kill(*pKill);
                    delete pKill;
                    return kill;
                }
                else
                    return [] () {};
            };

            /*!
             * A variant of listen that handles the first stream and then
             * automatically deregisters itself. This is useful for implementing things that
             * work like promises.
             */
            std::function<void()> listen_once(const std::function<void(const A&)>& handle) const {
                std::shared_ptr<std::function<void()>> lRef(new std::function<void()>);
                *lRef = listen([handle, lRef] (const A& a) {
                    handle(a);
                    (*lRef)();
                });
                return *lRef;
            }

            /*!
             * Map a function over this stream to modify the output value. The function must be
             * pure (referentially transparent), that is, it must not have effects.
             */
            template <typename Fn>
            stream<typename std::result_of<Fn(A)>::type> map(const Fn& f) const {
                typedef typename std::result_of<Fn(A)>::type B;
                transaction trans;
                auto sa = stream<B>(impl::map_(trans.impl(), SODIUM_DETYPE_FUNCTION1(A,B,f), *this));
                trans.close();
                return sa;
            }

            /*!
             * map() to an optional and only let present values through. Equivalent to map()
             * followed by filter_optional().
             */
            template <typename Fn>
            stream<typename std::remove_reference<decltype(std::declval<typename std::result_of<Fn(A)>::type>().get())>::type> map_optional(const Fn& f) const {
                using B = typename std::remove_reference<decltype(std::declval<typename std::result_of<Fn(A)>::type>().get())>::type;
                transaction trans;
                auto sa = filter_optional<B>(map(f));
                trans.close();
                return sa;
            }

            /*!
             * Map a function over this stream that always outputs a constant value.
             */
            template <typename B>
            stream<B> map_to(const B& value) const {
                return map([value] (const A&) -> B { return value; });
            }

            /*!
             * Variant of merge that merges two streams and will drop an stream
             * in the simultaneous case.
             * <p>
             * In the case where two streams are simultaneous (i.e. both
             * within the same transaction), the stream from <em>this</em> will take precedence, and
             * the stream from <em>s</em> will be dropped.
             * If you want to specify your own combining function, use {@link Stream#merge(Stream, Lambda2)}.
             * s1.orElse(s2) is equivalent to s1.merge(s2, (l, r) -&gt; l).
             * <p>
             * The name orElse() is used instead of merge() to make it really clear that care should
             * be taken, because streams can be dropped.
             */
            stream<A> or_else(const stream<A>& s) const {
                return merge(s, [] (const A& l, const A& r) { return l; });
            }

            /*!
             * Merge a stream of unary functions with a signature like Arg(Arg), in the
             * simultaneous case, composing them, with the function on the left going first.
             */
            template <typename Arg>
            stream<A> merge_functions(const stream<A>& s) const {
                return merge(s, [] (const A& f, const A& g) -> A {
                    return [f, g] (Arg a) -> Arg {
                        return g(f(a));
                    };
                });
            }

        private:
            /*!
             * If there's more than one firing in a single transaction, combine them into
             * one using the specified combining function.
             *
             * If the stream firings are ordered, then the first will appear at the left
             * input of the combining function. In most common cases it's best not to
             * make any assumptions about the ordering, and the combining function would
             * ideally be commutative.
             */
            stream<A> coalesce(const std::function<A(const A&, const A&)>& combine) const
            {
                transaction trans;
                stream<A> sa(coalesce_(trans.impl(),
                    [combine] (const light_ptr& a, const light_ptr& b) -> light_ptr {
                        return light_ptr::create<A>(combine(*a.cast_ptr<A>(NULL), *b.cast_ptr<A>(NULL)));
                    }
                ));
                trans.close();
                return sa;
            }

        public:
            /*!
             * A variant of {@link merge(Stream)} that uses the specified function to combine simultaneous
             * streams.
             * <p>
             * If the streams are simultaneous (that is, one stream from this and one from <em>s</em>
             * occurring in the same transaction), combine them into one using the specified combining function
             * so that the returned stream is guaranteed only ever to have one stream per transaction.
             * The stream from <em>this</em> will appear at the left input of the combining function, and
             * the stream from <em>s</em> will appear at the right.
             * @param f Function to combine the values. It may construct FRP logic or use
             *    {@link Cell#sample()}. Apart from this the function must be <em>referentially transparent</em>.
             */
            stream<A> merge(const stream<A>& s, const std::function<A(const A&, const A&)>& f) const
            {
                transaction trans;
                stream<A> sa = stream<A>(merge_(trans.impl(), s)).coalesce(f);
                trans.close();
                return sa;
            }

            /*!
             * Filter this stream based on the specified predicate, passing through values
             * where the predicate returns true.
             */
            stream<A> filter(const std::function<bool(const A&)>& pred) const
            {
                transaction trans;
                stream<A> sa = stream<A>(filter_(trans.impl(),
                    [pred] (const light_ptr& a) {
                        return pred(*a.cast_ptr<A>(NULL));
                    }
                  ));
                trans.close();
                return sa;
            }

            /*!
             * Create a cell that holds at any given time the most recent value
             * that has arrived from this stream. Since cells must always have a current
             * value, you must supply an initial value that it has until the first stream
             * occurrence updates it.
             */
            cell<A> hold(const A& initA) const
            {
                transaction trans;
                cell<A> ca(hold_(trans.impl(), light_ptr::create<A>(initA)));
                trans.close();
                return ca;
            }

            cell<A> hold(A&& initA) const
            {
                transaction trans;
                cell<A> ca(hold_(trans.impl(), light_ptr::create<A>(std::move(initA))));
                trans.close();
                return ca;
            }

            cell<A> hold_lazy(const lazy<A>& initA) const
            {
                transaction trans;
                cell<A> ca(hold_lazy_(trans.impl(), [initA] () -> light_ptr { return light_ptr::create<A>(initA()); }));
                trans.close();
                return ca;
            }

            /*!
             * Sample the cell's value as at the transaction before the
             * current one, i.e. no changes from the current transaction are
             * taken.
             */
            template <typename B, typename Fn>
            stream<typename std::result_of<Fn(A,B)>::type> snapshot(const cell<B>& beh, const Fn& combine) const
            {
                typedef typename std::result_of<Fn(A,B)>::type C;
                transaction trans;
                auto sa = stream<C>(snapshot_(trans.impl(), beh,
                    [combine] (const light_ptr& a, const light_ptr& b) -> light_ptr {
                        return light_ptr::create<C>(combine(*a.cast_ptr<A>(NULL), *b.cast_ptr<B>(NULL)));
                    }
                ));
                trans.close();
                return sa;
            }

            template <typename B, typename C, typename Fn>
            stream<typename std::result_of<Fn(A,B,C)>::type> snapshot(
                const cell<B>& bc, const cell<C>& cc, const Fn& f) const
            {
                typedef typename std::result_of<Fn(A,B,C)>::type D;
                return snapshot(bc, [cc, f] (const A& a, const B& b) {
                    return f(a, b, cc.sample());
                });
            }

            template <typename B, typename C, typename D, typename Fn>
            stream<typename std::result_of<Fn(A,B,C,D)>::type> snapshot(
                const cell<B>& bc, const cell<C>& cc, const cell<D>& cd,
                const Fn& f) const
            {
                typedef typename std::result_of<Fn(A,B,C,D)>::type E;
                return snapshot(bc, [cc, cd, f] (const A& a, const B& b) {
                    return f(a, b, cc.sample(), cd.sample());
                });
            }

            template <typename B, typename C, typename D, typename E, typename Fn>
            stream<typename std::result_of<Fn(A,B,C,D,E)>::type> snapshot(
                const cell<B>& bc, const cell<C>& cc, const cell<D>& cd, const cell<E>& ce,
                const Fn& f) const
            {
                typedef typename std::result_of<Fn(A,B,C,D,E)>::type F;
                return snapshot(bc, [cc, cd, ce, f] (const A& a, const B& b) {
                    return f(a, b, cc.sample(), cd.sample(), ce.sample());
                });
            }

            template <typename B, typename C, typename D, typename E, typename F, typename Fn>
            stream<typename std::result_of<Fn(A,B,C,D,E,F)>::type> snapshot(
                const cell<B>& bc, const cell<C>& cc, const cell<D>& cd, const cell<E>& ce, const cell<F>& cf,
                const Fn& f) const
            {
                typedef typename std::result_of<Fn(A,B,C,D,E,F)>::type G;
                return snapshot(bc, [cc, cd, ce, cf, f] (const A& a, const B& b) {
                    return f(a, b, cc.sample(), cd.sample(), ce.sample(), cf.sample());
                });
            }

            /*!
             * Sample the cell's value as at the transaction before the
             * current one, i.e. no changes from the current transaction are
             * taken.
             */
            template <typename B>
            stream<B> snapshot(const cell<B>& beh) const
            {
                return snapshot(beh,
                    [] (const A&, const B& b) { return b; }
                    );
            }

            /*!
             * Allow streams through only when the cell's value is true.
             */
            stream<A> gate(const cell<bool>& g) const
            {
                transaction trans;
                stream<A> sa = filter_optional<A>(snapshot(
                    g,
                    [] (const A& a, const bool& gated) {
                        return gated ? boost::optional<A>(a) : boost::optional<A>();
                    }
                ));
                trans.close();
                return sa;
            }

            /*!
             * Adapt an stream to a new stream statefully.  Always outputs one output for each
             * input.
             *
             * The supplied function should have the signature std::tuple<B, S>(A, S), where B
             * is the return cell's type, and S is the state type.
             */
            template <typename S, typename Fn>
            stream<typename std::tuple_element<0,typename std::result_of<Fn(A,S)>::type>::type> collect_lazy(
                const lazy<S>& initS,
                const Fn& f
            ) const
            {
                typedef typename std::tuple_element<0,typename std::result_of<Fn(A,S)>::type>::type B;
                transaction trans1;
                std::shared_ptr<lazy<S> > pState(new lazy<S>(initS));
                std::tuple<impl::stream_,std::shared_ptr<impl::node> > p = impl::unsafe_new_stream();
                auto kill = listen_raw(trans1.impl(), std::get<1>(p),
                    new std::function<void(const std::shared_ptr<impl::node>&, impl::transaction_impl*, const light_ptr&)>(
                        [pState, f] (const std::shared_ptr<impl::node>& target, impl::transaction_impl* trans2, const light_ptr& ptr) {
                            auto outsSt = f(*ptr.cast_ptr<A>(NULL), (*pState)());
                            const S& new_s = std::get<1>(outsSt);
                            *pState = lazy<S>(new_s);
                            send(target, trans2, light_ptr::create<B>(std::get<0>(outsSt)));
                        }), false);
                auto sa = std::get<0>(p).unsafe_add_cleanup(kill);
                trans1.close();
                return sa;
            }

            /*!
             * Adapt an stream to a new stream statefully.  Always outputs one output for each
             * input.
             *
             * The supplied function should have the signature std::tuple<B, S>(A, S), where B
             * is the return cell's type, and S is the state type.
             */
            template <typename S, typename Fn>
            stream<typename std::tuple_element<0,typename std::result_of<Fn(A,S)>::type>::type> collect(
                const S& initS,
                const Fn& f
            ) const
            {
                return collect_lazy<S,Fn>(lazy<S>(initS), f);
            }

            template <typename B>
            stream<B> accum_s_lazy(
                const lazy<B>& initB,
                const std::function<B(const A&, const B&)>& f
            ) const
            {
                transaction trans1;
                std::shared_ptr<lazy<B> > pState(new lazy<B>(initB));
                std::tuple<impl::stream_,std::shared_ptr<impl::node> > p = impl::unsafe_new_stream();
                auto kill = listen_raw(trans1.impl(), std::get<1>(p),
                    new std::function<void(const std::shared_ptr<impl::node>&, impl::transaction_impl*, const light_ptr&)>(
                        [pState, f] (const std::shared_ptr<impl::node>& target, impl::transaction_impl* trans2, const light_ptr& ptr) {
                            B b = f(*ptr.cast_ptr<A>(NULL), (*pState)());
                            *pState = lazy<B>(b);
                            send(target, trans2, light_ptr::create<B>(b));
                        })
                    , false);
                stream<B> sb(std::get<0>(p).unsafe_add_cleanup(kill));
                trans1.close();
                return sb;
            }

            template <typename B>
            stream<B> accum_s(
                const B& initB,
                const std::function<B(const A&, const B&)>& f
            ) const
            {
                return accum_s_lazy<B>(lazy<B>(initB), f);
            }

            /*!
             * Renamed to accum_s.
             */
            template <typename B>
            stream<B> accum_e(
                const B& initB,
                const std::function<B(const A&, const B&)>& f
            ) const __attribute__ ((deprecated));

            template <typename B>
            cell<B> accum(
                const B& initB,
                const std::function<B(const A&, const B&)>& f
            ) const
            {
                return accum_s(initB, f).hold(initB);
            }

            /*!
             * Variant of accum_s that accumulates a stream of functions.
             */
            template<class B>
            stream<B> accum_s(const B& initA) const
            {
                return this->accum_s<B>(initA, [] (const A& f, const B& a) -> B {
                    return f(a);
                });
            }

            /*!
             * Variant of accum_s that accumulates a stream of functions - lazy variant.
             */
            template<class B>
            stream<B> accum_s_lazy(const lazy<B>& initA) const
            {
                return this->accum_s_lazy<B>(initA, [] (const A& f, const B& a) -> B {
                    return f(a);
                });
            }

            /*!
             * Variant of accum_s that accumulates a stream of functions.
             */
            template<class B>
            cell<B> accum(const B& initA) const
            {
                transaction trans;
                cell<B> cb = this->accum_s<B>(initA).hold(initA);
                trans.close();
                return cb;
            }

            /*!
             * Variant of accum_s that accumulates a stream of functions.
             */
            template<class B>
            cell<B> accum_lazy(const lazy<B>& initA) const
            {
                transaction trans;
                cell<B> cb = this->accum_s_lazy<B>(initA).hold_lazy(initA);
                trans.close();
                return cb;
            }

            template<class B = int>
            cell<B> count(const B& initial=0) const
            {
                return accum<B>(initial,
                    [] (const A&, const B& total) -> B { return total+1; }
                );
            }

            stream<A> once() const
            {
                transaction trans;
                stream<A> sa(once_(trans.impl()));
                trans.close();
                return sa;
            }

            /*!
             * Delays each stream occurrence by putting it into a new transaction, using
             * the same method as split.
             */
            stream<A> defer() const
            {
                return split<A>(map(
                        [] (const A& a) -> std::list<A> { return { a }; }
                    ));
            }

            /*!
             * Delays each stream occurrence by putting it into a new transaction, using
             * the same method as split.
             * Renamed to defer();
             */
            stream<A> delay() const __attribute__ ((deprecated))
            {
                return this->defer();
            }

            /*!
             * Add a clean-up operation to be performed when this stream is no longer
             * referenced.
             */
            stream<A> add_cleanup(const std::function<void()>& cleanup) const
            {
                transaction trans;
                stream<A> sa(add_cleanup_(trans.impl(),
                    new std::function<void()>(cleanup)
                ));
                trans.close();
                return sa;
            }
    };  // end class stream

    template <typename A>
    template <typename B>
    stream<B> stream<A>::accum_e(
        const B& initB,
        const std::function<B(const A&, const B&)>& f
    ) const
    {
        return accum_s<B>(initB, f);
    }

    namespace impl {
        struct stream_sink_impl {
            stream_sink_impl();
            stream_ construct();
            void send(transaction_impl* trans, const light_ptr& ptr) const;
            std::shared_ptr<impl::node> target;
        };
    }

    namespace impl {
        template <typename A, typename L>
        stream<A> merge(const L& sas, size_t start, size_t end, const std::function<A(const A&, const A&)>& f) {
            size_t len = end - start;
            if (len == 0) return stream<A>(); else
            if (len == 1) return sas[start]; else
            if (len == 2) return sas[start].merge(sas[start+1], f); else {
                int mid = (start + end) / 2;
                return merge<A,L>(sas, start, mid, f).merge(merge<A,L>(sas, mid, end, f), f);
            }
        }
    }

    /*!
     * Variant of merge that merges a collection of streams.
     */
    template <typename A, typename L>
    stream<A> or_else(const L& sas) {
        return impl::merge<A, L>(sas, 0, sas.size(), [] (const A& l, const A& r) { return l; });
    }

    /*!
     * Variant of merge that merges a collection of streams.
     */
    template <typename A, typename L>
    stream<A> merge(const L& sas, const std::function<A(const A&, const A&)>& f) {
        return impl::merge<A, L>(sas, 0, sas.size(), f);
    }

    /*!
     * An stream with a send() method to allow values to be pushed into it
     * from the imperative world.
     */
    template <typename A>
    class stream_sink : public stream<A>
    {
        private:
            impl::stream_sink_impl impl;
            stream_sink(const impl::stream_& e) : stream<A>(e) {}

        public:
            stream_sink()
            {
                *static_cast<stream<A>*>(this) = stream<A>(impl.construct()).coalesce(
                    [] (const A&, const A& b) { return b; });
            }

            stream_sink(const std::function<A(const A&, const A&)>& f) {
                *static_cast<stream<A>*>(this) = stream<A>(impl.construct()).coalesce(f);
            }

            void send(const A& a) const {
                transaction trans;
                if (trans.impl()->inCallback > 0)
                    throw std::runtime_error("You are not allowed to use send() inside a Sodium callback");
                impl.send(trans.impl(), light_ptr::create<A>(a));
                trans.close();
            }

            void send(A&& a) const {
                transaction trans;
                if (trans.impl()->inCallback > 0)
                    throw std::runtime_error("You are not allowed to use send() inside a Sodium callback");
                impl.send(trans.impl(), light_ptr::create<A>(std::move(a)));
                trans.close();
            }
    };

    namespace impl {
        stream_ filter_optional_(transaction_impl* trans, const stream_& input,
            const std::function<boost::optional<light_ptr>(const light_ptr&)>& f);
    }

    /*!
     * Filter an stream of optionals, keeping only the defined values.
     */
    template <typename A>
    stream<A> filter_optional(const stream<boost::optional<A>>& input)
    {
        transaction trans;
        stream<A> sa = impl::filter_optional_(trans.impl(), input, [] (const light_ptr& poa) -> boost::optional<light_ptr> {
            const boost::optional<A>& oa = *poa.cast_ptr<boost::optional<A>>(NULL);
            if (oa)
                return boost::optional<light_ptr>(light_ptr::create<A>(oa.get()));
            else
                return boost::optional<light_ptr>();
        });
        trans.close();
        return sa;
    }

    /*!
     * A cell with a send() method to allow its value to be changed
     * from the imperative world.
     */
    template <typename A>
    class cell_sink : public cell<A>
    {
        private:
            stream_sink<A> e;

            cell_sink(const cell<A>& beh) : cell<A>(beh) {}

        public:
            cell_sink(const A& initA)
            {
                transaction trans;
                this->impl = std::shared_ptr<impl::cell_impl>(hold(trans.impl(), light_ptr::create<A>(initA), e));
                trans.close();
            }

            cell_sink(A&& initA)
            {
                transaction trans;
                this->impl = std::shared_ptr<impl::cell_impl>(hold(trans.impl(), light_ptr::create<A>(std::move(initA)), e));
                trans.close();
            }

            void send(const A& a) const
            {
                e.send(a);
            }

            void send(A&& a) const
            {
                e.send(std::move(a));
            }
    };

    namespace impl {
        /*!
         * Returns an stream describing the changes in a cell.
         */
        inline impl::stream_ underlying_stream(const impl::cell_& beh) {return beh.impl->updates;}
    };

    namespace impl {
        cell_ apply(transaction_impl* trans, const cell_& bf, const cell_& ba);
    };

    /*!
     * Apply a function contained in a cell to a cell value. This is the primitive
     * for all lifting of functions into cells.
     */
    template <typename A, typename B>
    cell<B> apply(
        const cell<std::function<B(const A&)>>& bf,
        const cell<A>& ba)
    {
        transaction trans;
        cell<B> cb = cell<B>(impl::apply(
            trans.impl(),
            impl::map_(trans.impl(),
                [] (const light_ptr& pf) -> light_ptr {
                    const std::function<B(const A&)>& f = *pf.cast_ptr<std::function<B(const A&)>>(NULL);
                    return light_ptr::create<std::function<light_ptr(const light_ptr&)> >(
                            SODIUM_DETYPE_FUNCTION1(A, B, f)
                        );
                },
                bf),
            ba
        ));
        trans.close();
        return cb;
    }

    /*!
     * Enable the construction of stream loops, like this. This gives the ability to
     * forward reference an stream.
     *
     *   stream_loop<A> ea;
     *   auto ea_out = do_something(ea);
     *   ea.loop(ea_out);  // ea is now the same as ea_out
     *
     * TO DO: Loops do not yet get deallocated properly.
     */
    template <typename A>
    class stream_loop : public stream<A>
    {
        private:
            struct info {
                info(
                    const std::shared_ptr<std::function<void()>*>& pKill_
                )
                : pKill(pKill_), looped(false)
                {
                }
                std::shared_ptr<impl::node> target;
                std::shared_ptr<std::function<void()>*> pKill;
                bool looped;
            };
            std::shared_ptr<info> i;

        private:
            stream_loop(const impl::stream_& ev, const std::shared_ptr<info>& i_) : stream<A>(ev), i(i_) {}

        public:
            stream_loop()
            {
                std::shared_ptr<std::function<void()>*> pKill(
                    new std::function<void()>*(new std::function<void()>(
                        [] () {
                        }
                    ))
                );
                std::shared_ptr<info> i_(new info(pKill));

                std::tuple<impl::stream_,std::shared_ptr<impl::node> > p = impl::unsafe_new_stream();
                i_->target = std::get<1>(p);
                *this = stream_loop<A>(
                    std::get<0>(p).unsafe_add_cleanup(
                        new std::function<void()>(
                            [pKill] () {
                                std::function<void()>* kill = *pKill;
                                if (kill)
                                    (*kill)();
                                delete kill;
                            }
                        )
                    ),
                    i_
                );
            }

            void loop(const stream<A>& e)
            {
                if (!i->looped) {
                    transaction trans;
                    std::shared_ptr<impl::node> target(i->target);
                    *i->pKill = e.listen_raw(trans.impl(), target, NULL, false);
                    i->looped = true;
                    trans.close();
                }
                else {
#if defined(SODIUM_NO_EXCEPTIONS)
                    abort();
#else
                    throw std::runtime_error("stream_loop looped back more than once");
#endif
                }
            }
    };

    /*!
     * Enable the construction of cell loops, like this. This gives the ability to
     * forward reference a cell.
     *
     *   cell_loop<A> ba;
     *   auto ba_out = do_something(ea);
     *   ea.loop(ba_out);  // ba is now the same as ba_out
     *
     * TO DO: Loops do not yet get deallocated properly.
     */
    template <typename A>
    class cell_loop : public cell<A>
    {
        private:
            stream_loop<A> elp;
            std::shared_ptr<std::shared_ptr<impl::cell_impl> > pLooped;

        public:
            cell_loop()
                : cell<A>(impl::cell_()),
                  pLooped(new std::shared_ptr<impl::cell_impl>)
            {
                this->impl = std::shared_ptr<impl::cell_impl>(new impl::cell_impl_loop(
                    elp,
                    pLooped,
                    std::shared_ptr<impl::cell_impl>()));
            }

            void loop(const cell<A>& b)
            {
                elp.loop(b.updates());
                *pLooped = b.impl;
                // TO DO: This keeps the memory allocated in a loop. Figure out how to
                // break the loop.
                this->impl->parent = b.impl;
            }
    };

    namespace impl {
        stream_ switch_s(transaction_impl* trans, const cell_& bea);
    }

    /*!
     * Flatten a cell that contains an stream to give an stream that reflects
     * the current state of the cell. Note that when an stream is updated,
     * due to cell's delay semantics, stream occurrences for the new
     * stream won't come through until the following transaction.
     */
    template <typename A>
    stream<A> switch_s(const cell<stream<A>>& bea)
    {
        transaction trans;
        stream<A> sa(impl::switch_s(trans.impl(), bea));
        trans.close();
        return sa;
    }

    template <typename A>
    stream<A> switch_e(const cell<stream<A>>& bea) __attribute__ ((deprecated));

    /*!
     * Deprecated old name.
     */
    template <typename A>
    stream<A> switch_e(const cell<stream<A>>& bea)
    {
        return switch_s<A>(bea);
    }

    namespace impl {
        cell_ switch_c(transaction_impl* trans, const cell_& bba);
    }

    /*!
     * Cell variant of switch.
     */
    template <typename A>
    cell<A> switch_c(const cell<cell<A>>& bba)
    {
        transaction trans;
        cell<A> ca(impl::switch_c(trans.impl(), bba));
        trans.close();
        return ca;
    }

    template <typename A>
    cell<A> switch_b(const cell<cell<A>>& bba) __attribute__ ((deprecated));

    /*!
     * Cell variant of switch - deprecated old name.
     */
    template <typename A>
    cell<A> switch_b(const cell<cell<A>>& bba)
    {
        return switch_c<A>(bba);
    }

    template <typename A, typename B, typename C>
    cell<C> lift(const std::function<C(const A&, const B&)>& f, const cell<A>& ba, const cell<B>& bb)
     __attribute__ ((deprecated));

    /*!
     * Lift a binary function into cells.
     */
    template <typename A, typename B, typename C>
    cell<C> lift(const std::function<C(const A&, const B&)>& f, const cell<A>& ba, const cell<B>& bb)
    {
        std::function<std::function<C(const B&)>(const A&)> fa(
            [f] (const A& a) -> std::function<C(const B&)> {
                return [f, a] (const B& b) -> C { return f(a, b); };
            }
        );
        transaction trans;
        cell<C> cc = apply<B, C>(ba.map(fa), bb);
        trans.close();
        return cc;
    }

    template <typename A, typename B, typename C, typename D>
    cell<D> lift(const std::function<D(const A&, const B&, const C&)>& f,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc
    )  __attribute__ ((deprecated));

    /*!
     * Lift a ternary function into cells.
     */
    template <typename A, typename B, typename C, typename D>
    cell<D> lift(const std::function<D(const A&, const B&, const C&)>& f,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc
    )
    {
        std::function<std::function<std::function<D(const C&)>(const B&)>(const A&)> fa(
            [f] (const A& a) -> std::function<std::function<D(const C&)>(const B&)> {
                return [f, a] (const B& b) -> std::function<D(const C&)> {
                    return [f, a, b] (const C& c) -> D {
                        return f(a,b,c);
                    };
                };
            }
        );
        return apply(apply(ba.map(fa), bb), bc);
    }

    template <typename A, typename B, typename C, typename D, typename E>
    cell<E> lift(const std::function<E(const A&, const B&, const C&, const D&)>& f,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd
    ) __attribute__ ((deprecated));

    /*!
     * Lift a quaternary function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E>
    cell<E> lift(const std::function<E(const A&, const B&, const C&, const D&)>& f,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd
    )
    {
        std::function<std::function<std::function<std::function<E(const D&)>(const C&)>(const B&)>(const A&)> fa(
            [f] (const A& a) -> std::function<std::function<std::function<E(const D&)>(const C&)>(const B&)> {
                return [f, a] (const B& b) -> std::function<std::function<E(const D&)>(const C&)> {
                    return [f, a, b] (const C& c) -> std::function<E(const D&)> {
                        return [f, a, b, c] (const D& d) -> E {
                            return f(a,b,c,d);
                        };
                    };
                };
            }
        );
        return apply(apply(apply(ba.map(fa), bb), bc), bd);
    }

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    cell<F> lift(const std::function<F(const A&, const B&, const C&, const D&, const E&)>& f,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd,
        const cell<E>& be
    ) __attribute__ ((deprecated));

    /*!
     * Lift a 5-argument function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E, typename F>
    cell<F> lift(const std::function<F(const A&, const B&, const C&, const D&, const E&)>& f,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd,
        const cell<E>& be
    )
    {
        std::function<std::function<std::function<std::function<std::function<F(const E&)>(const D&)>(const C&)>(const B&)>(const A&)> fa(
            [f] (const A& a) -> std::function<std::function<std::function<std::function<F(const E&)>(const D&)>(const C&)>(const B&)> {
                return [f, a] (const B& b) -> std::function<std::function<std::function<F(const E&)>(const D&)>(const C&)> {
                    return [f, a, b] (const C& c) -> std::function<std::function<F(const E&)>(const D&)> {
                        return [f, a, b, c] (const D& d) -> std::function<F(const E&)> {
                            return [f, a, b, c, d] (const E& e) -> F {
                                return f(a,b,c,d,e);
                            };
                        };
                    };
                };
            }
        );
        return apply(apply(apply(apply(ba.map(fa), bb), bc), bd), be);
    }

    template <typename A, typename B, typename C, typename D, typename E, typename F, typename G>
    cell<G> lift(const std::function<G(const A&, const B&, const C&, const D&, const E&, const F&)>& fn,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd,
        const cell<E>& be,
        const cell<F>& bf
    ) __attribute__ ((deprecated));

    /*!
     * Lift a 6-argument function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E, typename F, typename G>
    cell<G> lift(const std::function<G(const A&, const B&, const C&, const D&, const E&, const F&)>& fn,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd,
        const cell<E>& be,
        const cell<F>& bf
    )
    {
        std::function<std::function<std::function<std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)>(const C&)>(const B&)>(const A&)> fa(
            [fn] (const A& a) -> std::function<std::function<std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)>(const C&)>(const B&)> {
                return [fn, a] (const B& b) -> std::function<std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)>(const C&)> {
                    return [fn, a, b] (const C& c) -> std::function<std::function<std::function<G(const F&)>(const E&)>(const D&)> {
                        return [fn, a, b, c] (const D& d) -> std::function<std::function<G(const F&)>(const E&)> {
                            return [fn, a, b, c, d] (const E& e) -> std::function<G(const F&)> {
                                return [fn, a, b, c, d, e] (const F& f) -> G {
                                    return fn(a,b,c,d,e,f);
                                };
                            };
                        };
                    };
                };
            }
        );
        return apply(apply(apply(apply(apply(ba.map(fa), bb), bc), bd), be), bf);
    }

    template <typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
    cell<H> lift(const std::function<H(const A&, const B&, const C&, const D&, const E&, const F&, const G&)>& fn,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd,
        const cell<E>& be,
        const cell<F>& bf,
        const cell<G>& bg
    ) __attribute__ ((deprecated));

    /*!
     * Lift a 7-argument function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
    cell<H> lift(const std::function<H(const A&, const B&, const C&, const D&, const E&, const F&, const G&)>& fn,
        const cell<A>& ba,
        const cell<B>& bb,
        const cell<C>& bc,
        const cell<D>& bd,
        const cell<E>& be,
        const cell<F>& bf,
        const cell<G>& bg
    )
    {
        std::function<std::function<std::function<std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)>(const C&)>(const B&)>(const A&)> fa(
            [fn] (const A& a) -> std::function<std::function<std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)>(const C&)>(const B&)> {
                return [fn, a] (const B& b) -> std::function<std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)>(const C&)> {
                    return [fn, a, b] (const C& c) -> std::function<std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)>(const D&)> {
                        return [fn, a, b, c] (const D& d) -> std::function<std::function<std::function<H(const G&)>(const F&)>(const E&)> {
                            return [fn, a, b, c, d] (const E& e) -> std::function<std::function<H(const G&)>(const F&)> {
                                return [fn, a, b, c, d, e] (const F& f) -> std::function<H(const G&)> {
                                    return [fn, a, b, c, d, e, f] (const G& g) {
                                        return fn(a,b,c,d,e,f,g);
                                    };
                                };
                            };
                        };
                    };
                };
            }
        );
        return apply(apply(apply(apply(apply(apply(ba.map(fa), bb), bc), bd), be), bf), bg);
    }

    /*!
     * Take each list item and put it into a new transaction of its own.
     *
     * An example use case of this might be a situation where we are splitting
     * a block of input data into frames. We obviously want each frame to have
     * its own transaction so that state is updated separately for each frame.
     */
    template <typename A>
    stream<A> split(const stream<std::list<A>>& e)
    {
        std::tuple<impl::stream_,std::shared_ptr<impl::node> > p = impl::unsafe_new_stream();
        transaction trans1;
        auto kill = e.listen_raw(trans1.impl(), std::get<1>(p),
            new std::function<void(const std::shared_ptr<impl::node>&, impl::transaction_impl*, const light_ptr&)>(
                [] (const std::shared_ptr<impl::node>& target, impl::transaction_impl* trans2, const light_ptr& ptr) {
                    const std::list<A>& la = *ptr.cast_ptr<std::list<A>>(NULL);
                    trans2->part->post([la, target] () {
                        for (auto it = la.begin(); it != la.end(); ++it) {
                            transaction trans3;
                            send(target, trans3.impl(), light_ptr::create<A>(*it));
                            trans3.close();
                        }
                    });
                })
            , false);
        stream<A> sa = std::get<0>(p).unsafe_add_cleanup(kill);
        trans1.close();
        return sa;
    }

    // New type names:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    template <typename A>
    struct event : stream<A> {
        event() {}
        event(const stream<A>& other) : stream<A>(other) {}
    } __attribute__ ((deprecated));
    template <typename A>
    struct event_sink : stream_sink<A> {
        event_sink() {}
        event_sink(const std::function<A(const A&, const A&)>& f) : stream_sink<A>(f) {}
        event_sink(const stream_sink<A>& other) : stream_sink<A>(other) {}
    } __attribute__ ((deprecated));
    template <typename A>
    struct event_loop : stream_loop<A> {
        event_loop() {}
        event_loop(const stream_loop<A>& other) : event_loop<A>(other) {}
    } __attribute__ ((deprecated));
    template <typename A>
    struct behavior : cell<A> {
        behavior(const A& initValue) : cell<A>(initValue) {}
        behavior(const cell<A>& other) : cell<A>(other) {}
    } __attribute__ ((deprecated));
    template <typename A>
    struct behavior_sink : cell_sink<A> {
        behavior_sink(const A& initValue) : cell_sink<A>(initValue) {}
    } __attribute__ ((deprecated));
    template <typename A>
    struct behavior_loop : cell_loop<A> {
        behavior_loop() {}
    } __attribute__ ((deprecated));
#pragma GCC diagnostic pop
}  // end namespace sodium
#endif


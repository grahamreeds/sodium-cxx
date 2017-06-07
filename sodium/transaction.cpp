/**
 * Copyright (c) 2012-2014, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#include <sodium/sodium.hpp>

using namespace std;
using namespace boost;

namespace sodium {

#if defined(SODIUM_SINGLE_THREADED)
    static impl::transaction_impl* global_current_transaction;
#else
    static thread_local impl::transaction_impl* global_current_transaction;
#endif

    namespace impl {

        partition* transaction_impl::part;

        void intrusive_ptr_add_ref(sodium::impl::listen_impl_func<sodium::impl::H_STREAM>* p)
        {
            spin_lock* l = spin_get_and_lock(p);
            p->counts.inc_stream();
            l->unlock();
        }

        void intrusive_ptr_release(sodium::impl::listen_impl_func<sodium::impl::H_STREAM>* p)
        {
            spin_lock* l = spin_get_and_lock(p);
            p->counts.dec_stream();
            p->update_and_unlock(l);
        }

        void intrusive_ptr_add_ref(sodium::impl::listen_impl_func<sodium::impl::H_STRONG>* p)
        {
            spin_lock* l = spin_get_and_lock(p);
            p->counts.inc_strong();
            l->unlock();
        }
        
        void intrusive_ptr_release(sodium::impl::listen_impl_func<sodium::impl::H_STRONG>* p)
        {
            spin_lock* l = spin_get_and_lock(p);
            p->counts.dec_strong();
            p->update_and_unlock(l);
        }

        void intrusive_ptr_add_ref(sodium::impl::listen_impl_func<sodium::impl::H_NODE>* p)
        {
            spin_lock* l = spin_get_and_lock(p);
            p->counts.inc_node();
            l->unlock();
        }

        void intrusive_ptr_release(sodium::impl::listen_impl_func<sodium::impl::H_NODE>* p)
        {
            spin_lock* l = spin_get_and_lock(p);
            p->counts.dec_node();
            p->update_and_unlock(l);
        }
        
        void holder::handle(const std::shared_ptr<node>& target, transaction_impl* trans, const light_ptr& value) const
        {
            if (handler)
                (*handler)(target, trans, value);
            else
                send(target, trans, value);
        }

    }

    partition::partition()
        : depth(0),
          processing_post(false),
          processing_on_start_hooks(false),
          shutting_down(false)
    {
    }
                            
    partition::~partition()
    {
        shutting_down = true;
        on_start_hooks.clear();
    }

    void partition::post(std::function<void()> action)
    {
        mx.lock();
        postQ.push_back(std::move(action));
        mx.unlock();
    }

    void partition::on_start(std::function<void()> action)
    {
        mx.lock();
        on_start_hooks.push_back(std::move(action));
        mx.unlock();
    }

    void partition::process_post()
    {
#if !defined(SODIUM_SINGLE_THREADED)
        // Prevent it running on multiple threads at the same time, so posts
        // will be handled in order for the partition.
        if (!processing_post) {
            processing_post = true;
#endif
#if !defined(SODIUM_NO_EXCEPTIONS)
            try {
#endif
                while (postQ.begin() != postQ.end()) {
                    std::function<void()> action = *postQ.begin();
                    postQ.erase(postQ.begin());
                    mx.unlock();
                    action();
                    mx.lock();
                }
                processing_post = false;
#if !defined(SODIUM_NO_EXCEPTIONS)
            }
            catch (...) {
                processing_post = false;
                throw;
            }
#endif
#if !defined(SODIUM_SINGLE_THREADED)
        }
#endif
    }

    namespace impl {

        node::node() : rank(0) {}
        node::node(rank_t rank_) : rank(rank_) {}
        node::~node()
        {
            for (std::forward_list<node::target>::iterator it = targets.begin(); it != targets.end(); it++) {
                std::shared_ptr<node> targ = it->n;
                if (targ) {
                    boost::intrusive_ptr<listen_impl_func<H_STREAM> > li(
                        reinterpret_cast<listen_impl_func<H_STREAM>*>(listen_impl.get()));
                    targ->sources.remove(li);
                }
            }
        }

        bool node::link(void* holder, const std::shared_ptr<node>& targ)
        {
            bool changed;
            if (targ) {
                std::set<node*> visited;
                changed = targ->ensure_bigger_than(visited, rank);
                boost::intrusive_ptr<listen_impl_func<H_STREAM> > li(
                    reinterpret_cast<listen_impl_func<H_STREAM>*>(listen_impl.get()));
                targ->sources.push_front(li);
            }
            else
                changed = false;
            targets.push_front(target(holder, targ));
            return changed;
        }

        void node::unlink(void* holder)
        {
            std::forward_list<node::target>::iterator this_it;
            for (std::forward_list<node::target>::iterator last_it = targets.before_begin(); true; last_it = this_it) {
                this_it = last_it;
                ++this_it;
                if (this_it == targets.end())
                    break;
                if (this_it->h == holder) {
                    std::shared_ptr<node> targ = this_it->n;
                    targets.erase_after(last_it);
                    if (targ) {
                        boost::intrusive_ptr<listen_impl_func<H_STREAM> > li(
                            reinterpret_cast<listen_impl_func<H_STREAM>*>(listen_impl.get()));
                        targ->sources.remove(li);
                    }
                    break;
                }
            }
        }

        void node::unlink_by_target(const std::shared_ptr<node>& targ)
        {
            std::forward_list<node::target>::iterator this_it;
            for (std::forward_list<node::target>::iterator last_it = targets.before_begin(); true; last_it = this_it) {
                this_it = last_it;
                ++this_it;
                if (this_it == targets.end())
                    break;
                if (this_it->n == targ) {
                    targets.erase_after(last_it);
                    if (targ) {
                        boost::intrusive_ptr<listen_impl_func<H_STREAM> > li(
                            reinterpret_cast<listen_impl_func<H_STREAM>*>(listen_impl.get()));
                        targ->sources.remove(li);
                    }
                    break;
                }
            }
        }

        bool node::ensure_bigger_than(std::set<node*>& visited, rank_t limit)
        {
            if (rank > limit || visited.find(this) != visited.end())
                return false;
            else {
                visited.insert(this);
                rank = limit + 1;
                for (std::forward_list<node::target>::iterator it = targets.begin(); it != targets.end(); ++it)
                    if (it->n)
                        it->n->ensure_bigger_than(visited, rank);
                return true;
            }
        }

        rank_t rankOf(const std::shared_ptr<node>& target)
        {
            if (target.get() != NULL)
                return target->rank;
            else
                return SODIUM_IMPL_RANK_T_MAX;
        }

        transaction_impl::transaction_impl()
            : to_regen(false),
              inCallback(0)
        {
            if (part == nullptr)
                part = new partition;
        }

        void transaction_impl::check_regen() {
            if (to_regen) {
                to_regen = false;
                prioritizedQ.clear();
                for (std::map<entryID, prioritized_entry>::iterator it = entries.begin(); it != entries.end(); ++it)
                    prioritizedQ.insert(pair<rank_t, entryID>(rankOf(it->second.target), it->first));
            }
        }

        transaction_impl::~transaction_impl()
        {
        }

        void transaction_impl::process_transactional()
        {
            while (true) {
                check_regen();
                std::multiset<pair<rank_t, entryID>>::iterator pit = prioritizedQ.begin();
                if (pit == prioritizedQ.end()) break;
                std::map<entryID, prioritized_entry>::iterator eit = entries.find(pit->second);
                assert(eit != entries.end());
                std::function<void(transaction_impl*)> action = eit->second.action;
                prioritizedQ.erase(pit);
                entries.erase(eit);
                action(this);
            }
            while (lastQ.begin() != lastQ.end()) {
                (*lastQ.begin())();
                lastQ.erase(lastQ.begin());
            }
        }

        void transaction_impl::prioritized(std::shared_ptr<node> target,
                                           std::function<void(transaction_impl*)> f)
        {
            entryID id = next_entry_id;
            next_entry_id = next_entry_id.succ();
            entries.insert(pair<entryID, prioritized_entry>(id, prioritized_entry(target, std::move(f))));
            prioritizedQ.insert(pair<rank_t, entryID>(rankOf(target), id));
        }

        void transaction_impl::last(const std::function<void()>& action)
        {
            lastQ.push_back(action);
        }

        transaction_::transaction_()
            : impl_(current_transaction())
        {
            if (impl_ == NULL) {
                if (transaction_impl::part == nullptr)
                    transaction_impl::part = new partition;
                transaction_impl::part->mx.lock();
                if (!transaction_impl::part->processing_on_start_hooks) {
                    transaction_impl::part->processing_on_start_hooks = true;
                    try {
                        if (!transaction_impl::part->shutting_down) {
                            for (auto it = transaction_impl::part->on_start_hooks.begin();
                                   it != transaction_impl::part->on_start_hooks.end(); ++it)
                                (*it)();
                        }
                        transaction_impl::part->processing_on_start_hooks = false;
                    }
                    catch (...) {
                        transaction_impl::part->processing_on_start_hooks = false;
                        throw;
                    }
                }
                impl_ = new transaction_impl;
                global_current_transaction = impl_;
            }
            transaction_impl::part->depth++;
        }
        
        transaction_::~transaction_()
        {
            close();
        }

        /*static*/ transaction_impl* transaction_::current_transaction()
        {
            return global_current_transaction;
        }

        void transaction_::close()
        {
            impl::transaction_impl* impl__(this->impl_);
            if (impl__) {
                this->impl_ = NULL;
                partition* part = transaction_impl::part;
                if (part->depth == 1) {
                    try {
                        impl__->process_transactional();
                        part->depth--;
                        global_current_transaction = NULL;
                        delete impl__;
                    }
                    catch (...) {
                        part->depth--;
                        global_current_transaction = NULL;
                        delete impl__;
                        part->mx.unlock();
                        throw;
                    }
                    part->process_post();
                    part->mx.unlock();
                }
                else
                    part->depth--;
            }
        }
    };  // end namespace impl

};  // end namespace sodium

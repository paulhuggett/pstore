//===- lib/core/hamt_map_types.cpp ----------------------------------------===//
//*  _                     _                             _                          *
//* | |__   __ _ _ __ ___ | |_   _ __ ___   __ _ _ __   | |_ _   _ _ __   ___  ___  *
//* | '_ \ / _` | '_ ` _ \| __| | '_ ` _ \ / _` | '_ \  | __| | | | '_ \ / _ \/ __| *
//* | | | | (_| | | | | | | |_  | | | | | | (_| | |_) | | |_| |_| | |_) |  __/\__ \ *
//* |_| |_|\__,_|_| |_| |_|\__| |_| |_| |_|\__,_| .__/   \__|\__, | .__/ \___||___/ *
//*                                             |_|          |___/|_|               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file hamt_map_types.cpp
#include "pstore/core/hamt_map_types.hpp"

#include <new>

namespace pstore {
  namespace index {
    namespace details {

      //*  _ _                                  _      *
      //* | (_)_ _  ___ __ _ _ _   _ _  ___  __| |___  *
      //* | | | ' \/ -_) _` | '_| | ' \/ _ \/ _` / -_) *
      //* |_|_|_||_\___\__,_|_|   |_||_\___/\__,_\___| *
      //*                                              *

      linear_node::signature_type const linear_node::node_signature_ = {
        {'I', 'n', 'd', 'x', 'L', 'n', 'e', 'r'}};

      // operator new
      // ~~~~~~~~~~~~
      void * linear_node::operator new (std::size_t const s, nchildren const size) {
        (void) s;
        std::size_t const actual_bytes = linear_node::size_bytes (size.n);
        PSTORE_ASSERT (actual_bytes >= s);
        return ::operator new (actual_bytes);
      }

      // operator delete
      // ~~~~~~~~~~~~~~~
      void linear_node::operator delete (void * const p, nchildren const /*size*/) {
        ::operator delete (p);
      }
      void linear_node::operator delete (void * const p) {
        ::operator delete (p);
      }

      // (ctor)
      // ~~~~~~
      linear_node::linear_node (std::size_t const size)
              : size_{size} {

        static_assert (std::is_standard_layout<linear_node>::value,
                       "linear_node must be standard-layout");
        static_assert (alignof (linear_node) >= 4,
                       "linear_node must have alignment >= 4 to ensure the bottom two bits are 0");

        static_assert (offsetof (linear_node, signature_) == 0,
                       "offsetof (linear_node, signature_) must be 0");
        static_assert (offsetof (linear_node, size_) == 8,
                       "offsetof (linear_node, size_) must be 8");
        static_assert (offsetof (linear_node, leaves_) == 16,
                       "offset of the first child must be 16");

        std::fill_n (&leaves_[0], size, address::null ());
      }

      linear_node::linear_node (linear_node const & rhs)
              : size_{rhs.size ()} {

        std::copy (rhs.begin (), rhs.end (), &leaves_[0]);
      }

      // allocate
      // ~~~~~~~~
      std::unique_ptr<linear_node> linear_node::allocate (std::size_t const num_children,
                                                          linear_node const & from_node) {
        // Allocate the new node and fill in the basic fields.
        auto new_node =
          std::unique_ptr<linear_node> (new (nchildren{num_children}) linear_node (num_children));

        std::size_t const num_to_copy = std::min (num_children, from_node.size ());
        auto const * const src_begin = from_node.leaves_;
        // Note that the last argument is '&leaves[0]' rather than just 'leaves' to defeat
        // an MSVC debug assertion that thinks it knows how big the leaves_ array is...
        std::copy (src_begin, src_begin + num_to_copy, &new_node->leaves_[0]);

        return new_node;
      }

      std::unique_ptr<linear_node> linear_node::allocate (address const a, address const b) {
        auto result = std::unique_ptr<linear_node> (new (nchildren{2U}) linear_node (2U));
        (*result)[0] = a;
        (*result)[1] = b;
        return result;
      }

      // allocate_from
      // ~~~~~~~~~~~~~
      std::unique_ptr<linear_node> linear_node::allocate_from (linear_node const & orig_node,
                                                               std::size_t const extra_children) {
        return linear_node::allocate (orig_node.size () + extra_children, orig_node);
      }

      std::unique_ptr<linear_node> linear_node::allocate_from (database const & db,
                                                               index_pointer const node,
                                                               std::size_t const extra_children) {
        std::pair<std::shared_ptr<linear_node const>, linear_node const *> const p =
          linear_node::get_node (db, node);
        PSTORE_ASSERT (p.second != nullptr);
        return linear_node::allocate_from (*p.second, extra_children);
      }

      // get_node
      // ~~~~~~~~
      auto linear_node::get_node (database const & db, index_pointer const node)
        -> std::pair<std::shared_ptr<linear_node const>, linear_node const *> {

        if (node.is_heap ()) {
          auto const * ptr = node.untag<linear_node const *> ();
          PSTORE_ASSERT (ptr->signature_ == node_signature_);
          return {nullptr, ptr};
        }

        // Read an existing node. First work out its size.
        auto const addr = node.untag_address<linear_node> ();
        std::size_t const in_store_size = linear_node::size_bytes (db.getro (addr)->size ());

        // Now access the data block for this linear node. We need to use the "raw address"
        // version of getro() because in_store_size is a number of bytes, not a number of
        // instances of linear_node.
        auto const ln = std::static_pointer_cast<linear_node const> (
          db.getro (addr.to_address (), in_store_size));
#if PSTORE_SIGNATURE_CHECKS_ENABLED
        if (ln->signature_ != node_signature_) {
          raise (pstore::error_code::index_corrupt);
        }
#endif
        auto const * const p = ln.get ();
        return {std::move (ln), p};
      }

      // flush
      // ~~~~~
      address linear_node::flush (transaction_base & transaction) const {
        std::size_t const num_bytes = this->size_bytes ();

        std::shared_ptr<void> ptr;
        address result;
        std::tie (ptr, result) = transaction.alloc_rw (num_bytes, alignof (linear_node));
        new (ptr.get ()) linear_node (*this);
        return result;
      }

      //*  _                     _     *
      //* | |__ _ _ __ _ _ _  __| |_   *
      //* | '_ \ '_/ _` | ' \/ _| ' \  *
      //* |_.__/_| \__,_|_||_\__|_||_| *
      //*                              *
      branch::signature_type const branch::node_signature_ = {
        {'I', 'n', 't', 'e', 'r', 'n', 'a', 'l'}};

      // ctor (one child)
      // ~~~~~~~~~~~~~~~~
      branch::branch (index_pointer const & leaf, hash_type const hash)
              : bitmap_{hash_type{1} << hash}
              , children_{{leaf}} {

        static_assert (std::is_standard_layout_v<branch>,
                       "internal branch must be standard-layout");
        static_assert (alignof (linear_node) >= 4,
                       "branch must have alignment >= 4 to ensure the bottom two bits are 0");

        static_assert (offsetof (branch, signature_) == 0,
                       "offsetof (branch, signature) must be 0");
        static_assert (offsetof (branch, bitmap_) == 8, "offsetof (branch, bitmap_) must be 8");
        static_assert (offsetof (branch, children_) == 16, "offset of the first child must be 16.");
      }

      // ctor (two children)
      // ~~~~~~~~~~~~~~~~~~~
      branch::branch (index_pointer const & existing_leaf, index_pointer const & new_leaf,
                      hash_type const existing_hash, hash_type const new_hash)
              : bitmap_ (hash_type{1} << existing_hash | hash_type{1} << new_hash) {

        auto const index_a = get_new_index (new_hash, existing_hash);
        auto const index_b = static_cast<unsigned> (index_a == 0);

        PSTORE_ASSERT ((index_a & 1) == index_a); //! OCLINT(PH - bitwise in conditional is ok)
        PSTORE_ASSERT ((index_b & 1) == index_b); //! OCLINT(PH - bitwise in conditional is ok)
        PSTORE_ASSERT (index_a != index_b);

        children_[index_a] = index_pointer{new_leaf};
        children_[index_b] = existing_leaf;
      }

      // copy ctor
      // ~~~~~~~~~
      branch::branch (branch const & rhs)
              : bitmap_{rhs.bitmap_} {

        auto const * const first = std::begin (rhs.children_);
        std::copy (first, first + rhs.size (), std::begin (children_));
      }

      // validate after load
      // ~~~~~~~~~~~~~~~~~~~
      /// Perform crude validation of the branch that we've just read from the store.
      ///
      /// - We know that if this node is not on the heap then none of its immediate
      ///   children can be on the heap because creating a heap node causes all of its
      ///   parents to be recursively modified.
      /// - Child nodes (whether internal, linear, or leaf) will always be at a lower
      ///   store address than the parent node because they are written to the store
      ///   in depth-first order.
      /// - All child node addresses should be different.
      ///
      /// \param internal  The node to be validated.
      /// \param addr  The address from which this node was loaded.
      /// \return  A boolean indicating whether the validation was successful.
      bool branch::validate_after_load (branch const & internal, typed_address<branch> const addr) {
#if PSTORE_SIGNATURE_CHECKS_ENABLED
        if (internal.signature_ != node_signature_) {
          return false;
        }
#endif
        return std::all_of (std::begin (internal), std::end (internal),
                            [addr] (index_pointer const & child) {
                              if (child.is_heap () || child.untag_address<branch> () >= addr) {
                                return false;
                              }
                              return true;
                            });
      }

      // read node [static]
      // ~~~~~~~~~
      auto branch::read_node (database const & db, typed_address<branch> const addr)
        -> std::shared_ptr<branch const> {
        /// Sadly, loading a branch node needs to done in three stages:
        /// 1. Load the basic structure.
        /// 2. Calculate the actual size of the child pointer array.
        /// 3. Load the complete structure along with its child pointer array.
        auto base = std::static_pointer_cast<branch const> (
          db.getro (addr.to_address (), sizeof (branch) - sizeof (branch::children_)));

        if (base->get_bitmap () == 0) {
          raise (error_code::index_corrupt, db.path ());
        }
        std::size_t const actual_size = branch::size_bytes (base->size ());
        base.reset ();

        PSTORE_ASSERT (actual_size > sizeof (branch) - sizeof (branch::children_));
        auto resl =
          std::static_pointer_cast<branch const> (db.getro (addr.to_address (), actual_size));

        if (!validate_after_load (*resl, addr)) {
          raise (error_code::index_corrupt, db.path ());
        }

        return resl;
      }

      // get node [static]
      // ~~~~~~~~
      auto branch::get_node (database const & db, index_pointer const node)
        -> std::pair<std::shared_ptr<branch const>, branch const *> {

        if (node.is_heap ()) {
          return {nullptr, node.untag<branch *> ()};
        }

        std::shared_ptr<branch const> store_branch =
          branch::read_node (db, node.untag_address<branch> ());
        return {std::move (store_branch), store_branch.get ()};
      }

      // insert child
      // ~~~~~~~~~~~~
      void branch::insert_child (hash_type const hash, index_pointer const leaf,
                                 gsl::not_null<parent_stack *> const parents) {
        auto const hash_index = hash & details::hash_index_mask;
        auto const bit_pos = hash_type{1} << hash_index;
        PSTORE_ASSERT (bit_pos != 0); // guarantee that we didn't shift the bit to oblivion
        // check that this slot is free.
        PSTORE_ASSERT ((this->bitmap_ & bit_pos) == 0); //! OCLINT(PH - bitwise in conditional)

        // Compute the child index by counting the number of 1 bits below bit_pos.
        unsigned const index = bit_count::pop_count (this->bitmap_ & (bit_pos - 1));
        unsigned const old_size = bit_count::pop_count (this->bitmap_);
        PSTORE_ASSERT (old_size < hash_size);
        PSTORE_ASSERT (index <= old_size);

        // Move elements from [index..old_size) to [index+1..old_size+1)
        {
          auto const children_span = gsl::make_span (&children_[0], hash_size);
          auto const num_to_move = old_size - index;

          auto const span = children_span.subspan (index, num_to_move);
          auto const d_span = children_span.subspan (index + 1, num_to_move);
          PSTORE_ASSERT (d_span.last (0).data () <= children_span.last (0).data ());

          std::move_backward (std::begin (span), std::end (span), std::end (d_span));
        }

        children_[index] = leaf;

        this->bitmap_ = this->bitmap_ | bit_pos;
        PSTORE_ASSERT (bit_count::pop_count (this->bitmap_) == old_size + 1);
        parents->push (parent_type{index_pointer{this}, index});
      }

      // store node
      // ~~~~~~~~~~
      address branch::store_node (transaction_base & transaction) const {
        std::size_t const num_bytes = branch::size_bytes (this->size ());

        auto [ptr, addr] = transaction.alloc_rw (num_bytes, alignof (branch));
        new (ptr.get ()) branch (*this);
        return addr;
      }

      // flush
      // ~~~~~
      address branch::flush (transaction_base & transaction, unsigned shifts) {
        shifts += hash_index_bits;
        for (auto & p : *this) {
          // If it is a heap node, flush its children first (depth-first search).
          if (p.is_heap ()) {
            if (shifts < max_hash_bits) { // internal node
              PSTORE_ASSERT (p.is_branch ());
              auto * const internal = p.untag<branch *> ();
              p = internal->flush (transaction, shifts);
              // This node is owned by a container in the outer HAMT structure. Don't
              // delete it here. If this ever changes, then add a 'delete internal;'
              // here.
            } else { // linear node
              PSTORE_ASSERT (p.is_linear ());
              auto * const linear = p.untag<linear_node *> ();
              p = linear->flush (transaction) | branch_bit;
              delete linear;
            }
          }
        }
        // Flush itself.
        return this->store_node (transaction) | branch_bit;
      }

    } // namespace details
  }   // namespace index
} // namespace pstore

// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Holds state about hashes inserted or removed.
 * The hashdb_manager updates this information while performing actions,
 * then reports it using the logger.
 */

#ifndef HASHDB_CHANGES_HPP
#define HASHDB_CHANGES_HPP

#include <sstream>
#include <iostream>
#include "dfxml/src/dfxml_writer.h"

/**
 * The hashdb change logger holds all possible change values,
 * and is used for reporting changes to the database.
 */
class hashdb_changes_t {

  public:
  uint32_t hashes_inserted;
  uint32_t hashes_not_inserted_mismatched_hash_block_size;
  uint32_t hashes_not_inserted_invalid_byte_alignment;
  uint32_t hashes_not_inserted_exceeds_max_duplicates;
  uint32_t hashes_not_inserted_duplicate_element;

  uint32_t hashes_removed;
  uint32_t hashes_not_removed_mismatched_hash_block_size;
  uint32_t hashes_not_removed_invalid_byte_alignment;
  uint32_t hashes_not_removed_no_hash;
  uint32_t hashes_not_removed_no_element;

  hashdb_changes_t() :

                     hashes_inserted(0),
                     hashes_not_inserted_mismatched_hash_block_size(0),
                     hashes_not_inserted_invalid_byte_alignment(0),
                     hashes_not_inserted_exceeds_max_duplicates(0),
                     hashes_not_inserted_duplicate_element(0),

                     hashes_removed(0),
                     hashes_not_removed_mismatched_hash_block_size(0),
                     hashes_not_removed_invalid_byte_alignment(0),
                     hashes_not_removed_no_hash(0),
                     hashes_not_removed_no_element(0) {
  }

  void report_changes(dfxml_writer& x) const {

    // log any insert changes to x
    x.push("hashdb_changes");

    if (hashes_inserted)
      x.xmlout("hashes_inserted", hashes_inserted);
    if (hashes_not_inserted_mismatched_hash_block_size)
      x.xmlout("hashes_not_inserted_mismatched_hash_block_size", hashes_not_inserted_mismatched_hash_block_size);
    if (hashes_not_inserted_invalid_byte_alignment)
      x.xmlout("hashes_not_inserted_invalid_byte_alignment", hashes_not_inserted_invalid_byte_alignment);
    if (hashes_not_inserted_exceeds_max_duplicates)
      x.xmlout("hashes_not_inserted_exceeds_max_duplicates", hashes_not_inserted_exceeds_max_duplicates);
    if (hashes_not_inserted_duplicate_element)
      x.xmlout("hashes_not_inserted_duplicate_element", hashes_not_inserted_duplicate_element);

    // log any remove changes to x
    if (hashes_removed)
      x.xmlout("hashes_removed", hashes_removed);
    if (hashes_not_removed_mismatched_hash_block_size)
      x.xmlout("hashes_not_removed_mismatched_hash_block_size", hashes_not_removed_mismatched_hash_block_size);
    if (hashes_not_removed_invalid_byte_alignment)
      x.xmlout("hashes_not_removed_invalid_byte_alignment", hashes_not_removed_invalid_byte_alignment);
    if (hashes_not_removed_no_hash)
      x.xmlout("hashes_not_removed_no_hash", hashes_not_removed_no_hash);
    if (hashes_not_removed_no_element)
      x.xmlout("hashes_not_removed_no_element", hashes_not_removed_no_element);

    x.pop();
  }

  void report_changes(std::ostream& os) const {
    bool has_insert_action = (hashes_inserted ||
                              hashes_not_inserted_mismatched_hash_block_size ||
                              hashes_not_inserted_invalid_byte_alignment ||
                              hashes_not_inserted_exceeds_max_duplicates ||
                              hashes_not_inserted_duplicate_element);

    bool has_remove_action = (hashes_removed ||
                              hashes_not_removed_mismatched_hash_block_size ||
                              hashes_not_removed_invalid_byte_alignment ||
                              hashes_not_removed_no_hash ||
                              hashes_not_removed_no_element); 

    if (!has_insert_action && !has_remove_action) {
      std::cout << "No hashdb changes.\n";
    }

    if (has_insert_action) {
      // log any insert changes to stdout
      std::cout << "hashdb changes (insert):\n";
      if (hashes_inserted)
       std::cout << "    hashes inserted=" << hashes_inserted << "\n";
      if (hashes_not_inserted_mismatched_hash_block_size)
       std::cout << "    hashes not inserted, mismatched hash block size=" << hashes_not_inserted_mismatched_hash_block_size << "\n";
      if (hashes_not_inserted_invalid_byte_alignment)
       std::cout << "    hashes not inserted, invalid byte alignment=" << hashes_not_inserted_invalid_byte_alignment << "\n";
      if (hashes_not_inserted_exceeds_max_duplicates)
       std::cout << "    hashes not inserted, exceeds max duplicates=" << hashes_not_inserted_exceeds_max_duplicates << "\n";
      if (hashes_not_inserted_duplicate_element)
       std::cout << "    hashes not inserted, duplicate element=" << hashes_not_inserted_duplicate_element << "\n";
    }

    if (has_remove_action) {
      // log any remove changes to stdout
        std::cout << "hashdb changes (remove):\n";
      if (hashes_removed)
        std::cout << "    hashes removed=" << hashes_removed << "\n";
      if (hashes_not_removed_mismatched_hash_block_size)
        std::cout << "    hashes not removed, mismatched hash block size=" << hashes_not_removed_mismatched_hash_block_size << "\n";
      if (hashes_not_removed_invalid_byte_alignment)
        std::cout << "    hashes not removed, invalid byte alignment=" << hashes_not_removed_invalid_byte_alignment << "\n";
      if (hashes_not_removed_no_hash)
        std::cout << "    hashes not removed, no hash=" << hashes_not_removed_no_hash << "\n";
      if (hashes_not_removed_no_element)
        std::cout << "    hashes not removed, no element=" << hashes_not_removed_no_element << "\n";
    }
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hashdb_changes_t& changes) {
  changes.report_changes(os);
  return os;
}

#endif


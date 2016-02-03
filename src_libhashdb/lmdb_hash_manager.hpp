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
 * Manage the LMDB hash store.  Threadsafe.
 */

#ifndef LMDB_HASH_MANAGER_HPP
#define LMDB_HASH_MANAGER_HPP
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <set>
#ifdef DEBUG
#include "to_hex.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

static uint8_t masks[8] = {0xff,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe};

class lmdb_hash_manager_t {

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const int prefix_bytes;
  const uint8_t prefix_mask;
  const int suffix_bytes;
  std::set<std::string>* suffix_strings;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_hash_manager_t(const lmdb_hash_manager_t&);
  lmdb_hash_manager_t& operator=(const lmdb_hash_manager_t&);

  // encode strings
  std::string encode_data(const std::set<std::string> strings) const {

    // allocate space for the encoding, this can break if
    // it is faster to be liberal than to be exact,
    // so permit up to 15-byte size suffixes
    size_t max_size = strings.size() * 16;

    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    for (std::set<std::string>::const_iterator it = strings.begin();
                                               it != strings.end(); ++it) {
      p = lmdb_helper::encode_string(*it, p);
    }

#ifdef DEBUG
    std::string encoding_string(reinterpret_cast<char*>(encoding), (p-encoding));
    std::cout << "encoding " << strings.size()
              << " of data\n";
    for (int i=0; i<strings.size(); ++i) {
      std::cout << "string " << i
                << ": " << hashdb::to_hex(encoding_string) << "\n";
    }



    std::cout << "to binary data "
              << hashdb::to_hex(encoding_string)
              << " size " << encoding_string.size() << "\n";
#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decode strings
  void decode_data(const std::string& encoding,
                   std::set<std::string>& strings) const {
    strings.clear();
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p_stop = p_start + encoding.size();
    const uint8_t* p = p_start;
    std::string s;
    while(p < p_stop) {
      p = lmdb_helper::decode_string(p, s);
      strings.insert(s);
    }

#ifdef DEBUG
    std::string hex_encoding = hashdb::to_hex(encoding);
    std::cout << "decoding strings data " << hex_encoding
              << " size " << encoding.size() << "\n"
              << " to lmdb_source_data array size " << strings.size() << "\n";
#endif

    // validate that the decoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }
  }

  inline std::pair<std::string, std::string> hash_pair(
                              const std::string& binary_hash) const {
    int hash_size = binary_hash.size();

    // prefix string
    int prefix_size = (hash_size > prefix_bytes) ? prefix_bytes : hash_size;
    std::string prefix_string = binary_hash.substr(0, prefix_size);

    // maybe zero out some bits at the end of the prefix string
    if (prefix_size == prefix_bytes) {
      prefix_string[prefix_bytes-1] &= prefix_mask;
    }

    // suffix string
    int suffix_start = hash_size - suffix_bytes;
    if (suffix_start < prefix_size) {
      suffix_start = prefix_size;
    }
    std::string suffix_string = (suffix_start < hash_size) ?
                      binary_hash.substr(suffix_start, suffix_bytes) : "";
#ifdef DEBUG
    std::cout << "binary hash '" << hashdb::to_hex(binary_hash)
              << "' to hash prefix '" << hashdb::to_hex(prefix_string)
              << "', hash suffix '" << hashdb::to_hex(suffix_string)
              << "'\n";
#endif

    return std::pair<std::string, std::string>(prefix_string, suffix_string);
  }

  public:
  lmdb_hash_manager_t(const std::string& p_hashdb_dir,
                      const file_mode_type_t p_file_mode,
                      const uint32_t p_hash_prefix_bits,
                      const uint32_t p_hash_suffix_bytes) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       prefix_bytes((p_hash_prefix_bits + 7) / 8),
       prefix_mask(masks[p_hash_prefix_bits % 8]),
       suffix_bytes(p_hash_suffix_bytes),
       suffix_strings(new(std::set<std::string>)),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_store", file_mode)),
       M() {

    MUTEX_INIT(&M);

    // require valid parameters
    if (prefix_bytes == 0) {
      std::cerr << "invalid hash store configuration\n";
      assert(0);
    }
  }

  ~lmdb_hash_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
    delete suffix_strings;

    MUTEX_DESTROY(&M);
  }

  void insert(const std::string& binary_hash, lmdb_changes_t& changes) {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // convert binary_hash into prefix and suffix
    std::pair<std::string, std::string> pair = hash_pair(binary_hash);

    // see if prefix is already there
    // set key to prefix
    lmdb_helper::point_to_string(pair.first, context.key);

    // set cursor to prefix
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    std::string encoding;

    // handle when prefix and suffix is new
    if (rc == MDB_NOTFOUND) {
      // set data to suffix
      suffix_strings->clear();
      suffix_strings->insert(pair.second);
      encoding = encode_data(*suffix_strings);
      lmdb_helper::point_to_string(encoding, context.data);

      // add this new prefix and its suffix
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the add request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // hash inserted
      context.close();
      ++changes.hash_inserted;
      MUTEX_UNLOCK(&M);
      return;

    // handle when prefix already exists
    } else if (rc == 0) {
      encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, *suffix_strings);

      if (suffix_strings->find(pair.second) != suffix_strings->end()) {

        // suffix already exists, hash not inserted
        context.close();
        ++changes.hash_already_present;
        MUTEX_UNLOCK(&M);
        return;

      } else {

        // suffix did not exist, so add suffix and write back
        suffix_strings->insert(pair.second);
        encoding = encode_data(*suffix_strings);
        lmdb_helper::point_to_string(encoding, context.data);
        rc = mdb_put(context.txn, context.dbi,
                       &context.key, &context.data, MDB_NODUPDATA);

        // write must work
        if (rc != 0) {
          std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
          assert(0);
        }

        // hash inserted
        context.close();
        ++changes.hash_inserted;
        MUTEX_UNLOCK(&M);
        return;
      }

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Find if hash is present.
   */
  bool find(const std::string& binary_hash) const {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // convert binary_hash into prefix and suffix
    std::pair<std::string, std::string> pair = hash_pair(binary_hash);

    // see if prefix is already there
    // set key to prefix
    lmdb_helper::point_to_string(pair.first, context.key);

    // set cursor to prefix
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // handle when prefix is not there
    if (rc == MDB_NOTFOUND) {
      // the hash is not present because the prefix is not present
      context.close();
      return false;

    } else if (rc == 0) {
      // prefix present, so get set of suffixes and look for a match
      std::string encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, *suffix_strings);

      if (suffix_strings->find(pair.second) != suffix_strings->end()) {
        // suffix found
        context.close();
        return true;

      } else {
        // suffix not found
        context.close();
        return false;
      }
    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif


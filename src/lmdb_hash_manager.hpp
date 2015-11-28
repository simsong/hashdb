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
 * Provides services for modifying the DB, including tracking changes.
 *
 * Lock non-thread-safe interfaces before use.
 */

#ifndef LMDB_HASH_MANAGER_HPP
#define LMDB_HASH_MANAGER_HPP
#include "globals.hpp"
#include "file_modes.h"
#include "db_typedefs.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_data_codec.hpp"
#include "mutex_lock.hpp"
#include "bloom_filter_manager.hpp"
#include "hashdb_changes.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

// no concurrent changes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

class lmdb_hash_manager_t {

  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  hashdb_settings_t settings;
  bloom_filter_manager_t bloom_filter_manager;
  MDB_env* env;

  // do not allow copy or assignment
  lmdb_hash_manager_t(const lmdb_hash_manager_t&);
  lmdb_hash_manager_t& operator=(const lmdb_hash_manager_t&);

  public:
  lmdb_hash_manager_t(const std::string& p_hashdb_dir,
                      const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       settings(hashdb_settings_t(hashdb_settings_store_t::read_settings(
                                                               hashdb_dir))),
       bloom_filter_manager(hashdb_dir,
                            file_mode,
                            settings.hash_truncation,
                            settings.bloom_is_used,
                            settings.bloom_M_hash_size,
                            settings.bloom_k_hash_functions),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_store", file_mode)) {
  }

  ~lmdb_hash_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
  }

  void insert(const uint64_t source_id,
              const hash_data_list_t& hash_data_list,
              hashdb_changes_t& changes) {

    // insert each hash_data entry
    for(hash_data_list_t::const_iterator it=hash_data_list.begin();
                                       it != hash_data_list.end(); ++it) {

      // validate the byte alignment
      if (it->file_offset % settings.sector_size != 0) {
        ++changes.hashes_not_inserted_invalid_sector_size;
        continue;
      }
      size_t offset_index = it->file_offset / settings.sector_size;

      // maybe grow the DB
      lmdb_helper::maybe_grow(env);

      // get context
      lmdb_context_t context(env, true, true);
      context.open();

      // set key
      lmdb_helper::point_to_string(it->binary_hash, context.key);

      // truncate key if truncation is used and binary_hash is longer
      if (settings.hash_truncation != 0 &&
          context.key.mv_size > settings.hash_truncation) {
        context.key.mv_size = settings.hash_truncation;
      }

      // set data
      std::string encoding = lmdb_data_codec::encode_hash_data(
                                                    source_id, offset_index);
      lmdb_helper::point_to_string(encoding, context.data);

      // see if this entry exists yet
      // set the cursor to this key,data pair
      int rc = mdb_cursor_get(context.cursor,
                              &context.key, &context.data,
                              MDB_GET_BOTH);
      bool has_pair = false;
      if (rc == 0) {
        has_pair = true;
      } else if (rc == MDB_NOTFOUND) {
        // not found
      } else {
        // program error
        has_pair = false; // satisfy mingw32-g++ compiler
        std::cerr << "LMDB find error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
 
      if (has_pair) {
        // this exact entry already exists
        ++changes.hashes_not_inserted_duplicate_element;
        context.close();
        continue;
      }

      // insert the entry since all the checks passed
      rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);
      if (rc != 0) {
        std::cerr << "LMDB insert error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
      ++changes.hashes_inserted;

      context.close();

      // add hash to bloom filter, too, even if already there
      bloom_filter_manager.add_hash_value(it->binary_hash);
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif


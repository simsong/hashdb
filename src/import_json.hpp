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
 * Import from data in JSON format.  Lines are one of:
 *   source data, block hash data, or comment.
 *
 * Source data:
 *   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
 *   "nonprobative_count":4, "names":[{"repository_name":"repository1",
 *   "filename":"filename1"}]}
 *
 * Block hash data:
 *   {"block_hash":"a7df...", "entropy":8, "block_label":"W",
 *   "source_offset_pairs":["b9e7...", 4096]}
 *
 * Comment line:
 *   Comment lines start with #.
 */

#ifndef IMPORT_JSON_HPP
#define IMPORT_JSON_HPP

#include <zlib.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"
#include "hex_helper.hpp"
#include <string.h> // for strerror
#include <fstream>
#include "rapidjson.h"
#include "writer.h"
#include "document.h"

class import_json_t {
  private:

  // state
  const std::string& hashdb_dir;
  size_t line_number;

  // resources
  hashdb::import_manager_t manager;
  progress_tracker_t progress_tracker;

  // do not allow these
  import_json_t();
  import_json_t(const import_json_t&);
  import_json_t& operator=(const import_json_t&);

  // private, used by read()
  import_json_t(const std::string& p_hashdb_dir,
                const std::string& cmd) :
        hashdb_dir(p_hashdb_dir),
        line_number(0),
        manager(hashdb_dir, cmd),
        progress_tracker(hashdb_dir, 0, cmd) {
  }

  void report_invalid_line(const std::string& field,
                           const std::string& line) const {
    std::cerr << "Invalid line " << line_number
              << " field: " << field
              << ": '" << line << "'\n";
  }

  // Source data:
  //   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
  //   "nonprobative_count":4, "names":[{"repository_name":"repository1",
  //   "filename":"filename1"}]}
  void read_source_data(const rapidjson::Document& document,
                        const std::string& line) {

    // parse file_hash
    if (!document.HasMember("file_hash") ||
                  !document["file_hash"].IsString()) {
      report_invalid_line("source data file_hash", line);
      return;
    }
    const std::string file_binary_hash = hex_to_bin(
                                     document["file_hash"].GetString());

    // parse filesize
    if (!document.HasMember("filesize") ||
                  !document["filesize"].IsUint64()) {
      report_invalid_line("source data filesize", line);
      return;
    }
    const uint64_t filesize = document["filesize"].GetUint64();

    // parse file_type (optional)
    std::string file_type =
                 (document.HasMember("file_type") &&
                  document["file_type"].IsString()) ?
                     document["file_type"].GetString() : "";

    // parse nonprobative_count (optional)
    uint64_t nonprobative_count =
                 (document.HasMember("nonprobative_count") &&
                  document["nonprobative_count"].IsUint64()) ?
                     document["nonprobative_count"].GetUint64() : 0;

    // add the source data
    manager.insert_source_data(file_binary_hash,
                               filesize, file_type, nonprobative_count);

    // parse names:[]
    if (!document.HasMember("names") ||
                  !document["names"].IsArray()) {
      report_invalid_line("source data names", line);
      return;
    }
    const rapidjson::Value& json_names = document["names"];
    for (rapidjson::SizeType i = 0; i< json_names.Size(); ++i) {

      // parse repository_name
      if (!json_names[i].HasMember("repository_name") ||
                    !json_names[i]["repository_name"].IsString()) {
        report_invalid_line("source data repository_name", line);
        return;
      }
      std::string repository_name =
                     json_names[i]["repository_name"].GetString();

      // parse filename
      if (!json_names[i].HasMember("filename") ||
                    !json_names[i]["filename"].IsString()) {
        report_invalid_line("source data filename", line);
        return;
      }
      const std::string filename = json_names[i]["filename"].GetString();

      // add the name pair
      manager.insert_source_name(file_binary_hash, repository_name, filename);
    }
  }

  // Block hash data:
  //   {"block_hash":"a7df...", "entropy":8, "block_label":"W",
  //   "source_offset_pairs":["b9e7...", 4096]}


  void read_block_hash_data(const rapidjson::Document& document,
                            const std::string& line) {

    // block_hash
    if (!document.HasMember("block_hash") ||
                  !document["block_hash"].IsString()) {
      report_invalid_line("block hash data block_hash", line);
      return;
    }
    const std::string binary_hash = hex_to_bin(
                                       document["block_hash"].GetString());

    // entropy (optional)
    uint64_t entropy =
                 (document.HasMember("entropy") &&
                  document["entropy"].IsUint64()) ?
                     document["entropy"].GetUint64() : 0;

    // block_label (optional)
    std::string block_label =
                 (document.HasMember("block_label") &&
                  document["block_label"].IsString()) ?
                     document["block_label"].GetString() : "";

    // source_offset_pairs:[]
    if (!document.HasMember("source_offset_pairs") ||
                  !document["source_offset_pairs"].IsArray()) {
      report_invalid_line("block hash data source_offset_pairs", line);
      return;
    }
    const rapidjson::Value& json_pairs = document["source_offset_pairs"];
    for (rapidjson::SizeType i = 0; i+1 < json_pairs.Size(); i+=2) {

      // source hash
      if (!json_pairs[i].IsString()) {
        report_invalid_line("block hash data source_offset_pair source hash",
                            line);
        return;
      }
      const std::string file_binary_hash = hex_to_bin(
                                                 json_pairs[i].GetString());

      // file offset
      if (!json_pairs[i+1].IsUint64()) {
        report_invalid_line("block hash data source_offset_pair file offset",
                            line);
        return;
      }
      const uint64_t file_offset = json_pairs[i+1].GetUint64();

      // insert the hash
      manager.insert_hash(binary_hash, file_binary_hash, file_offset,
                          entropy, block_label);
    }
  }

  void read_lines(std::istream& in) {
    std::string line;
    while(getline(in, line)) {
      ++line_number;

      // skip comment lines
      if (line[0] == '#') {
        continue;
      }

      // skip empty lines
      if (line.size() == 0) {
        continue;
      }

      // open the line as a JSON DOM document
      rapidjson::Document document;
      if (document.Parse(line.c_str()).HasParseError() ||
          !document.IsObject()) {
        report_invalid_line("DOM parse error", line);
        continue;
      }

      // import JSON
      if (document.HasMember("file_hash")) {
        read_source_data(document, line);
      } else if (document.HasMember("block_hash")) {
        read_block_hash_data(document, line);
      } else {
        report_invalid_line("no file_hash or block_hash", line);
      }
    }
  }

  public:

  // read JSON file
  static void read(const std::string& hashdb_dir,
                   const std::string& cmd,
                   std::istream& in) {

    // create the reader
    import_json_t reader(hashdb_dir, cmd);

    // read the lines
    reader.read_lines(in);
  }
};

#endif


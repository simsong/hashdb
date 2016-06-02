// Author:  Bruce Allen
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
 * Read raw bytes from a media image.
 */
#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif

#include <string>
#include <cassert>
#include <iostream>
#include <unistd.h> // for F_OK
#include <sstream>
#include "num_cpus.hpp"
#include "hashdb.hpp"
#include "file_reader.hpp"
#include "hash_calculator.hpp"
#include "threadpool.hpp"
#include "job.hpp"
#include "job_queue.hpp"
#include "scan_tracker.hpp"
#include "tprint.hpp"

namespace hashdb {

  // read bytes from image starting at offset.
  std::string read_bytes(const std::string& image_filename,
                         const uint64_t offset,
                         const uint64_t count,
                         std::string& bytes) {

    // open the file reader
    const hasher::file_reader_t file_reader(hasher::utf8_to_native(
                                                           image_filename));
    if (file_reader.error_message.size() > 0) {
      // the file failed to open
      return file_reader.error_message;
    }

    // create a buffer to read into
    uint8_t* b = new (std::nothrow) uint8_t[count]();
    if (b == NULL) {
      // abort
      return "bad memory allocation";
    }

    // read into b
    size_t b_bytes_read = 0;
    const std::string error_message =
                          file_reader.read(offset, b, count, &b_bytes_read);

    if (error_message.size() == 0) {
      // good, print bytes to stdout
      bytes = std::string(reinterpret_cast<const char*>(b), b_bytes_read);
    }

    // done
    delete[] b;
    return error_message;
  }

  // read bytes from image starting at forensic path.
  std::string read_bytes_fp(const std::string& image_filename,
                            const std::string& forensic_path,
                            const uint64_t count,
                            std::string& bytes) {
/*

    // split forensic path into array of parts
    std::vector<std::string> parts;
    std::stringstream ss(forensic_path);
    std::string part;
    while (std::getline(ss, part, '-')) {
      parts.push_back(part);
    }

    // get file offset
    std::vector<std::string>::const_iterator it = parts.begin();
    if (it == parts.end()) {
      return "invalid forensic path";
    }
    uint64_t offset = 0;
    std::istringstream iss(str);
    iss >> offset;

    // open the file reader
    const hasher::file_reader_t file_reader(hasher::utf8_to_native(
                                                           image_filename));
    if (file_reader.error_message.size() > 0) {
      // the file failed to open
      return file_reader.error_message;
    }

    // create a buffer to read into, allow 1MB
    static const size_t in_size = 1048576; // 1MiB = 2^20
    uint8_t* in_buf = new (std::nothrow) uint8_t[buffer_size]();
    if (in_buf == NULL) {
      // abort
      return "bad memory allocation";
    }

    // read into in_buf
    size_t b_bytes_read = 0;
    const std::string error_message =
               file_reader.read(offset, in_buf, buffer_size, &b_bytes_read);

    // now recursively read down the forensic path
    while (++it != parts.end()) {
      std::string compression_type = *it;

      // pointer to the output buffer that will be created using new
      *out_buf = NULL;
      *out_size = 0;

      if (compression_type == "zip") {
        new_from_zip(image_filename, in_buf, in_size, &out_buf, &out_size);
        delete[] in_buf;
}
}



    // get compression type

    if (error_message.size() == 0) {
      // good, print bytes to stdout
      bytes = std::string(reinterpret_cast<const char*>(b), b_bytes_read);
    }

    // done
    delete[] b;
*/

//    return error_message;
    return "";
  }

} // end namespace hashdb


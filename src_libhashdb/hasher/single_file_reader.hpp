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
 * Read chunks from a single file
 *
 * Adapted heavily from bulk_extractor/src/image_process.cpp.
 */


#ifndef SINGLE_FILE_READER_HPP
#define SINGLE_FILE_READER_HPP

#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cassert>
#include <libewf.h>
#include "file_reader_helper.hpp"

#ifndef O_BINARY
#define O_BINARY 0
#endif

namespace hasher {

class single_file_reader_t {

  public:
  const filename_t filename;
  std::string error_message;

  private:
  // SINGLE file data
#ifdef WIN32
  mutable HANDLE file_handle;         // currently open file
#else
  mutable int fd;                     // currently open file
#endif

  public:
  const bool is_open;
  const uint64_t filesize;

  private:

  // do not allow copy or assignment
  single_file_reader_t(const single_file_reader_t&);
  single_file_reader_t& operator=(const single_file_reader_t&);

  // open the file for reading
  bool open_reader() {

    // open the file for reading
#ifdef WIN32
    file_handle = CreateFileA(filename.c_str(), FILE_READ_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                     OPEN_EXISTING, 0, NULL);
    if(file_handle==INVALID_HANDLE_VALUE){
      std::stringstream ss;
      ss << "hashdb WIN32 subsystem: cannot open file " << filename;
      error_message = ss.str();
      return false;
    }
#else        
    fd = ::open(filename.c_str(),O_RDONLY|O_BINARY);
    if(fd<=0) {
      std::stringstream ss;
      ss << "hashdb Linux subsystem: cannot open file " << filename;
      error_message = ss.str();
      return false;
    }
#endif
    return true;
  }

  public:
  /**
   * Opens a single file reader.
   * Provide the filename or device name to read from.
   * Check is_open.  If false, print error_message.
   */
  single_file_reader_t(const filename_t& p_filename) :
          filename(p_filename),
          error_message(""),
#ifdef WIN32
          file_handle(INVALID_HANDLE_VALUE),
#else
          fd(-1),
#endif
          is_open(open_reader()),
          filesize(getSizeOfFile(filename)) {
  }

  // close any open resources
  ~single_file_reader_t() {
    // SINGLE binary file
#ifdef WIN32
    if(single_handle!=INVALID_HANDLE_VALUE) ::CloseHandle(single_handle);
#else
    if(fd>=0) close(fd);
#endif
  }

  std::string read(const uint64_t offset,
                   uint8_t* const buffer,
                   const size_t buffer_size,
                   size_t* const bytes_read) const {

#ifdef WIN32
    *bytes_read = 0; //DWORD
    LARGE_INTEGER li;
    li.QuadPart = offset;
    li.LowPart = SetFilePointer(serial_current_handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if(li.LowPart == INVALID_SET_FILE_POINTER) {
      return "read failed, invalid set file pointer";
    }
    bool did_read = ReadFile(serial_current_handle,
                    buffer, (DWORD) buffer_size, bytes_read, NULL);
    if (!did_read) {
      return "read failed";
    }
#else
  #if defined(HAVE_PREAD64)
    /* If we have pread64, make sure it is defined */
    extern size_t pread64(int fd,char *buffer,size_t nbyte,off_t offset);
  #endif

  #if !defined(HAVE_PREAD64) && defined(HAVE_PREAD)
    /* if we are not using pread64, make sure that off_t is 8 bytes in size */
  #define pread64(d,buffer,nbyte,offset) pread(d,buffer,nbyte,offset)
  #endif

    ssize_t count = ::pread64(fd,buffer,buffer_size,offset);
    if (count < 0) {
      *bytes_read = 0;
      return "read failed";
    } else {
      *bytes_read = static_cast<size_t>(count);
      return "";
    }
#endif
  }
};

} // end namespace hasher

#endif


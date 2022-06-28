/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SD_h
#define SD_h

#include "Arduino.h"
#include "libs/base/filesystem.h"

#define FILE_READ 0
#define FILE_WRITE 1
#define DONT_CARE 0

namespace coral::micro {
namespace arduino {

namespace SDLib {

class File : public Stream {
 private:
  char name_[LFS_NAME_MAX + 1];
  std::shared_ptr<lfs_file_t> file_handle_ = nullptr;
  std::shared_ptr<lfs_dir_t> dir_handle_ = nullptr;
  bool open_ = false;
  bool is_dir_ = false;
  bool is_writable_ = false;

 protected:
  File(std::shared_ptr<lfs_file_t> handle, const char *name, bool writable);
  File(std::shared_ptr<lfs_dir_t> handle, const char *name);

 public:
  File(void);  // 'empty' constructor
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual int read();
  virtual int peek();
  virtual int available();
  virtual void flush();
  int read(void *buf, size_t nbyte);
  bool seek(size_t pos);
  size_t position();
  size_t size();
  void close();
  operator bool() { return open_; }
  char *name();

  bool isDirectory(void) { return is_dir_; }
  File openNextFile(uint8_t mode = FILE_READ);
  void rewindDirectory(void);

  friend class SDClass;
};

class SDClass {
 public:
  // The arguments to the begin methods are ignored since it's not a "real" SD
  // card
  bool begin(uint8_t csPin = DONT_CARE);
  bool begin(uint32_t clock, uint8_t csPin) { return begin(); }

  void end() {}

  File open(const char *filename, uint8_t mode = FILE_READ);
  File open(const String &filename, uint8_t mode = FILE_READ) {
    return open(filename.c_str(), mode);
  }

  bool exists(const char *filepath);
  bool exists(const String &filepath) { return exists(filepath.c_str()); }

  bool mkdir(const char *filepath);
  bool mkdir(const String &filepath) { return mkdir(filepath.c_str()); }

  bool remove(const char *filepath);
  bool remove(const String &filepath) { return remove(filepath.c_str()); }

  bool rmdir(const char *filepath);
  bool rmdir(const String &filepath) { return rmdir(filepath.c_str()); }
};

}  // namespace SDLib
}  // namespace arduino
}  // namespace coral::micro
// using namespace coral::micro::arduino::SDLib;

typedef coral::micro::arduino::SDLib::File SDFile;
extern coral::micro::arduino::SDLib::SDClass SD;

#endif  // SD_h

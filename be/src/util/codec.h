// Copyright 2012 Cloudera Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef IMPALA_UTIL_CODEC_H
#define IMPALA_UTIL_CODEC_H

#include "common/status.h"
#include "runtime/mem-pool.h"
#include "util/runtime-profile.h"

#include <boost/scoped_ptr.hpp>
#include "gen-cpp/Descriptors_types.h"

namespace impala {

class MemPool;
class RuntimeState;

// Create a compression object.  This is the base class for all compression
// algorithms. A compression algorithm is either a compressor or a decompressor.
// To add a new algorithm, generally, both a compressor and a decompressor
// will be added.  Each of these objects inherits from this class. The objects
// are instantiated in the Create static methods defined here.  The type of
// compression is defined in the Thrift interface THdfsCompression.
class Codec {
 public:
  // These are the codec string representation used in Hadoop.
  static const char* const DEFAULT_COMPRESSION;
  static const char* const GZIP_COMPRESSION;
  static const char* const BZIP2_COMPRESSION;
  static const char* const SNAPPY_COMPRESSION;

  // Map from codec string to compression format
  static const std::map<const std::string, const THdfsCompression> CODEC_MAP;

  // Create a decompressor.
  // Input: 
  //  runtime_state: the current runtime state.
  //  mem_pool: the memory pool used to store the decompressed data.
  //  reuse: if true the allocated buffer can be reused.
  //  format: the type of decompressor to create.
  // Output:
  //  decompressor: pointer to the decompressor class to use.
  static Status CreateDecompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                   bool reuse, THdfsCompression::type format,
                                   Codec** decompressor);

  // Alternate creator: takes a codec string and returns a scoped pointer.
  static Status CreateDecompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                   bool reuse, const std::string& codec,
                                   boost::scoped_ptr<Codec>* decompressor);

  // Create the compressor.
  // Input: 
  //  runtime_state: the current runtime state.
  //  mem_pool: the memory pool used to store the compressed data.
  //  format: The type of compressor to create.
  //  reuse: if true the allocated buffer can be reused.
  // Output:
  //  compressor: pointer to the compressor class to use.
  static Status CreateCompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                 bool reuse, THdfsCompression::type format,
                                 Codec** decompressor);

  // Alternate creator: takes a codec string and returns a scoped pointer.
  // Input, as above except:
  //  codec: the string representing the codec of the current file.
  static Status CreateCompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                 bool reuse, const std::string& codec,
                                 boost::scoped_ptr<Codec>* compressor);

  virtual ~Codec() {}

  // Process a block of data.  The operator will allocate the output buffer
  // if output_length is passed as 0 and return the length in output_length.
  // If it is non-zero the length must be the correct size to hold the transformed output.
  // Inputs:
  //   input_length: length of the data to process
  //   input: data to process
  // In/Out:
  //   output_length: Length of the output, if known, 0 otherwise.
  // Output:
  //   output: Pointer to processed data
  virtual Status ProcessBlock(int input_length, uint8_t* input,
                              int* output_length, uint8_t** output)  = 0;

  // Return the name of a compression algorithm.
  static std::string GetCodecName(THdfsCompression::type);

 protected:
  // Largest block we will compress/decompress: 2GB.
  // We are dealing with compressed blocks that are never this big but we
  // want to guard against a corrupt file that has the block length as some
  // large number.
  static const int MAX_BLOCK_SIZE = (2L * 1024 * 1024 * 1024) - 1;
  // Create a compression operator
  // Inputs:
  //   mem_pool: memory pool to allocate the output buffer, this implies that the
  //             caller is responsible for the memory allocated by the operator.
  //   reuse_buffer: if false always allocate a new buffer rather than reuse.
  Codec(MemPool* mem_pool, bool reuse_buffer);

  // Initialize the operation.
  virtual Status Init() = 0;

  // Pool to allocate the buffer to hold transformed data.
  MemPool* memory_pool_;

  // Temporary memory pool: in case we get the output size too small we can
  // use this to free unused buffers.
  MemPool temp_memory_pool_;

  // Can we reuse the output buffer or do we need to allocate on each call?
  bool reuse_buffer_;

  // Buffer to hold transformed data.
  // Either passed from the caller or allocated from memory_pool_.
  uint8_t* out_buffer_;

  // Length of the output buffer.
  int buffer_length_;
};

}
#endif

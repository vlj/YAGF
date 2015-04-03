// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

#include <Core/IMeshBuffer.h>
#include <Core/Singleton.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>


#include <D3DAPI/Context.h>
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>


template<typename Data>
class ArrayBuffer
{
private:
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
  size_t realSize;
  size_t advertisedSize;

  Data *Pointer;
public:
  ArrayBuffer(size_t preallocsize) : realSize(preallocsize * sizeof(Data)), advertisedSize(0), Pointer(0)
  {
    hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(preallocsize * sizeof(Data)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&buffer));
    Pointer = buffer->Map(0, nullptr, (void**)&Pointer);
  }

  ~ArrayBuffer()
  {

  }

  void resizeBufferIfNecessary(size_t requestedSize)
  {
/*    if (requestedSize >= realSize)
    {
      while (requestedSize >= realSize)
        realSize = 2 * realSize + 1;
      GLuint newVBO;
      glGenBuffers(1, &newVBO);
      glBindBuffer(BufferType, newVBO);
                  if (CVS->supportsAsyncInstanceUpload())
      {
      glBufferStorage(BufferType, realSize * sizeof(Data), 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
      Pointer = static_cast<Data *>(glMapBufferRange(BufferType, 0, realSize * sizeof(Data), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT));
      }
      else
      glBufferData(BufferType, realSize * sizeof(Data), 0, GL_DYNAMIC_DRAW);

      if (buffer)
      {
        // Copy old data
        GLuint oldVBO = buffer;
        glBindBuffer(GL_COPY_WRITE_BUFFER, newVBO);
        glBindBuffer(GL_COPY_READ_BUFFER, oldVBO);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, advertisedSize * sizeof(Data));
        glDeleteBuffers(1, &oldVBO);
      }
      buffer = newVBO;
    }
    advertisedSize = requestedSize;*/
  }

  Data *getPointer()
  {
    return Pointer;
  }

  size_t getSize() const
  {
    return advertisedSize;
  }

  void append(size_t count, const void *ptr)
  {
    size_t from = advertisedSize;
    memcpy(&Pointer[from], ptr, count * sizeof(Data));
  }
};
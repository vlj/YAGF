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


/** Set of buffers describing a formatted (ie with bound attributes) vertex storage.
*  /tparam S3DVertexFormat is the vertex type
*  /tparam AppendedData is used for vertex "annotation" in companion buffer (like skinning weight)
*/
template <typename S3DVertexFormat, typename ...AppendedData>
class FormattedVertexStorage
{
private:
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > vertexbuffers;
public:
  FormattedVertexStorage(std::vector<irr::scene::IMeshBuffer<S3DVertexFormat> *> meshes)
  {
    size_t total_vertex_cnt = 0, total_index_cnt = 0;
    for (irr::scene::IMeshBuffer<S3DVertexFormat> *mesh : meshes)
    {
      total_vertex_cnt += mesh->getVertexCount();
      total_index_cnt += mesh->getIndexCount();
    }

    ID3D12Resource *cpudata;
    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_vertex_cnt * sizeof(S3DVertexFormat)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&cpudata));

    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_vertex_cnt * sizeof(S3DVertexFormat)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&vertexbuffers[0]));
  }

};
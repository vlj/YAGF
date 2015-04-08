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
#include <D3DAPI/Resource.h>

#include <thread>


/** Set of buffers describing a formatted (ie with bound attributes) vertex storage.
*  /tparam S3DVertexFormat is the vertex type
*  /tparam AppendedData is used for vertex "annotation" in companion buffer (like skinning weight)
*/
template <typename S3DVertexFormat, typename ...AppendedData>
class FormattedVertexStorage
{
private:
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > vertexbuffers;
  Microsoft::WRL::ComPtr<ID3D12Resource> indexbuffer;

  D3D12_VERTEX_BUFFER_VIEW vtxb = {};
  D3D12_INDEX_BUFFER_VIEW idxb = {};

public:
  std::vector<std::tuple<size_t, size_t, size_t> > meshOffset;

  FormattedVertexStorage(ID3D12CommandQueue *queue, const std::vector<irr::scene::IMeshBuffer<S3DVertexFormat> >& meshes)
  {
    size_t total_vertex_cnt = 0, total_index_cnt = 0;
    for (const irr::scene::IMeshBuffer<S3DVertexFormat>& mesh : meshes)
    {
      total_vertex_cnt += mesh.getVertexCount();
      total_index_cnt += mesh.getIndexCount();
    }
    vertexbuffers.resize(1);
    ID3D12Resource *cpuvertexdata;
    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_vertex_cnt * sizeof(S3DVertexFormat)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&cpuvertexdata));

    ID3D12Resource *cpuindexdata;
    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_index_cnt * sizeof(unsigned short)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&cpuindexdata));

    S3DVertexFormat *vertexmap;
    unsigned short *indexmap;
    hr = cpuvertexdata->Map(0, nullptr, (void**)&vertexmap);
    hr = cpuindexdata->Map(0, nullptr, (void**)&indexmap);

    size_t basevertex = 0, baseindex = 0;

    for (const irr::scene::IMeshBuffer<S3DVertexFormat>& mesh : meshes)
    {
      memcpy(&vertexmap[basevertex], mesh.getVertices(), sizeof(S3DVertexFormat) * mesh.getVertexCount());
      memcpy(&indexmap[baseindex], mesh.getIndices(), sizeof(unsigned short) * mesh.getIndexCount());
      meshOffset.push_back(std::make_tuple(mesh.getIndexCount(), basevertex, baseindex));
      basevertex += mesh.getVertexCount();
      baseindex += mesh.getIndexCount();
    }

    cpuvertexdata->Unmap(0, nullptr);
    cpuindexdata->Unmap(0, nullptr);

    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_vertex_cnt * sizeof(S3DVertexFormat)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&vertexbuffers[0]));

    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_index_cnt * sizeof(unsigned short)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&indexbuffer));

    ID3D12CommandAllocator* temporarycommandalloc;
    hr = Context::getInstance()->dev->CreateCommandAllocator(queue->GetDesc().Type, IID_PPV_ARGS(&temporarycommandalloc));

    ID3D12GraphicsCommandList *uploadcmdlist;
    hr = Context::getInstance()->dev->CreateCommandList(1, queue->GetDesc().Type, temporarycommandalloc, nullptr, IID_PPV_ARGS(&uploadcmdlist));
    uploadcmdlist->CopyBufferRegion(vertexbuffers[0].Get(), 0, cpuvertexdata, 0, total_vertex_cnt * sizeof(S3DVertexFormat), D3D12_COPY_NONE);
    uploadcmdlist->CopyBufferRegion(indexbuffer.Get(), 0, cpuindexdata, 0, total_index_cnt * sizeof(unsigned short), D3D12_COPY_NONE);

    D3D12_RESOURCE_BARRIER_DESC barriers[2] = {
      setResourceTransitionBarrier(vertexbuffers[0].Get(), D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_GENERIC_READ),
      setResourceTransitionBarrier(indexbuffer.Get(), D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_GENERIC_READ)
    };
    uploadcmdlist->ResourceBarrier(2, barriers);

    uploadcmdlist->Close();
    queue->ExecuteCommandLists(1, (ID3D12CommandList**)&uploadcmdlist);

    std::thread t1([=]() {temporarycommandalloc->Release(); uploadcmdlist->Release(); cpuindexdata->Release(); cpuvertexdata->Release(); });
    t1.detach();

    vtxb.BufferLocation = vertexbuffers[0]->GetGPUVirtualAddress();
    vtxb.SizeInBytes = (UINT)(total_vertex_cnt * sizeof(S3DVertexFormat));
    vtxb.StrideInBytes = sizeof(S3DVertexFormat);

    idxb.BufferLocation = indexbuffer->GetGPUVirtualAddress();
    idxb.Format = DXGI_FORMAT_R16_UINT;
    idxb.SizeInBytes = (UINT)(total_index_cnt * sizeof(unsigned short));
  }

  D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const
  {
    return vtxb;
  }

  D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const
  {
    return idxb;
  }

};
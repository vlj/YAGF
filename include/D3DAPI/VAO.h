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
public:
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

    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_vertex_cnt * sizeof(S3DVertexFormat)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&vertexbuffers[0]));

    ID3D12Resource *cpuindexdata;
    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_index_cnt * sizeof(unsigned short)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&cpuindexdata));

    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(total_index_cnt * sizeof(unsigned short)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&indexbuffer));

    ID3D12CommandAllocator* cmdalloc;
    hr = Context::getInstance()->dev->CreateCommandAllocator(queue->GetDesc().Type, IID_PPV_ARGS(&cmdalloc));

    ID3D12GraphicsCommandList *cmdlist;
    hr = Context::getInstance()->dev->CreateCommandList(1, queue->GetDesc().Type, cmdalloc, nullptr, IID_PPV_ARGS(&cmdlist));
    cmdlist->CopyBufferRegion(vertexbuffers[0].Get(), 0, cpuvertexdata, 0, total_vertex_cnt * sizeof(S3DVertexFormat), D3D12_COPY_NONE);
    cmdlist->CopyBufferRegion(indexbuffer.Get(), 0, cpuindexdata, 0, total_index_cnt * sizeof(unsigned short), D3D12_COPY_NONE);
    cmdlist->Close();
    queue->ExecuteCommandLists(1, (ID3D12CommandList**) &cmdlist);

    std::thread t1([=]() {cmdalloc->Release(); cmdlist->Release(); cpuindexdata->Release(); cpuvertexdata->Release(); });
    t1.detach();
  }

};
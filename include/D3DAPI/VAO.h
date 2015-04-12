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

template<typename... Args>
struct VertexFiller;

template<>
struct VertexFiller<>
{
  static void Fill(std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > &vertexbuffer, std::vector<ID3D12Resource*> &cpuVertexBuffers, size_t vtx_count)
  {}

  template<unsigned N>
  static void BuildDesc(std::vector<D3D12_VERTEX_BUFFER_VIEW> &vtxb, size_t vtx_count, std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > &vertexbuffers)
  {
    assert(vtxb.size() == N + 1);
  }
};

template<typename T, typename... Args>
struct VertexFiller<T, Args...>
{
  static void Fill(std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > &vertexbuffers, std::vector<ID3D12Resource*> &cpuVertexBuffers, size_t vtx_count,
    const std::vector<std::vector<T> > &Data, const std::vector<std::vector<Args> > &... Otherdata)
  {
    ID3D12Resource *newcpubuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> newgpubuffer;
    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(vtx_count * sizeof(T)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&newcpubuffer));

    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(vtx_count * sizeof(T)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&newgpubuffer));

    void *ptr;
    newcpubuffer->Map(0, nullptr, &ptr);
    size_t offset = 0;
    for (const std::vector<T> &appendedData : Data)
    {
      memcpy((char*)ptr + offset, appendedData.data(), sizeof(T) * appendedData.size());
      offset += sizeof(T) * appendedData.size();
    }

    cpuVertexBuffers.push_back(newcpubuffer);
    vertexbuffers.push_back(newgpubuffer);
    VertexFiller<Args...>::Fill(vertexbuffers, cpuVertexBuffers, vtx_count, Otherdata...);
  }

  template<unsigned N>
  static void BuildDesc(std::vector<D3D12_VERTEX_BUFFER_VIEW> &vtxb, size_t vtx_count, std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > &vertexbuffers)
  {
    D3D12_VERTEX_BUFFER_VIEW newvtxb = {};
    newvtxb.BufferLocation = vertexbuffers[N + 1]->GetGPUVirtualAddress();
    newvtxb.SizeInBytes = (UINT)vtx_count * sizeof(T);
    newvtxb.StrideInBytes = sizeof(T);
    vtxb.push_back(newvtxb);
    VertexFiller<Args...>::template BuildDesc<N + 1>(vtxb, vtx_count, vertexbuffers);
  }
};

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

  std::vector<D3D12_VERTEX_BUFFER_VIEW> vtxb;
  D3D12_INDEX_BUFFER_VIEW idxb = {};

public:
  std::vector<std::tuple<size_t, size_t, size_t> > meshOffset;

  FormattedVertexStorage(ID3D12CommandQueue *queue, const std::vector<irr::scene::IMeshBuffer<S3DVertexFormat> >& meshes, const std::vector<std::vector<AppendedData> > &... Otherdata)
  {
    size_t total_vertex_cnt = 0, total_index_cnt = 0;
    for (const irr::scene::IMeshBuffer<S3DVertexFormat>& mesh : meshes)
    {
      total_vertex_cnt += mesh.getVertexCount();
      total_index_cnt += mesh.getIndexCount();
    }
    vertexbuffers.resize(1);
    ID3D12Resource *cpuvertexdata;
    ID3D12Resource *cpuindexdata;

    S3DVertexFormat *vertexmap;
    unsigned short *indexmap;

    {
      HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
        &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_MISC_NONE,
        &CD3D12_RESOURCE_DESC::Buffer(total_vertex_cnt * sizeof(S3DVertexFormat)),
        D3D12_RESOURCE_USAGE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&cpuvertexdata));


      hr = cpuvertexdata->Map(0, nullptr, (void**)&vertexmap);

      hr = Context::getInstance()->dev->CreateCommittedResource(
        &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_MISC_NONE,
        &CD3D12_RESOURCE_DESC::Buffer(total_vertex_cnt * sizeof(S3DVertexFormat)),
        D3D12_RESOURCE_USAGE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&vertexbuffers[0]));
    }

    {
      HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
        &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_MISC_NONE,
        &CD3D12_RESOURCE_DESC::Buffer(total_index_cnt * sizeof(unsigned short)),
        D3D12_RESOURCE_USAGE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&cpuindexdata));

      hr = cpuindexdata->Map(0, nullptr, (void**)&indexmap);

      hr = Context::getInstance()->dev->CreateCommittedResource(
        &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_MISC_NONE,
        &CD3D12_RESOURCE_DESC::Buffer(total_index_cnt * sizeof(unsigned short)),
        D3D12_RESOURCE_USAGE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&indexbuffer));
    }

    std::vector<ID3D12Resource*> cpuresources = { cpuvertexdata };

    VertexFiller<AppendedData...>::Fill(vertexbuffers, cpuresources, total_vertex_cnt, Otherdata...);

    size_t basevertex = 0, baseindex = 0;

    for (const irr::scene::IMeshBuffer<S3DVertexFormat>& mesh : meshes)
    {
      memcpy(&vertexmap[basevertex], mesh.getVertices(), sizeof(S3DVertexFormat) * mesh.getVertexCount());
      memcpy(&indexmap[baseindex], mesh.getIndices(), sizeof(unsigned short) * mesh.getIndexCount());
      meshOffset.push_back(std::make_tuple(mesh.getIndexCount(), basevertex, baseindex));
      basevertex += mesh.getVertexCount();
      baseindex += mesh.getIndexCount();
    }

    cpuindexdata->Unmap(0, nullptr);
    for (ID3D12Resource* res : cpuresources)
      res->Unmap(0, nullptr);

    ID3D12CommandAllocator* temporarycommandalloc;
    HRESULT hr = Context::getInstance()->dev->CreateCommandAllocator(queue->GetDesc().Type, IID_PPV_ARGS(&temporarycommandalloc));

    ID3D12GraphicsCommandList *uploadcmdlist;
    hr = Context::getInstance()->dev->CreateCommandList(1, queue->GetDesc().Type, temporarycommandalloc, nullptr, IID_PPV_ARGS(&uploadcmdlist));

    uploadcmdlist->CopyBufferRegion(indexbuffer.Get(), 0, cpuindexdata, 0, total_index_cnt * sizeof(unsigned short), D3D12_COPY_NONE);
    for (unsigned i = 0; i < vertexbuffers.size(); i++)
      uploadcmdlist->CopyBufferRegion(vertexbuffers[i].Get(), 0, cpuresources[i], 0, cpuresources[i]->GetDesc().Width, D3D12_COPY_NONE);

    std::vector<D3D12_RESOURCE_BARRIER_DESC> barriers = {
      setResourceTransitionBarrier(indexbuffer.Get(), D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_GENERIC_READ)
    };
    for (unsigned i = 0; i < vertexbuffers.size(); i++)
      barriers.push_back(setResourceTransitionBarrier(vertexbuffers[i].Get(), D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_GENERIC_READ));

    uploadcmdlist->ResourceBarrier((UINT)barriers.size(), barriers.data());

    uploadcmdlist->Close();
    queue->ExecuteCommandLists(1, (ID3D12CommandList**)&uploadcmdlist);

    std::thread t1([=]() {temporarycommandalloc->Release(); uploadcmdlist->Release(); cpuindexdata->Release(); for (unsigned i = 0; i < cpuresources.size(); i++) cpuresources[i]->Release(); });
    t1.detach();

    idxb.BufferLocation = indexbuffer->GetGPUVirtualAddress();
    idxb.Format = DXGI_FORMAT_R16_UINT;
    idxb.SizeInBytes = (UINT)(total_index_cnt * sizeof(unsigned short));

    D3D12_VERTEX_BUFFER_VIEW newvtxb = {};
    newvtxb.BufferLocation = vertexbuffers[0]->GetGPUVirtualAddress();
    newvtxb.SizeInBytes = (UINT) total_vertex_cnt * sizeof(S3DVertexFormat);
    newvtxb.StrideInBytes = sizeof(S3DVertexFormat);
    vtxb.push_back(newvtxb);
    VertexFiller<AppendedData...>::template BuildDesc<0>(vtxb, total_vertex_cnt, vertexbuffers);
  }

  std::vector<D3D12_VERTEX_BUFFER_VIEW> getVertexBufferView() const
  {
    return vtxb;
  }

  D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const
  {
    return idxb;
  }

};
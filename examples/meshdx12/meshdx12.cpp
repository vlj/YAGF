//#include <API/d3dapi.h>
#include <API/GfxApi.h>
#include <Scene/Shaders.h>

//#include <D3DAPI/D3DRTTSet.h>
//#include <D3DAPI/VAO.h>
//#include <D3DAPI/D3DTexture.h>
#include <Loaders/B3D.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl\client.h>
#include <d3dx12.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

#define CHECK_HRESULT(cmd) {HRESULT hr = cmd; if (hr != 0) throw;}

using command_list_storage_t = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
using command_list_t = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;
using device_t = Microsoft::WRL::ComPtr<ID3D12Device>;
using command_queue_t = Microsoft::WRL::ComPtr<ID3D12CommandQueue>;
using buffer_t = Microsoft::WRL::ComPtr<ID3D12Resource>;
using image_t = Microsoft::WRL::ComPtr<ID3D12Resource>;
using descriptor_storage_t = Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;
using pipeline_state_t = Microsoft::WRL::ComPtr<ID3D12PipelineState>;

static float timer = 0.;

struct Matrixes
{
    float Model[16];
    float ViewProj[16];
};

struct JointTransform
{
    float Model[16 * 48];
};


class WindowUtil
{
private:
    // this is the main message handler for the program
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // sort through and find what code to run for the message given
        switch (message)
        {
            // this message is read when the window is closed
        case WM_DESTROY:
        {
            // close the application entirely
            PostQuitMessage(0);
            return 0;
        } break;
        }

        // Handle any messages the switch statement didn't
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

public:
    static HWND Create(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
    {
        // the handle for the window, filled by a function
        HWND hWnd;
        // this struct holds information for the window class
        WNDCLASSEX wc = {};

        // fill in the struct with the needed information
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = L"WindowClass1";

        // register the window class
        RegisterClassEx(&wc);

        // create the window and use the result as the handle
        hWnd = CreateWindowEx(NULL,
            L"WindowClass1",    // name of the window class
            L"MeshDX12",   // title of the window
            WS_OVERLAPPEDWINDOW,    // window style
            300,    // x-position of the window
            300,    // y-position of the window
            1024,    // width of the window
            1024,    // height of the window
            NULL,    // we have no parent window, NULL
            NULL,    // we aren't using menus, NULL
            hInstance,    // application handle
            NULL);    // used with multiple windows, NULL
        ShowWindow(hWnd, nCmdShow);
        return hWnd;
    }
};

namespace
{
    DXGI_FORMAT getDXGIFormatFromColorFormat(irr::video::ECOLOR_FORMAT fmt)
    {
        switch (fmt)
        {
        case irr::video::ECF_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case irr::video::ECF_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case irr::video::ECF_R16G16B16A16F:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case irr::video::ECF_R32G32B32A32F:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case irr::video::ECF_A8R8G8B8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case irr::video::ECF_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM;
        case irr::video::ECF_BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case irr::video::ECF_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM;
        case irr::video::ECF_BC2_UNORM_SRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case irr::video::ECF_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM;
        case irr::video::ECF_BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case irr::video::ECF_BC4_UNORM:
            return DXGI_FORMAT_BC4_UNORM;
        case irr::video::ECF_BC4_SNORM:
            return DXGI_FORMAT_BC4_SNORM;
        case irr::video::ECF_BC5_UNORM:
            return DXGI_FORMAT_BC5_UNORM;
        case irr::video::ECF_BC5_SNORM:
            return DXGI_FORMAT_BC5_SNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    command_list_storage_t create_command_storage(device_t dev)
    {
        command_list_storage_t result;
        CHECK_HRESULT(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(result.GetAddressOf())));
        return result;
    }

    command_list_t create_command_list(device_t dev, command_list_storage_t storage)
    {
        command_list_t result;
        CHECK_HRESULT(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, storage.Get(), nullptr, IID_PPV_ARGS(result.GetAddressOf())));
        return result;
    }

    buffer_t create_buffer(device_t dev, size_t size)
    {
        buffer_t result;
        size_t real_size = std::max<size_t>(size, 256);
        CHECK_HRESULT(dev->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(real_size),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(result.GetAddressOf())));
        return result;
    }

    void* map_buffer(device_t dev, buffer_t buffer)
    {
        void* result;
        CHECK_HRESULT(buffer->Map(0, nullptr, (void**)&result));
        return result;
    }

    void unmap_buffer(device_t dev, buffer_t buffer)
    {
        buffer->Unmap(0, nullptr);
    }

    image_t create_image(device_t dev, DXGI_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, D3D12_RESOURCE_FLAGS flags)
    {
        image_t result;
        CHECK_HRESULT(dev->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, mipmap, 1, 0, flags),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &CD3DX12_CLEAR_VALUE(format, 1., 0),
            IID_PPV_ARGS(result.GetAddressOf())));
        return result;
    }

    descriptor_storage_t create_descriptor_storage(device_t dev, uint32_t num_descriptors)
    {
        descriptor_storage_t result;
        D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
        heapdesc.NumDescriptors = num_descriptors;
        heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        CHECK_HRESULT(dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(result.GetAddressOf())));
        return result;
    }

    void create_constant_buffer_view(device_t dev, descriptor_storage_t storage, size_t index, buffer_t buffer, uint32_t buffer_size)
    {
        uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = buffer->GetGPUVirtualAddress();
        desc.SizeInBytes = std::max<uint32_t>(256, buffer_size);
        dev->CreateConstantBufferView(&desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(storage->GetCPUDescriptorHandleForHeapStart()).Offset(index, stride));
    }

    descriptor_storage_t create_sampler_heap(device_t dev, uint32_t num_descriptors)
    {
        descriptor_storage_t result;
        D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
        heapdesc.NumDescriptors = num_descriptors;
        heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        CHECK_HRESULT(dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(result.GetAddressOf())));
        return result;
    }

    void create_sampler(device_t dev, descriptor_storage_t storage, size_t index, SAMPLER_TYPE sampler_type)
    {
        uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        D3D12_SAMPLER_DESC samplerdesc = {};

        samplerdesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerdesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerdesc.MaxAnisotropy = 1;
        samplerdesc.MinLOD = 0;
        samplerdesc.MaxLOD = 1000;

        switch (sampler_type)
        {
        case SAMPLER_TYPE::TRILINEAR:
            samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            break;
        case SAMPLER_TYPE::BILINEAR:
            samplerdesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            samplerdesc.MaxLOD = 0;
            break;
        case SAMPLER_TYPE::NEAREST:
            samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            samplerdesc.MaxLOD = 0;
            break;
        case SAMPLER_TYPE::ANISOTROPIC:
            samplerdesc.Filter = D3D12_FILTER_ANISOTROPIC;
            samplerdesc.MaxAnisotropy = 16;
            break;
        }
        dev->CreateSampler(&samplerdesc,CD3DX12_CPU_DESCRIPTOR_HANDLE(storage->GetCPUDescriptorHandleForHeapStart()).Offset(index, stride));
    }

    void create_image_view(device_t dev, descriptor_storage_t storage, uint32_t index, image_t img)
    {
        uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        dev->CreateShaderResourceView(img.Get(), &desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(storage->GetCPUDescriptorHandleForHeapStart()).Offset(index, stride));
    }

    void start_command_list_recording(device_t dev, command_list_t command_list, command_list_storage_t storage)
    {
        command_list->Reset(storage.Get(), nullptr);
    }

    void make_command_list_executable(device_t dev, command_list_t command_list)
    {
        command_list->Close();
    }

    void wait_for_command_queue_idle(device_t dev, command_queue_t command_queue)
    {
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        CHECK_HRESULT(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
        HANDLE completion_event = CreateEvent(0, FALSE, FALSE, 0);
        fence->SetEventOnCompletion(1, completion_event);
        command_queue->Signal(fence.Get(), 1);
        WaitForSingleObject(completion_event, INFINITE);
        CloseHandle(completion_event);
    }

    struct framebuffer_t
    {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtt_heap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;
        size_t NumRTT;
        bool hasDepthStencil;

        framebuffer_t() {}

        framebuffer_t(device_t dev, const std::vector<ID3D12Resource*> &RTTs, ID3D12Resource *DepthStencil)
            : NumRTT(RTTs.size()), hasDepthStencil(DepthStencil != nullptr)
        {
            D3D12_DESCRIPTOR_HEAP_DESC rtt_desc = {};
            rtt_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtt_desc.NumDescriptors = NumRTT;
            CHECK_HRESULT(dev->CreateDescriptorHeap(&rtt_desc, IID_PPV_ARGS(rtt_heap.GetAddressOf())));

            for (unsigned i = 0; i < RTTs.size(); i++)
            {
                D3D12_RENDER_TARGET_VIEW_DESC rttvd = {};
                rttvd.Format = RTTs[i]->GetDesc().Format;
                rttvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rttvd.Texture2D.MipSlice = 0;
                rttvd.Texture2D.PlaneSlice = 0;
                dev->CreateRenderTargetView(RTTs[i], &rttvd, CD3DX12_CPU_DESCRIPTOR_HANDLE(rtt_heap->GetCPUDescriptorHandleForHeapStart()).Offset(i, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)));
            }

            if (DepthStencil == nullptr)
                return;

            D3D12_DESCRIPTOR_HEAP_DESC dsv_desc = {};
            dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            dsv_desc.NumDescriptors = 1;
            CHECK_HRESULT(dev->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(dsv_heap.GetAddressOf())));
            D3D12_DEPTH_STENCIL_VIEW_DESC DSVDescription = {};
            DSVDescription.Format = DepthStencil->GetDesc().Format;
            DSVDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            DSVDescription.Texture2D.MipSlice = 0;
            dev->CreateDepthStencilView(DepthStencil, &DSVDescription, dsv_heap->GetCPUDescriptorHandleForHeapStart());
        }

        ~framebuffer_t()
        {

        }

/*        void Bind(ID3D12GraphicsCommandList *cmdlist)
        {
            D3D12_RECT rect = {};
            rect.left = 0;
            rect.top = 0;
            rect.bottom = (LONG)width;
            rect.right = (LONG)height;

            D3D12_VIEWPORT view = {};
            view.Height = (FLOAT)width;
            view.Width = (FLOAT)height;
            view.TopLeftX = 0;
            view.TopLeftY = 0;
            view.MinDepth = 0;
            view.MaxDepth = 1.;

            cmdlist->RSSetViewports(1, &view);
            cmdlist->RSSetScissorRects(1, &rect);

            if (hasDepthStencil)
                cmdlist->SetRenderTargets(&DescriptorHeap->GetCPUDescriptorHandleForHeapStart(), true, (UINT)NumRTT, &DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
            else
                cmdlist->SetRenderTargets(&DescriptorHeap->GetCPUDescriptorHandleForHeapStart(), true, (UINT)NumRTT, nullptr);
        }

        void Clear(ID3D12GraphicsCommandList *cmdlist, float clearColor[4])
        {
            UINT Increment = Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP);
            for (unsigned i = 0; i < NumRTT; i++)
            {
                cmdlist->ClearRenderTargetView(DescriptorHeap->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(i * Increment), clearColor, 0, 0);
            }
        }

        void ClearDepthStencil(ID3D12GraphicsCommandList *cmdlist, float Depth, unsigned Stencil)
        {
            cmdlist->ClearDepthStencilView(DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_DEPTH, Depth, Stencil, nullptr, 0);
        }*/
    };

/*    {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    result->D3DValue.description.TextureView.DSV = dsv;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format = DXGI_FORMAT_R32_FLOAT;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;
    result->D3DValue.description.TextureView.SRV = srv;

    return result;
    }*/
}

struct MipLevelData
{
    size_t Offset;
    size_t Width;
    size_t Height;
    size_t RowPitch;
};

struct Sample
{
    device_t dev;
    command_queue_t cmdqueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> chain;
    image_t back_buffer[2];
    descriptor_storage_t BackBufferDescriptorsHeap;

    command_list_storage_t command_allocator;
    command_list_t command_list;
    buffer_t cbuffer;
    buffer_t jointbuffer;
    descriptor_storage_t constant_buffer_descriptors;

    descriptor_storage_t TexResourceHeaps;
    std::vector<image_t> Textures;
    std::vector<buffer_t> upload_buffers;
    descriptor_storage_t sampler_heap;
    image_t depth_buffer;

    std::unordered_map<std::string, uint32_t> textureSet;

    pipeline_state_t objectpso;
    framebuffer_t fbo[2];

    irr::scene::CB3DMeshFileLoader *loader;

    void Init()
    {
        command_allocator = create_command_storage(dev);
        command_list = create_command_list(dev, command_allocator);

        cbuffer = create_buffer(dev, sizeof(Matrixes));
        jointbuffer = create_buffer(dev, sizeof(JointTransform));

        constant_buffer_descriptors = create_descriptor_storage(dev, 2);
        create_constant_buffer_view(dev, constant_buffer_descriptors, 0, cbuffer, sizeof(Matrixes));
        create_constant_buffer_view(dev, constant_buffer_descriptors, 1, jointbuffer, sizeof(JointTransform));

        sampler_heap = create_sampler_heap(dev, 1);
        create_sampler(dev, sampler_heap, 0, SAMPLER_TYPE::TRILINEAR);

        depth_buffer = create_image(dev, DXGI_FORMAT_D24_UNORM_S8_UINT, 1024, 1024, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        fbo[0] = framebuffer_t(dev, { back_buffer[0].Get() }, depth_buffer.Get());
        fbo[1] = framebuffer_t(dev, { back_buffer[1].Get() }, depth_buffer.Get());

        irr::io::CReadFile reader("..\\examples\\assets\\xue.b3d");
        loader = new irr::scene::CB3DMeshFileLoader(&reader);
        std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader->AnimatedMesh.getMeshBuffers();
        std::vector<irr::scene::SMeshBufferLightMap> reorg;

        for (auto buf : buffers)
            reorg.push_back(buf.first);

        // Format Weight

        std::vector<std::vector<irr::video::SkinnedVertexData> > weightsList;
        for (auto weightbuffer : loader->AnimatedMesh.WeightBuffers)
        {
            std::vector<irr::video::SkinnedVertexData> weights;
            for (unsigned j = 0; j < weightbuffer.size(); j += 4)
            {
                irr::video::SkinnedVertexData tmp = {
                    weightbuffer[j].Index, weightbuffer[j].Weight,
                    weightbuffer[j + 1].Index, weightbuffer[j + 1].Weight,
                    weightbuffer[j + 2].Index, weightbuffer[j + 2].Weight,
                    weightbuffer[j + 3].Index, weightbuffer[j + 3].Weight,
                };
                weights.push_back(tmp);
            }
            weightsList.push_back(weights);
        }

        size_t total_vertex_cnt = 0, total_index_cnt = 0;
        for (const irr::scene::IMeshBuffer<irr::video::S3DVertex2TCoords>& mesh : reorg)
        {
            total_vertex_cnt += mesh.getVertexCount();
            total_index_cnt += mesh.getIndexCount();
        }
        //vao = new FormattedVertexStorage(Context::getInstance()->cmdqueue.Get(), reorg, weightsList);
        size_t bufferSize = total_vertex_cnt * sizeof(irr::video::S3DVertex2TCoords);

        buffer_t cpuvertexdata = create_buffer(dev, bufferSize);
        void *vertexmap = map_buffer(dev, cpuvertexdata);
        memcpy(vertexmap, reorg.data(), bufferSize);
        unmap_buffer(dev, cpuvertexdata);
        // TODO: Upload to GPUmem
        /*
        hr = Context::getInstance()->dev->CreateCommittedResource(
            &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_MISC_NONE,
            &CD3D12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_USAGE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&vertexbuffers[0]));

        memcpy(vertexmap, meshes.data(), bufferSize);

        ID3D12CommandAllocator* temporarycommandalloc;
        ID3D12GraphicsCommandList *uploadcmdlist;
        hr = Context::getInstance()->dev->CreateCommandAllocator(queue->GetDesc().Type, IID_PPV_ARGS(&temporarycommandalloc));
        hr = Context::getInstance()->dev->CreateCommandList(1, queue->GetDesc().Type, temporarycommandalloc, nullptr, IID_PPV_ARGS(&uploadcmdlist));

        uploadcmdlist->CopyBufferRegion(vertexbuffers[0], 0, cpuvertexdata, 0, bufferSize, D3D12_COPY_NONE);

        D3D12_RESOURCE_BARRIER_DESC barriers = setResourceTransitionBarrier(vertexbuffers[0], D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_GENERIC_READ);
        uploadcmdlist->ResourceBarrier(1, &barriers);

        uploadcmdlist->Close();
        queue->ExecuteCommandLists(1, (ID3D12CommandList**)&uploadcmdlist);

        std::thread t1([=]() {
            HANDLE handle = getCPUSyncHandle(queue);
            WaitForSingleObject(handle, INFINITE);
            CloseHandle(handle);
            temporarycommandalloc->Release();
            uploadcmdlist->Release();
            cpuvertexdata->Release();
        });
        t1.detach();*/

/*        D3D12_VERTEX_BUFFER_VIEW newvtxb = {};
        newvtxb.BufferLocation = vertexbuffers[0]->GetGPUVirtualAddress();
        newvtxb.SizeInBytes = (UINT)bufferSize;
        newvtxb.StrideInBytes = sizeof(S3DVertexFormat);
        vtxb.push_back(newvtxb);*/

        // Upload to gpudata

        // Texture
        TexResourceHeaps = create_descriptor_storage(dev, 1000);
        std::vector<std::tuple<std::string, ID3D12Resource*, D3D12_SHADER_RESOURCE_VIEW_DESC> > textureNamePairs;
        command_list_t uploadcmdlist = command_list;
        uint32_t texture_id = 0;
        for (auto Tex : loader->Textures)
        {
            const std::string &fixed = "..\\examples\\assets\\" + Tex.TextureName.substr(0, Tex.TextureName.find_last_of('.')) + ".DDS";
            std::ifstream DDSFile(fixed, std::ifstream::binary);
            irr::video::CImageLoaderDDS DDSPic(DDSFile);

            uint32_t width = DDSPic.getLoadedImage().Layers[0][0].Width;
            uint32_t height = DDSPic.getLoadedImage().Layers[0][0].Height;
            uint16_t mipmap_count = DDSPic.getLoadedImage().Layers[0].size();
            uint16_t layer_count = 1;

            buffer_t upload_buffer = create_buffer(dev, width * height * 3 * 6);
            upload_buffers.push_back(upload_buffer);
            void *pointer = map_buffer(dev, upload_buffer);

            size_t offset_in_texram = 0;

            size_t block_height = 1;
            size_t block_width = 1;
            size_t block_size = 4;
            std::vector<MipLevelData> Mips;
            for (unsigned face = 0; face < layer_count; face++)
            {
                for (unsigned i = 0; i < mipmap_count; i++)
                {
                    const IImage &image = DDSPic.getLoadedImage();
                    struct PackedMipMapLevel miplevel = image.Layers[face][i];
                    // Offset needs to be aligned to 512 bytes
                    offset_in_texram = (offset_in_texram + 511) & ~512;
                    // Row pitch is always a multiple of 256
                    size_t height_in_blocks = (image.Layers[face][i].Height + block_height - 1) / block_height;
                    size_t width_in_blocks = (image.Layers[face][i].Width + block_width - 1) / block_width;
                    size_t height_in_texram = height_in_blocks * block_height;
                    size_t width_in_texram = width_in_blocks * block_width;
                    size_t rowPitch = width_in_blocks * block_size;
                    rowPitch = (rowPitch + 255) & ~256;
                    MipLevelData mml = { offset_in_texram, width_in_texram, height_in_texram, rowPitch };
                    Mips.push_back(mml);
                    for (unsigned row = 0; row < height_in_blocks; row++)
                    {
                        memcpy(((char*)pointer) + offset_in_texram, ((char*)miplevel.Data) + row * width_in_blocks * block_size, width_in_blocks * block_size);
                        offset_in_texram += rowPitch;
                    }
                }
            }
            unmap_buffer(dev, upload_buffer);

            image_t texture = create_image(dev, getDXGIFormatFromColorFormat(DDSPic.getLoadedImage().Format),
                width, height, mipmap_count, D3D12_RESOURCE_FLAG_NONE);

            Textures.push_back(texture);

            uint32_t miplevel = 0;
            for (const MipLevelData mipmapData : Mips)
            {
                command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST, miplevel));

                command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(texture.Get(), miplevel), 0, 0, 0,
                    &CD3DX12_TEXTURE_COPY_LOCATION(upload_buffer.Get(), { mipmapData.Offset, { getDXGIFormatFromColorFormat(DDSPic.getLoadedImage().Format), (UINT)mipmapData.Width, (UINT)mipmapData.Height, 1, (UINT16)mipmapData.RowPitch } }),
                    &CD3DX12_BOX());

                command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, miplevel));
                miplevel++;
            }

            create_image_view(dev, TexResourceHeaps, texture_id++, texture);
        }
//        objectpso = createSkinnedObjectShader();
        make_command_list_executable(dev, command_list);
        cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)command_list.GetAddressOf());
    }

public:
    Sample(HWND window)
    {
#ifndef NDEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
        D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
        debugInterface->EnableDebugLayer();
#endif //  DEBUG

        Microsoft::WRL::ComPtr<IDXGIFactory> fact;
        CHECK_HRESULT(CreateDXGIFactory(IID_PPV_ARGS(fact.GetAddressOf())));
        Microsoft::WRL::ComPtr<IDXGIAdapter> adaptater;
        CHECK_HRESULT(fact->EnumAdapters(0, adaptater.GetAddressOf()));

        CHECK_HRESULT(D3D12CreateDevice(adaptater.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(dev.GetAddressOf())));

        D3D12_COMMAND_QUEUE_DESC cmddesc = { D3D12_COMMAND_LIST_TYPE_DIRECT };
        CHECK_HRESULT(dev->CreateCommandQueue(&cmddesc, IID_PPV_ARGS(&cmdqueue)));

        DXGI_SWAP_CHAIN_DESC swapChain = {};
        swapChain.BufferCount = 2;
        swapChain.Windowed = true;
        swapChain.OutputWindow = window;
        swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChain.SampleDesc.Count = 1;
        swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;


        CHECK_HRESULT(fact->CreateSwapChain(cmdqueue.Get(), &swapChain, (IDXGISwapChain**)chain.GetAddressOf()));
        CHECK_HRESULT(chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer[0])));
        CHECK_HRESULT(chain->GetBuffer(1, IID_PPV_ARGS(&back_buffer[1])));

        back_buffer[0]->SetName(L"BackBuffer0");
        back_buffer[1]->SetName(L"BackBuffer1");

        Init();
    }

    ~Sample()
    {
        wait_for_command_queue_idle(dev, cmdqueue);
    }

    void Draw()
    {
        start_command_list_recording(dev, command_list, command_allocator);
        Matrixes *cbufdata = static_cast<Matrixes*>(map_buffer(dev, cbuffer));
        irr::core::matrix4 Model, View;
        View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
        Model.setTranslation(irr::core::vector3df(0.f, 0.f, 2.f));
        Model.setRotationDegrees(irr::core::vector3df(0.f, timer / 360.f, 0.f));

        memcpy(cbufdata->Model, Model.pointer(), 16 * sizeof(float));
        memcpy(cbufdata->ViewProj, View.pointer(), 16 * sizeof(float));
        unmap_buffer(dev, cbuffer);

        timer += 16.f;

        double intpart;
        float frame = (float)modf(timer / 10000., &intpart);
        frame *= 300.f;
        loader->AnimatedMesh.animateMesh(frame, 1.f);
        loader->AnimatedMesh.skinMesh(1.f);

        memcpy(map_buffer(dev, jointbuffer), loader->AnimatedMesh.JointMatrixes.data(), loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float));
        unmap_buffer(dev, jointbuffer);

        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(back_buffer[chain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

        float clearColor[] = { .25f, .25f, 0.35f, 1.0f };
        command_list->ClearRenderTargetView(fbo[chain->GetCurrentBackBufferIndex()].rtt_heap->GetCPUDescriptorHandleForHeapStart(), clearColor, 0, nullptr);
//        fbo[chain->GetCurrentBackBufferIndex()]->Clear(cmdlist->D3DValue.CommandList, clearColor);
//        fbo[Context::getInstance()->getCurrentBackBufferIndex()]->ClearDepthStencil(cmdlist->D3DValue.CommandList, 1.f, 0);

//        fbo[Context::getInstance()->getCurrentBackBufferIndex()]->Bind(cmdlist->D3DValue.CommandList);
//        command_list->SetPipelineState(objectpso.Get());

        std::array<ID3D12DescriptorHeap*, 2> descriptors = {constant_buffer_descriptors.Get(), sampler_heap.Get()};
        command_list->SetDescriptorHeaps(2, descriptors.data());

//        command_list->IASetIndexBuffer(&vao->getIndexBufferView());
//        command_list->IASetVertexBuffers(0, &vao->getVertexBufferView()[0], 1);
//        command_list->IASetVertexBuffers(1, &vao->getVertexBufferView()[1], 1);

        std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader->AnimatedMesh.getMeshBuffers();
        for (unsigned i = 0; i < buffers.size(); i++)
        {
//            command_list->SetGraphicsRootDescriptorTable(1, textureSet[buffers[i].second.TextureNames[0]]);
//            GlobalGFXAPI->drawIndexedInstanced(cmdlist, std::get<0>(vao->meshOffset[i]), 1, std::get<2>(vao->meshOffset[i]), std::get<1>(vao->meshOffset[i]), 0);
        }

        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(back_buffer[chain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
        make_command_list_executable(dev, command_list);
        cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)command_list.GetAddressOf());
        wait_for_command_queue_idle(dev, cmdqueue);
        CHECK_HRESULT(chain->Present(1, 0));
    }
};



GFXAPI *GlobalGFXAPI;

int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  Sample app(WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow));

  // this struct holds Windows event messages
  MSG msg = {};

  // Loop from https://msdn.microsoft.com/en-us/library/windows/apps/dn166880.aspx
  while (WM_QUIT != msg.message)
  {
    bool bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

    if (bGotMsg)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    app.Draw();
  }

  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}

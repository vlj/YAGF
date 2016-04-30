// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <Scene/IBL.h>
#include <Maths/matrix4.h>
#include <cmath>
#include <set>
#include <d3dcompiler.h>

namespace
{
	constexpr auto object_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 1, 1),
		range_of_descriptors(RESOURCE_VIEW::UAV, 2, 1) },

		shader_stage::all);

	constexpr auto sampler_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1) },
		shader_stage::fragment_shader);

	pipeline_state_t get_compute_sh_pipeline_state(device_t* dev, pipeline_layout_t pipeline_layout)
	{
#ifdef D3D12
		ID3D12PipelineState* result;
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		CHECK_HRESULT(D3DReadFileToBlob(L"computesh.cso", blob.GetAddressOf()));

		D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};
		pipeline_desc.CS.BytecodeLength = blob->GetBufferSize();
		pipeline_desc.CS.pShaderBytecode = blob->GetBufferPointer();
		pipeline_desc.pRootSignature = pipeline_layout.Get();

		CHECK_HRESULT(dev->object->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&result)));
		return result;
#endif // D3D12
	}

}

struct SHCoefficients
{
	float Red[9];
	float Green[9];
	float Blue[9];
};


std::unique_ptr<buffer_t> computeSphericalHarmonics(device_t* dev, command_queue_t* cmd_queue, image_t *probe, size_t edge_size)
{

  std::unique_ptr<command_list_storage_t> command_storage = create_command_storage(dev);
  std::unique_ptr<command_list_t> command_list = create_command_list(dev, command_storage.get());

  start_command_list_recording(dev, command_list.get(), command_storage.get());
  std::unique_ptr<buffer_t> cbuf = create_buffer(dev, sizeof(int), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
  void* tmp = map_buffer(dev, cbuf.get());
  float cube_size = (float)edge_size / 10.;
  memcpy(tmp, &cube_size, sizeof(int));
  unmap_buffer(dev, cbuf.get());
#ifdef D3D12
  auto compute_sh_sig = get_pipeline_layout_from_desc(dev, { object_descriptor_set_type, sampler_descriptor_set_type });
#else

#endif
  pipeline_state_t compute_sh_pso = get_compute_sh_pipeline_state(dev, compute_sh_sig);
  std::unique_ptr<descriptor_storage_t> srv_cbv_uav_heap = create_descriptor_storage(dev, 1, { { RESOURCE_VIEW::CONSTANTS_BUFFER, 1 }, { RESOURCE_VIEW::SHADER_RESOURCE, 1}, { RESOURCE_VIEW::UAV, 1} });
  std::unique_ptr<descriptor_storage_t> sampler_heap = create_descriptor_storage(dev, 1, { { RESOURCE_VIEW::SAMPLER, 1 } });

  std::unique_ptr<buffer_t> sh_buffer = create_buffer(dev, sizeof(SH), irr::video::E_MEMORY_POOL::EMP_GPU_LOCAL, usage_uav);
  std::unique_ptr<buffer_t> sh_buffer_readback = create_buffer(dev, sizeof(SH), irr::video::E_MEMORY_POOL::EMP_CPU_READABLE, none);

#ifdef D3D12
  create_constant_buffer_view(dev, srv_cbv_uav_heap.get(), 0, cbuf.get(), sizeof(int));
  create_image_view(dev, srv_cbv_uav_heap.get(), 1, probe, 9, irr::video::ECF_BC1_UNORM_SRGB, D3D12_SRV_DIMENSION_TEXTURECUBE);

  D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
  desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  desc.Format = DXGI_FORMAT_R32_TYPELESS;
//  desc.Buffer.StructureByteStride = sizeof(SH);
  desc.Buffer.NumElements = sizeof(SH) / 4;
  desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

  dev->object->CreateUnorderedAccessView(sh_buffer->object, nullptr, &desc,
	CD3DX12_CPU_DESCRIPTOR_HANDLE(srv_cbv_uav_heap->object->GetCPUDescriptorHandleForHeapStart())
	  .Offset(2, dev->object->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

  command_list->object->SetPipelineState(compute_sh_pso.Get());
  command_list->object->SetComputeRootSignature(compute_sh_sig.Get());
  std::array<ID3D12DescriptorHeap*, 2> heaps = { srv_cbv_uav_heap->object, sampler_heap->object };
  command_list->object->SetDescriptorHeaps(heaps.size(), heaps.data());
  command_list->object->SetComputeRootDescriptorTable(0, srv_cbv_uav_heap->object->GetGPUDescriptorHandleForHeapStart());
  command_list->object->SetComputeRootDescriptorTable(1, sampler_heap->object->GetGPUDescriptorHandleForHeapStart());

  command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
  command_list->object->Dispatch(1, 1, 1);
  command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
  command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(sh_buffer->object));
  command_list->object->CopyBufferRegion(sh_buffer_readback->object, 0, sh_buffer->object, 0, sizeof(SH));
#else

#endif
  make_command_list_executable(command_list.get());
  submit_executable_command_list(cmd_queue, command_list.get());
  // for debug
  wait_for_command_queue_idle(dev, cmd_queue);
  SHCoefficients Result;
  float* Shval = (float*)map_buffer(dev, sh_buffer_readback.get());
  memcpy(Result.Blue, Shval, 9 * sizeof(float));
  memcpy(Result.Green, &Shval[9], 9 * sizeof(float));
  memcpy(Result.Red, &Shval[18], 9 * sizeof(float));
#ifdef D3D12



#else

#endif


  return std::move(sh_buffer);
}

#if 0

// From http://http.developer.nvidia.com/GPUGems3/gpugems3_ch20.html
/** Returns the index-th pair from Hammersley set of pseudo random set.
Hammersley set is a uniform distribution between 0 and 1 for 2 components.
We use the natural indexation on the set to avoid storing the whole set.
\param index of the pair
\param size of the set. */
std::pair<float, float> HammersleySequence(int index, int samples)
{
  float InvertedBinaryRepresentation = 0.;
  for (size_t i = 0; i < 32; i++)
  {
    InvertedBinaryRepresentation += ((index >> i) & 0x1) * powf(.5, (float)(i + 1.));
  }
  return std::make_pair(float(index) / float(samples), InvertedBinaryRepresentation);
}


/** Returns a pseudo random (theta, phi) generated from a probability density function modeled after Phong function.
\param a pseudo random float pair from a uniform density function between 0 and 1.
\param exponent from the Phong formula. */
std::pair<float, float> ImportanceSamplingPhong(std::pair<float, float> Seeds, float exponent)
{
  return std::make_pair(acosf(powf(Seeds.first, 1.f / (exponent + 1.f))), 2.f * 3.14f * Seeds.second);
}

/** Returns a pseudo random (theta, phi) generated from a probability density function modeled after GGX distribtion function.
From "Real Shading in Unreal Engine 4" paper
\param a pseudo random float pair from a uniform density function between 0 and 1.
\param exponent from the Phong formula. */
std::pair<float, float> ImportanceSamplingGGX(std::pair<float, float> Seeds, float roughness)
{
  float a = roughness * roughness;
  float CosTheta = sqrtf((1.f - Seeds.second) / (1.f + (a * a - 1.f) * Seeds.second));
  return std::make_pair(acosf(CosTheta), 2.f * 3.14f * Seeds.first);
}

/** Returns a pseudo random (theta, phi) generated from a probability density function modeled after cos distribtion function.
\param a pseudo random float pair from a uniform density function between 0 and 1. */
std::pair<float, float> ImportanceSamplingCos(std::pair<float, float> Seeds)
{
  return std::make_pair(acosf(Seeds.first), 2.f * 3.14f * Seeds.second);
}

static
irr::core::matrix4 getPermutationMatrix(size_t indexX, float valX, size_t indexY, float valY, size_t indexZ, float valZ)
{
  irr::core::matrix4 resultMat;
  float *M = resultMat.pointer();
  memset(M, 0, 16 * sizeof(float));
  assert(indexX < 4);
  assert(indexY < 4);
  assert(indexZ < 4);
  M[indexX] = valX;
  M[4 + indexY] = valY;
  M[8 + indexZ] = valZ;
  return resultMat;
}

struct PermutationMatrix
{
  float Matrix[16];
};

WrapperResource *generateSpecularCubemap(WrapperResource *probe)
{
  size_t cubemap_size = 256;
  WrapperResource *result = (WrapperResource*)malloc(sizeof(WrapperResource));

  WrapperCommandList *CommandList = GlobalGFXAPI->createCommandList();
  WrapperPipelineState *PSO = ImportanceSamplingForSpecularCubemap();
  WrapperIndexVertexBuffersSet *bigtri = GlobalGFXAPI->createFullscreenTri();

  WrapperResource *cbuf[6];
  for(unsigned i = 0; i < 6; i++)
    cbuf[i] = GlobalGFXAPI->createConstantsBuffer(sizeof(PermutationMatrix));
  WrapperDescriptorHeap *probeheap = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({ std::make_tuple(probe, RESOURCE_VIEW::SHADER_RESOURCE, 0) });
  WrapperDescriptorHeap *cbufheap[6];
  for (unsigned i = 0; i < 6; i++)
    cbufheap[i] = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({ std::make_tuple(cbuf[i], RESOURCE_VIEW::CONSTANTS_BUFFER, 0) });
  WrapperDescriptorHeap *samplers = GlobalGFXAPI->createSamplerHeap({std::make_pair(SAMPLER_TYPE::ANISOTROPIC, 0)});

#ifdef GLBUILD
  result->GLValue.Type = GL_TEXTURE_CUBE_MAP;
  glGenTextures(1, &result->GLValue.Resource);
  glBindTexture(GL_TEXTURE_CUBE_MAP, result->GLValue.Resource);
  for (int i = 0; i < 6; i++)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, (GLsizei)cubemap_size, (GLsizei)cubemap_size, 0, GL_BGRA, GL_FLOAT, 0);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, (GLsizei)cubemap_size, (GLsizei)cubemap_size);
  GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, bufs);
#endif

#if DXBUILD
  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)cubemap_size, (UINT)cubemap_size, 6, 8, 1, 0, D3D12_RESOURCE_MISC_ALLOW_RENDER_TARGET),
    D3D12_RESOURCE_USAGE_RENDER_TARGET,
    nullptr,
    IID_PPV_ARGS(&result->D3DValue.resource)
    );
  result->D3DValue.description.TextureView.SRV = {};
  result->D3DValue.description.TextureView.SRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  result->D3DValue.description.TextureView.SRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
  result->D3DValue.description.TextureView.SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  result->D3DValue.description.TextureView.SRV.TextureCube.MipLevels = 8;


  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CubeFaceRTT;

  {
    D3D12_DESCRIPTOR_HEAP_DESC dh = {};
    dh.Type = D3D12_RTV_DESCRIPTOR_HEAP;
    dh.NumDescriptors = 6 * 8;
    hr = Context::getInstance()->dev->CreateDescriptorHeap(&dh, IID_PPV_ARGS(&CubeFaceRTT));
  }

  size_t index = 0, increment = Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP);
  for (unsigned mipmaplevel = 0; mipmaplevel < 8; mipmaplevel++)
  {
    for (unsigned face = 0; face < 6; face++)
    {
      D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
      rtv.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
      rtv.Texture2DArray.ArraySize = 1;
      rtv.Texture2DArray.MipSlice = mipmaplevel;
      rtv.Texture2DArray.FirstArraySlice = face;
      Context::getInstance()->dev->CreateRenderTargetView(result->D3DValue.resource, &rtv, CubeFaceRTT->GetCPUDescriptorHandleForHeapStart().MakeOffsetted((INT)(index * increment)));
      index++;
    }
  }
  index = 0;
#endif

  GlobalGFXAPI->openCommandList(CommandList);
  GlobalGFXAPI->setPipelineState(CommandList, PSO);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 3, samplers);

  irr::core::matrix4 M[6] = {
    getPermutationMatrix(2, -1., 1, -1., 0, 1.),
    getPermutationMatrix(2, 1., 1, -1., 0, -1.),
    getPermutationMatrix(0, 1., 2, 1., 1, 1.),
    getPermutationMatrix(0, 1., 2, -1., 1, -1.),
    getPermutationMatrix(0, 1., 1, -1., 2, 1.),
    getPermutationMatrix(0, -1., 1, -1., 2, -1.),
  };

  for (unsigned i = 0; i < 6; i++)
  {
    memcpy(GlobalGFXAPI->mapConstantsBuffer(cbuf[i]), M[i].pointer(), 16 * sizeof(float));
    GlobalGFXAPI->unmapConstantsBuffers(cbuf[i]);
  }

#ifdef DXBUILD
  Microsoft::WRL::ComPtr<ID3D12Resource> tbuffer[8];
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> tbufferheap[8];
  for (unsigned i = 0; i < 8; i++)
  {
    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(1024 * 2 * sizeof(float)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&tbuffer[i])
      );

    D3D12_DESCRIPTOR_HEAP_DESC dh = {};
    dh.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
    dh.NumDescriptors = 1;
    dh.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
    hr = Context::getInstance()->dev->CreateDescriptorHeap(&dh, IID_PPV_ARGS(&tbufferheap[i]));
    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv.Buffer.FirstElement = 0;
    srv.Buffer.NumElements = 1024;
    srv.Buffer.StructureByteStride = 2 * sizeof(float);
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    Context::getInstance()->dev->CreateShaderResourceView(tbuffer[i].Get(), &srv, tbufferheap[i]->GetCPUDescriptorHandleForHeapStart());
  }
#endif

  for (unsigned level = 0; level < 8; level++)
  {
    float roughness = .05f + .95f * level / 8.f;
    float viewportSize = float(1 << (8 - level));

    float *tmp = new float[2048];
    for (unsigned i = 0; i < 1024; i++)
    {
      std::pair<float, float> sample = ImportanceSamplingGGX(HammersleySequence(i, 1024), roughness);
      tmp[2 * i] = sample.first;
      tmp[2 * i + 1] = sample.second;
    }

#ifdef GLBUILD
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE1);
    GLuint sampleTex, sampleBuffer;
    glGenBuffers(1, &sampleBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, sampleBuffer);
    glBufferData(GL_TEXTURE_BUFFER, 2048 * sizeof(float), tmp, GL_STATIC_DRAW);
    glGenTextures(1, &sampleTex);
    glBindTexture(GL_TEXTURE_BUFFER, sampleTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, sampleBuffer);
#endif

#ifdef DXBUILD
    D3D12_RECT rect = {};
    rect.left = 0;
    rect.top = 0;
    rect.bottom = (LONG)viewportSize;
    rect.right = (LONG)viewportSize;

    D3D12_VIEWPORT view = {};
    view.Height = (FLOAT)viewportSize;
    view.Width = (FLOAT)viewportSize;
    view.TopLeftX = 0;
    view.TopLeftY = 0;
    view.MinDepth = 0;
    view.MaxDepth = 1.;

    void *tbuffermap;
    tbuffer[level]->Map(0, nullptr, &tbuffermap);
    memcpy(tbuffermap, tmp, 2048 * sizeof(float));
    tbuffer[level]->Unmap(0, nullptr);

    CommandList->D3DValue.CommandList->RSSetViewports(1, &view);
    CommandList->D3DValue.CommandList->RSSetScissorRects(1, &rect);
    CommandList->D3DValue.CommandList->SetGraphicsRootDescriptorTable(1, tbufferheap[level]->GetGPUDescriptorHandleForHeapStart());
#endif

    GlobalGFXAPI->setDescriptorHeap(CommandList, 2, probeheap);
    GlobalGFXAPI->setIndexVertexBuffersSet(CommandList, bigtri);
    for (unsigned face = 0; face < 6; face++)
    {
#ifdef GLBUILD
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, result->GLValue.Resource, level);
      glViewport(0, 0, (GLsizei)viewportSize, (GLsizei)viewportSize);
      GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      assert(status == GL_FRAMEBUFFER_COMPLETE);
#endif

#ifdef DXBUILD
      CommandList->D3DValue.CommandList->SetRenderTargets(&CubeFaceRTT->GetCPUDescriptorHandleForHeapStart().MakeOffsetted((INT)(index * increment)), true, 1, nullptr);
      index++;
#endif
      GlobalGFXAPI->setDescriptorHeap(CommandList, 0, cbufheap[face]);
      GlobalGFXAPI->drawInstanced(CommandList, 3, 1, 0, 0);
    }

#ifdef GLBUILD
    glActiveTexture(GL_TEXTURE1);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glDeleteTextures(1, &sampleTex);
    glDeleteBuffers(1, &sampleBuffer);
#endif

    delete[] tmp;
  }

  GlobalGFXAPI->writeResourcesTransitionBarrier(CommandList, { std::make_tuple(result, RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC) });
  GlobalGFXAPI->closeCommandList(CommandList);
  GlobalGFXAPI->submitToQueue(CommandList);


  GlobalGFXAPI->releaseCommandList(CommandList);
  GlobalGFXAPI->releasePSO(PSO);
  GlobalGFXAPI->releaseIndexVertexBuffersSet(bigtri);
  for (unsigned i = 0; i < 6; i++)
  {
    GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(cbufheap[i]);
    GlobalGFXAPI->releaseConstantsBuffers(cbuf[i]);
  }
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(probeheap);
  GlobalGFXAPI->releaseSamplerHeap(samplers);

#ifdef GLBUILD
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
#endif
  return result;
}

static float G1_Schlick(const irr::core::vector3df &V, const irr::core::vector3df &normal, float k)
{
  float NdotV = V.dotProduct(normal);
  NdotV = NdotV > 0.f ? NdotV : 0.f;
  NdotV = NdotV < 1.f ? NdotV : 1.f;
  return 1.f / (NdotV * (1.f - k) + k);
}

float G_Smith(const irr::core::vector3df &lightdir, const irr::core::vector3df &viewdir, const irr::core::vector3df &normal, float roughness)
{
  float k = (roughness + 1.f) * (roughness + 1.f) / 8.f;
  return G1_Schlick(lightdir, normal, k) * G1_Schlick(viewdir, normal, k);
}

static
std::pair<float, float> getSpecularDFG(float roughness, float NdotV)
{
  // We assume a local referential where N points in Y direction
  irr::core::vector3df V(sqrtf(1.f - NdotV * NdotV), NdotV, 0.f);

  float DFG1 = 0., DFG2 = 0.;
  for (unsigned sample = 0; sample < 1024; sample++)
  {
    std::pair<float, float> ThetaPhi = ImportanceSamplingGGX(HammersleySequence(sample, 1024), roughness);
    float Theta = ThetaPhi.first, Phi = ThetaPhi.second;
    irr::core::vector3df H(sinf(Theta) * cosf(Phi), cosf(Theta), sinf(Theta) * sinf(Phi));
    irr::core::vector3df L = 2 * H.dotProduct(V) * H - V;
    float NdotL = L.Y;

    float NdotH = H.Y > 0. ? H.Y : 0.;
    if (NdotL > 0.)
    {
      float VdotH = V.dotProduct(H);
      VdotH = VdotH > 0.f ? VdotH : 0.f;
      VdotH = VdotH < 1.f ? VdotH : 1.f;
      float Fc = powf(1.f - VdotH, 5.f);
      float G = G_Smith(L, V, irr::core::vector3df(0.f, 1.f, 0.f), roughness);
      DFG1 += (1.f - Fc) * G * VdotH;
      DFG2 += Fc * G * VdotH;
    }
  }
  return std::make_pair(DFG1 / 1024, DFG2 / 1024);
}

static
float getDiffuseDFG(float roughness, float NdotV)
{
  // We assume a local referential where N points in Y direction
  irr::core::vector3df V(sqrtf(1.f - NdotV * NdotV), NdotV, 0.f);
  float DFG = 0.f;
  for (unsigned sample = 0; sample < 1024; sample++)
  {
    std::pair<float, float> ThetaPhi = ImportanceSamplingCos(HammersleySequence(sample, 1024));
    float Theta = ThetaPhi.first, Phi = ThetaPhi.second;
    irr::core::vector3df L(sinf(Theta) * cosf(Phi), cosf(Theta), sinf(Theta) * sinf(Phi));
    float NdotL = L.Y;
    if (NdotL > 0.f)
    {
      irr::core::vector3df H = (L + V).normalize();
      float LdotH = L.dotProduct(H);
      float f90 = .5f + 2.f * LdotH * LdotH * roughness * roughness;
      DFG += (1.f + (f90 - 1.f) * (1.f - powf(NdotL, 5.f))) * (1.f + (f90 - 1.f) * (1.f - powf(NdotV, 5.f)));
    }
  }
  return DFG / 1024;
}

/** Generate the Look Up Table for the DFG texture.
    DFG Texture is used to compute diffuse and specular response from environmental lighting. */
IImage getDFGLUT(size_t DFG_LUT_size)
{
  IImage DFG_LUT_texture;
  DFG_LUT_texture.Format = irr::video::ECF_R32G32B32A32F;
  DFG_LUT_texture.Type = TextureType::TEXTURE2D;
  float *texture_content = new float[4 * DFG_LUT_size * DFG_LUT_size];

  PackedMipMapLevel LUT = {
    DFG_LUT_size,
    DFG_LUT_size,
    texture_content,
    4 * DFG_LUT_size * DFG_LUT_size * sizeof(float)
  };
  DFG_LUT_texture.Layers.push_back({ LUT });

#pragma omp parallel for
  for (int i = 0; i < int(DFG_LUT_size); i++)
  {
    float roughness = .05f + .95f * float(i) / float(DFG_LUT_size - 1);
    for (unsigned j = 0; j < DFG_LUT_size; j++)
    {
      float NdotV = float(1 + j) / float(DFG_LUT_size + 1);
      std::pair<float, float> DFG = getSpecularDFG(roughness, NdotV);
      texture_content[4 * (i * DFG_LUT_size + j)] = DFG.first;
      texture_content[4 * (i * DFG_LUT_size + j) + 1] = DFG.second;
      texture_content[4 * (i * DFG_LUT_size + j) + 2] = getDiffuseDFG(roughness, NdotV);
      texture_content[4 * (i * DFG_LUT_size + j) + 3] = 0.;
    }
  }

  return DFG_LUT_texture;
}
#endif


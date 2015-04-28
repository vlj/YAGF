// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <Scene/IBL.h>
#include <Scene/Shaders.h>
#include <Core/BasicVertexLayout.h>
#include <Maths/matrix4.h>
#include <cmath>
#include <set>


enum CubeFace
{
  POSITIVE_X = 0,
  NEGATIVE_X,
  POSITIVE_Y,
  NEGATIVE_Y,
  POSITIVE_Z,
  NEGATIVE_Z
};


static
irr::core::vector3df vectorFromIndex(CubeFace face, float i, float j)
{
  float x, y, z;
  switch (face)
  {
  case POSITIVE_X:
    x = 1.;
    y = -i;
    z = -j;
    break;
  case NEGATIVE_X:
    x = -1.;
    y = -i;
    z = j;
    break;
  case POSITIVE_Y:
    x = j;
    y = 1.;
    z = i;
    break;
  case NEGATIVE_Y:
    x = j;
    y = -1;
    z = -i;
    break;
  case POSITIVE_Z:
    x = j;
    y = -i;
    z = 1;
    break;
  case NEGATIVE_Z:
    x = -j;
    y = -i;
    z = -1;
    break;
  }

  float norm = sqrt(x * x + y * y + z * z);
  x /= norm, y /= norm, z /= norm;
  return irr::core::vector3df(x, y, z);
}

struct YmlModuledBySolidAngleCoefficientsTable
{
  float *Y00;
  float *Y1minus1;
  float *Y10;
  float *Y11;
  float *Y2minus2;
  float *Y2minus1;
  float *Y20;
  float *Y21;
  float *Y22;

  YmlModuledBySolidAngleCoefficientsTable(size_t cubeEdge)
  {
    Y00 = new float[cubeEdge * cubeEdge];
    Y1minus1 = new float[cubeEdge * cubeEdge];
    Y10 = new float[cubeEdge * cubeEdge];
    Y11 = new float[cubeEdge * cubeEdge];
    Y2minus2 = new float[cubeEdge * cubeEdge];
    Y2minus1 = new float[cubeEdge * cubeEdge];
    Y20 = new float[cubeEdge * cubeEdge];
    Y21 = new float[cubeEdge * cubeEdge];
    Y22 = new float[cubeEdge * cubeEdge];
  }

  ~YmlModuledBySolidAngleCoefficientsTable()
  {
    delete[] Y00;
    delete[] Y1minus1;
    delete[] Y10;
    delete[] Y11;
    delete[] Y2minus2;
    delete[] Y2minus1;
    delete[] Y20;
    delete[] Y21;
    delete[] Y22;
  }

  YmlModuledBySolidAngleCoefficientsTable(const YmlModuledBySolidAngleCoefficientsTable&) = delete;
  YmlModuledBySolidAngleCoefficientsTable& operator=(const YmlModuledBySolidAngleCoefficientsTable&) = delete;
};

static
YmlModuledBySolidAngleCoefficientsTable *getYmlModuledBySolidAngle(CubeFace face, size_t edge_size)
{
  YmlModuledBySolidAngleCoefficientsTable *Res = new YmlModuledBySolidAngleCoefficientsTable(edge_size);
  const YmlModuledBySolidAngleCoefficientsTable &Result = *Res;
#pragma omp parallel for
  for (int i = 0; i < int(edge_size); i++)
  {
    for (unsigned j = 0; j < edge_size; j++)
    {
      float fi = float(i), fj = float(j);
      fi /= edge_size, fj /= edge_size;
      fi = 2 * fi - 1, fj = 2 * fj - 1;
      irr::core::vector3df &V = vectorFromIndex(face, fi, fj);

      float d = sqrt(fi * fi + fj * fj + 1);
      // Constant obtained by projecting unprojected ref values
      float solidangle = 2.75f / (edge_size * edge_size * pow(d, 1.5f));

      // constant part of Ylm
      float c00 = 0.282095f;
      float c1minus1 = 0.488603f;
      float c10 = 0.488603f;
      float c11 = 0.488603f;
      float c2minus2 = 1.092548f;
      float c2minus1 = 1.092548f;
      float c21 = 1.092548f;
      float c20 = 0.315392f;
      float c22 = 0.546274f;

      size_t idx = i * edge_size + j;

      Result.Y00[idx] = c00 * solidangle;
      Result.Y1minus1[idx] = c1minus1 * V.Y  * solidangle;
      Result.Y10[idx] = c10 * V.Z  * solidangle;
      Result.Y11[idx] = c11 * V.X  * solidangle;
      Result.Y2minus2[idx] = c2minus2 * V.X * V.Y  * solidangle;
      Result.Y2minus1[idx] = c2minus1 * V.Y * V.Z  * solidangle;
      Result.Y21[idx] = c21 * V.X * V.Z  * solidangle;
      Result.Y20[idx] = c20 * (3 * V.Z * V.Z - 1) * solidangle;
      Result.Y22[idx] = c22 * (V.X * V.X - V.Y * V.Y)  * solidangle;
    }
  }
  return Res;
}


SHCoefficients computeSphericalHarmonics(const IImage &cubemap, size_t edge_size)
{
  assert(cubemap.Type == TextureType::CUBEMAP);
  SHCoefficients Result = {};

  float b0 = 0., b1 = 0., b2 = 0., b3 = 0., b4 = 0., b5 = 0., b6 = 0., b7 = 0., b8 = 0.;
  float r0 = 0., r1 = 0., r2 = 0., r3 = 0., r4 = 0., r5 = 0., r6 = 0., r7 = 0., r8 = 0.;
  float g0 = 0., g1 = 0., g2 = 0., g3 = 0., g4 = 0., g5 = 0., g6 = 0., g7 = 0., g8 = 0.;
  for (size_t face = 0; face < 6; face++)
  {
    YmlModuledBySolidAngleCoefficientsTable *tmpyml = getYmlModuledBySolidAngle((CubeFace)face, edge_size);
    const YmlModuledBySolidAngleCoefficientsTable &Yml = *tmpyml;

#pragma omp parallel for reduction(+ : b0, b1, b2, b3, b4, b5, b6, b7, b8, r0, r1, r2, r3, r4, r5, r6, r7, r8, g0, g1, g2, g3, g4, g5, g6, g7, g8)
    for (int i = 0; i < int(edge_size); i++)
    {
      for (size_t j = 0; j < edge_size; j++)
      {
        int idx = i * edge_size + j;

        char *tmp = (char*)cubemap.Layers[face][0].Data;
        float b = (float)tmp[4 * idx] / 255;
        float g = (float)tmp[4 * idx + 1] / 255;
        float r = (float)tmp[4 * idx + 2] / 255;

        b0 += b * Yml.Y00[idx];
        b1 += b * Yml.Y1minus1[idx];
        b2 += b * Yml.Y10[idx];
        b3 += b * Yml.Y11[idx];
        b4 += b * Yml.Y2minus2[idx];
        b5 += b * Yml.Y2minus1[idx];
        b6 += b * Yml.Y20[idx];
        b7 += b * Yml.Y21[idx];
        b8 += b * Yml.Y22[idx];

        g0 += g * Yml.Y00[idx];
        g1 += g * Yml.Y1minus1[idx];
        g2 += g * Yml.Y10[idx];
        g3 += g * Yml.Y11[idx];
        g4 += g * Yml.Y2minus2[idx];
        g5 += g * Yml.Y2minus1[idx];
        g6 += g * Yml.Y20[idx];
        g7 += g * Yml.Y21[idx];
        g8 += g * Yml.Y22[idx];


        r0 += r * Yml.Y00[idx];
        r1 += r * Yml.Y1minus1[idx];
        r2 += r * Yml.Y10[idx];
        r3 += r * Yml.Y11[idx];
        r4 += r * Yml.Y2minus2[idx];
        r5 += r * Yml.Y2minus1[idx];
        r6 += r * Yml.Y20[idx];
        r7 += r * Yml.Y21[idx];
        r8 += r * Yml.Y22[idx];
      }
    }
  }

  Result.Blue[0] = b0;
  Result.Blue[1] = b1;
  Result.Blue[2] = b2;
  Result.Blue[3] = b3;
  Result.Blue[4] = b4;
  Result.Blue[5] = b5;
  Result.Blue[6] = b6;
  Result.Blue[7] = b7;
  Result.Blue[8] = b8;

  Result.Red[0] = r0;
  Result.Red[1] = r1;
  Result.Red[2] = r2;
  Result.Red[3] = r3;
  Result.Red[4] = r4;
  Result.Red[5] = r5;
  Result.Red[6] = r6;
  Result.Red[7] = r7;
  Result.Red[8] = r8;

  Result.Green[0] = g0;
  Result.Green[1] = g1;
  Result.Green[2] = g2;
  Result.Green[3] = g3;
  Result.Green[4] = g4;
  Result.Green[5] = g5;
  Result.Green[6] = g6;
  Result.Green[7] = g7;
  Result.Green[8] = g8;

  return Result;
}

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

#ifdef GLBUILD
#include <API/glapi.h>
#endif
#ifdef DXBUILD
#include <API/d3dapi.h>
#endif

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
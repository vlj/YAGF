#include <MeshSceneNode.h>
#include <MeshManager.h>


#ifdef GLBUILD
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Core/VAO.h>
#include <GLAPI/S3DVertex.h>

#include <Core/Shaders.h>
#include <Core/FBO.h>
#include <Util/Misc.h>
#include <Util/Samplers.h>
#include <Util/Text.h>
#include <Util/Debug.h>
#endif

#ifdef DXBUILD
#include <D3DAPI/Context.h>
#include <D3DAPI/RootSignature.h>

#include <D3DAPI/Misc.h>
#include <D3DAPI/VAO.h>
#include <D3DAPI/S3DVertex.h>
#include <Loaders/B3D.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <D3DAPI/PSO.h>
#include <D3DAPI/Resource.h>
#include <D3DAPI/ConstantBuffer.h>


#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#endif

#ifdef GLBUILD
FrameBuffer *MainFBO;
FrameBuffer *LinearDepthFBO;

GLuint TrilinearSampler;

const char *vtxshader = TO_STRING(
  \#version 330 \n

layout(std140) uniform ObjectData
{
  mat4 ModelMatrix;
  mat4 InverseModelMatrix;
};

layout(std140) uniform ViewMatrices
{
  mat4 ViewProjectionMatrix;
};

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;
layout(location = 4) in vec2 SecondTexcoord;
layout(location = 5) in vec3 Tangent;
layout(location = 6) in vec3 Bitangent;

layout(location = 7) in int index0;
layout(location = 8) in float weight0;
layout(location = 9) in int index1;
layout(location = 10) in float weight1;
layout(location = 11) in int index2;
layout(location = 12) in float weight2;
layout(location = 13) in int index3;
layout(location = 14) in float weight3;

out vec3 nor;
out vec3 tangent;
out vec3 bitangent;
out vec2 uv;
out vec2 uv_bis;
out vec4 color;

void main()
{
  color = Color.zyxw;
  mat4 ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
  //  mat4 TransposeInverseModelView = transpose(InverseModelMatrix * InverseViewMatrix);
  gl_Position = ModelViewProjectionMatrix * vec4(Position.xyz, 1.);
  //  nor = (TransposeInverseModelView * vec4(Normal, 0.)).xyz;
  //  tangent = (TransposeInverseModelView * vec4(Tangent, 0.)).xyz;
  //  bitangent = (TransposeInverseModelView * vec4(Bitangent, 0.)).xyz;
  uv = vec4(Texcoord, 1., 1.).xy;
  //  uv_bis = SecondTexcoord;
}
);

const char *fragshader =
"#version 330\n"
"uniform sampler2D tex;\n"
"in vec2 uv;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"  FragColor = texture(tex, uv);\n"
"}\n";


class ObjectShader : public ShaderHelperSingleton<ObjectShader>, public TextureRead<UniformBufferResource<0>, UniformBufferResource<1>, TextureResource<GL_TEXTURE_2D, 0> >
{
public:
  ObjectShader()
  {
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vtxshader,
      GL_FRAGMENT_SHADER, fragshader);

    AssignSamplerNames(Program, "ObjectData", "ViewMatrices", "tex");
  }
};

#endif

#ifdef GLBUILD
static GLuint generateRTT(GLsizei width, GLsizei height, GLint internalFormat, GLint format, GLint type, unsigned mipmaplevel = 1)
{
  GLuint result;
  glGenTextures(1, &result);
  glBindTexture(GL_TEXTURE_2D, result);
  /*    if (CVS->isARBTextureStorageUsable())
  glTexStorage2D(GL_TEXTURE_2D, mipmaplevel, internalFormat, Width, Height);
  else*/
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, 0);
  return result;
}

GLuint cbuf;
#endif

struct ViewBuffer
{
  float ViewProj[16];
};

#ifdef DXBUILD
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbufferDescriptorHeap;
ConstantBuffer<ViewBuffer> *cbuffer;

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdalloc;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdlist;
Microsoft::WRL::ComPtr<ID3D12Resource> DepthBuffer;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DepthDescriptorHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Sampler;

typedef RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
  DescriptorTable<ConstantsBufferResource<1>>,
  DescriptorTable<ConstantsBufferResource<0>>,
  DescriptorTable<ShaderResource<0>>,
  DescriptorTable<SamplerResource<0>> > RS;


class Object : public PipelineStateObject<Object, VertexLayout<irr::video::S3DVertex2TCoords>>
{
public:
  Object() : PipelineStateObject<Object, VertexLayout<irr::video::S3DVertex2TCoords>>(L"Debug\\object_pass_vtx.cso", L"Debug\\object_pass_pix.cso")
  {}

  static void SetRasterizerAndBlendStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psodesc)
  {
    psodesc.pRootSignature = RS::getInstance()->pRootSignature.Get();
    psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psodesc.NumRenderTargets = 1;
    psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psodesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  }
};
#endif

irr::scene::IMeshSceneNode *xue;

void init()
{
  std::vector<std::string> xueB3Dname = { "..\\examples\\xue.b3d" };

  MeshManager::getInstance()->LoadMesh(xueB3Dname);

  xue = new irr::scene::IMeshSceneNode(nullptr, irr::core::vector3df(0.f, 0.f, 2.f));
  xue->setMesh(MeshManager::getInstance()->getMesh(xueB3Dname[0]));

#ifdef GLBUILD
  DebugUtil::enableDebugOutput();
  glGenBuffers(1, &cbuf);
  TrilinearSampler = SamplerHelper::createTrilinearSampler();
  glDepthFunc(GL_LEQUAL);
#endif

#ifdef DXBUILD
  {
    D3D12_DESCRIPTOR_HEAP_DESC cbuffheapdesc = {};
    cbuffheapdesc.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
    cbuffheapdesc.NumDescriptors = 1;
    cbuffheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
    HRESULT hr = Context::getInstance()->dev->CreateDescriptorHeap(&cbuffheapdesc, IID_PPV_ARGS(&cbufferDescriptorHeap));

    cbuffer = new ConstantBuffer<ViewBuffer>();
    Context::getInstance()->dev->CreateConstantBufferView(&cbuffer->getDesc(), cbufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
    hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist));

    hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, 1024, 1024, 1, 0, 1, 0, D3D12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL),
      D3D12_RESOURCE_USAGE_DEPTH,
      &CD3D12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1., 0),
      IID_PPV_ARGS(&DepthBuffer));

    D3D12_DESCRIPTOR_HEAP_DESC Depthdesc = {};
    Depthdesc.Type = D3D12_DSV_DESCRIPTOR_HEAP;
    Depthdesc.NumDescriptors = 1;
    Depthdesc.Flags = D3D12_DESCRIPTOR_HEAP_NONE;
    hr = Context::getInstance()->dev->CreateDescriptorHeap(&Depthdesc, IID_PPV_ARGS(&DepthDescriptorHeap));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    Context::getInstance()->dev->CreateDepthStencilView(DepthBuffer.Get(), &dsv, DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_DESCRIPTOR_HEAP_DESC sampler_heap = {};
    sampler_heap.Type = D3D12_SAMPLER_DESCRIPTOR_HEAP;
    sampler_heap.NumDescriptors = 1;
    sampler_heap.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
    hr = Context::getInstance()->dev->CreateDescriptorHeap(&sampler_heap, IID_PPV_ARGS(&Sampler));

    D3D12_SAMPLER_DESC samplerdesc = {};
    samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerdesc.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.MaxAnisotropy = 1;
    samplerdesc.MinLOD = 0;
    samplerdesc.MaxLOD = 10;
    Context::getInstance()->dev->CreateSampler(&samplerdesc, Sampler->GetCPUDescriptorHandleForHeapStart());
  }
#endif
}

void clean()
{
  TextureManager::getInstance()->kill();
  MeshManager::getInstance()->kill();
  delete xue;
#ifdef GLBUILD
  glDeleteSamplers(1, &TrilinearSampler);
  glDeleteBuffers(1, &cbuf);
#endif
#ifdef DXBUILD
  Object::getInstance()->kill();
  RS::getInstance()->kill();
  delete cbuffer;
  Context::getInstance()->kill();
#endif
}

static float timer = 0.;

void draw()
{
  ViewBuffer cbufdata;
  irr::core::matrix4 View;
  View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
  memcpy(&cbufdata, View.pointer(), 16 * sizeof(float));

  xue->setRotation(irr::core::vector3df(0.f, timer / 360.f, 0.f));
  xue->updateAbsolutePosition();
  xue->render();

  timer += 160.f;

#ifdef GLBUILD
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, 1024, 1024);

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClearDepth(1.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindBuffer(GL_UNIFORM_BUFFER, cbuf);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(ViewBuffer), &cbufdata, GL_STATIC_DRAW);

  glUseProgram(ObjectShader::getInstance()->Program);
  glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getVAO());
  for (const irr::video::DrawData &drawdata : xue->getDrawDatas())
  {
    ObjectShader::getInstance()->SetTextureUnits(xue->getConstantBuffer(), cbuf, drawdata.textures[0]->Id, TrilinearSampler);
    glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)drawdata.IndexCount, GL_UNSIGNED_SHORT, (void *)drawdata.vaoOffset, (GLsizei)drawdata.vaoBaseVertex);
  }
#endif

#ifdef DXBUILD

  D3D12_RECT rect = {};
  rect.left = 0;
  rect.top = 0;
  rect.bottom = 1024;
  rect.right = 1024;

  D3D12_VIEWPORT view = {};
  view.Height = 1024;
  view.Width = 1024;
  view.TopLeftX = 0;
  view.TopLeftY = 0;
  view.MinDepth = 0;
  view.MaxDepth = 1.;

  memcpy(cbuffer->map(), &cbufdata, sizeof(ViewBuffer));
  cbuffer->unmap();

  cmdlist->RSSetViewports(1, &view);
  cmdlist->RSSetScissorRects(1, &rect);

  ID3D12DescriptorHeap *descriptorlst[] =
  {
    cbufferDescriptorHeap.Get()
  };
  cmdlist->SetDescriptorHeaps(descriptorlst, 1);

  cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_RENDER_TARGET));

  float clearColor[] = { .25f, .25f, 0.35f, 1.0f };
  cmdlist->ClearRenderTargetView(Context::getInstance()->getCurrentBackBufferDescriptor(), clearColor, 0, 0);
  cmdlist->ClearDepthStencilView(DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_DEPTH, 1., 0, nullptr, 0);

  cmdlist->SetPipelineState(Object::getInstance()->pso.Get());
  cmdlist->SetGraphicsRootSignature(RS::getInstance()->pRootSignature.Get());
  float c[] = { 1., 1., 1., 1. };
  cmdlist->SetGraphicsRootDescriptorTable(0, cbufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
  cmdlist->SetRenderTargets(&Context::getInstance()->getCurrentBackBufferDescriptor(), true, 1, &DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
  cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdlist->SetIndexBuffer(&xue->getVAO()->getIndexBufferView());
  cmdlist->SetVertexBuffers(0, xue->getVAO()->getVertexBufferView().data(), 1);

  for (irr::video::DrawData drawdata : xue->getDrawDatas())
  {

    cmdlist->SetGraphicsRootDescriptorTable(1, drawdata.descriptors->GetGPUDescriptorHandleForHeapStart());
    cmdlist->SetGraphicsRootDescriptorTable(2, drawdata.descriptors->GetGPUDescriptorHandleForHeapStart().MakeOffsetted(Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));
    cmdlist->SetGraphicsRootDescriptorTable(3, Sampler->GetGPUDescriptorHandleForHeapStart());
    cmdlist->DrawIndexedInstanced((UINT)drawdata.IndexCount, 1, (UINT)drawdata.vaoOffset, (UINT)drawdata.vaoBaseVertex, 0);
  }

  cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_RENDER_TARGET, D3D12_RESOURCE_USAGE_PRESENT));
  HRESULT hr = cmdlist->Close();
  Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
  cmdalloc->Reset();
  cmdlist->Reset(cmdalloc.Get(), nullptr);
  Context::getInstance()->Swap();
#endif
}

#ifdef GLBUILD
int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(1024, 1024, "GLtest", NULL, NULL);
  glfwMakeContextCurrent(window);

  glewExperimental = GL_TRUE;
  glewInit();
  init();

  while (!glfwWindowShouldClose(window))
  {
    draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  clean();
  glfwTerminate();
  return 0;
}
#endif

#ifdef DXBUILD
int WINAPI WinMain(HINSTANCE hInstance,
HINSTANCE hPrevInstance,
LPSTR lpCmdLine,
int nCmdShow)
{
  Context::getInstance()->InitD3D(WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow));
  init();
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

    draw();
  }

  clean();
  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}
#endif
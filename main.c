#include <Windows.h>
#include <Windowsx.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

LPWSTR
AppEncodeTitle(const char* String)
{
  usize Length = MultiByteToWideChar(CP_UTF8, 0, String, -1, 0, 0);
  u16* Out = malloc(sizeof(*Out) * (Length + 1));
  
  if (Out)
  {
    MultiByteToWideChar(CP_UTF8, 0, String, -1, Out, Length + 1);
    Out[Length] = 0;
  };
  return Out;
};

LPWSTR
AppGetShaderPath(LPWSTR BaseName)
{
  wchar_t ExePath[32768];
  usize Length = GetModuleFileNameW(NULL, ExePath, ARRAYSIZE(ExePath));
  
  usize Directory = 0;
  
  for (usize i = Length; i; i--)
  {
    if (ExePath[i - 1] == '\\')
    {
      Directory = i;
      break;
    };
  };
  
  usize BaseLength = wcslen(BaseName);
  usize Count = Directory + BaseLength;
  u16* Out = malloc(sizeof(*Out) * (Count + 1));
  
  if (Out)
  {
    memcpy(Out, ExePath, Directory * 2);
    memcpy(Out + Directory, BaseName, BaseLength * 2);
    Out[Count] = 0;
  };
  return Out;
};

#define Free(x) if ((x)) free((void*)(x));

void
AppError_(const char* Error, const char* Path, int Line)
{
  char ErrorBuffer[0x1000];
  snprintf(
    ErrorBuffer,
    ARRAYSIZE(ErrorBuffer),
    "<%s:%d>\n"
    "       %s\n",
    Path, Line, Error
  );
  ErrorBuffer[ARRAYSIZE(ErrorBuffer) - 1] = 0;

  MessageBoxA(NULL, ErrorBuffer, "Error", MB_ICONERROR | MB_OK);

  abort();
};

#define AppError(s) AppError_((s), __FILE__, __LINE__)

#define AppComRelease(p) \
if ((p)) {(p)->lpVtbl->Release((p)); (p) = NULL; };

#define AppCleanupHr(Hr) \
if (FAILED(Hr)) goto Cleanup;


typedef float float2[2];
typedef float float3[3];
typedef float float4[4];

typedef struct vertex vertex;
struct vertex
{
  float2 Position;
  float3 Color;
};

typedef struct window window;
struct window
{
  HWND Hwnd;
  ID3D11Device* Device;
  ID3D11DeviceContext* Context;
  IDXGISwapChain* SwapChain;
  ID3D11RenderTargetView* Target;
  ID3D11InputLayout* InputLayout;
  ID3D11PixelShader* PixelShader;
  ID3D11VertexShader* VertexShader;
  ID3D11Buffer* VertexBuffer;
  int Width, Height;
};

u32
WindowPrepareDevice(window* Window)
{
  u32 Ok = 0;
  ID3D11Device* Device = NULL;
  ID3D11DeviceContext* Context = NULL;
  IDXGISwapChain* SwapChain = NULL;
  ID3D11RenderTargetView* Target = NULL;
  ID3D11InputLayout* InputLayout = NULL;
  ID3D11PixelShader* PixelShader = NULL;
  ID3D11VertexShader* VertexShader = NULL;
  ID3D11Buffer* VertexBuffer = NULL;

  RECT Client; GetClientRect(Window->Hwnd, &Client);
  int Width = Client.right - Client.left;
  int Height = Client.bottom - Client.top;
  D3D_FEATURE_LEVEL FeatureLevel = 0;
  UINT Flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED;
  
  DXGI_SWAP_CHAIN_DESC Desc = {0};
  Desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  Desc.BufferDesc.Width = Width;
  Desc.BufferDesc.Height = Height;
  Desc.BufferCount = 2;
  Desc.Windowed = TRUE;
  Desc.OutputWindow = Window->Hwnd;
  Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  Desc.SampleDesc.Count = 1;
  Desc.SampleDesc.Quality = 0;

  LPWSTR ShaderPath = AppGetShaderPath(L"Shaders\\Shader.hlsl");
  ID3DBlob* PsBlob = NULL;
  ID3DBlob* VsBlob = NULL;
  ID3DBlob* ErrorBlob = NULL;
  ID3D11Texture2D* BackBuffer = NULL;
  
  AppCleanupHr(
    D3D11CreateDeviceAndSwapChain(
      NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, Flags,
      NULL, 0, D3D11_SDK_VERSION, &Desc, &SwapChain,
      &Device, &FeatureLevel, &Context
    )
  );
  
  AppCleanupHr(
    SwapChain->lpVtbl->GetBuffer(
      SwapChain, 0, &IID_ID3D11Texture2D, (void**)&BackBuffer
    )
  );
  
  AppCleanupHr(
    Device->lpVtbl->CreateRenderTargetView(
      Device, (ID3D11Resource*)BackBuffer, NULL, &Target
    )
  );

  AppCleanupHr(
    D3DCompileFromFile(
      ShaderPath, NULL, NULL, "VertexMain", "vs_5_0", 
      0, 0, &VsBlob, &ErrorBlob
    )
  );

  AppCleanupHr(
    Device->lpVtbl->CreateVertexShader(
      Device, VsBlob->lpVtbl->GetBufferPointer(VsBlob),
      VsBlob->lpVtbl->GetBufferSize(VsBlob), NULL, &VertexShader
    )
  );

  AppCleanupHr(
    D3DCompileFromFile(
      ShaderPath, NULL, NULL, "PixelMain", "ps_5_0", 
      0, 0, &PsBlob, &ErrorBlob
    )
  );

  AppCleanupHr(
    Device->lpVtbl->CreatePixelShader(
      Device, PsBlob->lpVtbl->GetBufferPointer(PsBlob),
      PsBlob->lpVtbl->GetBufferSize(PsBlob), NULL, &PixelShader
    )
  );

  D3D11_INPUT_ELEMENT_DESC ElementDesc[] = 
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,  0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  AppCleanupHr(
    Device->lpVtbl->CreateInputLayout(
      Device, ElementDesc, ARRAYSIZE(ElementDesc), 
      VsBlob->lpVtbl->GetBufferPointer(VsBlob), 
      VsBlob->lpVtbl->GetBufferSize(VsBlob), &InputLayout
    )
  );

  vertex Vertices[] = 
  {
    {{0.0, 0.5}, {1.0, 0.0, 0.0}},
    {{0.5, -0.5}, {0.0, 1.0, 0.0}},
    {{-0.5, -0.5}, {0.0, 0.0, 1.0}},
  };

  D3D11_BUFFER_DESC BufferDesc = {0};
  BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  BufferDesc.ByteWidth = sizeof(Vertices);
  BufferDesc.StructureByteStride = sizeof(vertex);
  BufferDesc.Usage = D3D11_USAGE_DEFAULT;

  D3D11_SUBRESOURCE_DATA BufferData = {0};
  BufferData.pSysMem = Vertices;

  AppCleanupHr(
    Device->lpVtbl->CreateBuffer(
      Device, &BufferDesc, &BufferData, &VertexBuffer
    )
  );

  Context->lpVtbl->OMSetRenderTargets(
    Context, 1, &Target, NULL
  );

  D3D11_VIEWPORT ViewPort = {0};
  ViewPort.Width = Width;
  ViewPort.Height = Height;
  ViewPort.MaxDepth = 1;

  Context->lpVtbl->RSSetViewports(
    Context, 1, &ViewPort
  );

  Window->Width = Width;
  Window->Height = Height;
  Ok = 1;
  
Cleanup:
  AppComRelease(PsBlob);
  AppComRelease(VsBlob);
  AppComRelease(ErrorBlob);
  AppComRelease(BackBuffer);
  Free(ShaderPath);
  
  
  if (Ok)
  {
    Window->Device = Device;
    Window->Context = Context;
    Window->SwapChain = SwapChain;
    Window->Target = Target;
    Window->InputLayout = InputLayout;
    Window->PixelShader = PixelShader;
    Window->VertexShader = VertexShader;
    Window->VertexBuffer = VertexBuffer;
  } else
  {
    AppComRelease(Device);
    AppComRelease(Context);
    AppComRelease(SwapChain);
    AppComRelease(Target);
    AppComRelease(InputLayout);
    AppComRelease(PixelShader);
    AppComRelease(VertexShader);
    AppComRelease(VertexBuffer);
  };
  return Ok;
};

window*
WindowMake(HWND Hwnd)
{
  window* Out = calloc(1, sizeof(*Out));

  if (Out)
  {
    Out->Hwnd = Hwnd;

    if (!WindowPrepareDevice(Out))
    {
      AppError("Could not initialize Direct X");
    };
  };
  return Out;
};

void
WindowTake(window* Window)
{
  if (!Window) return;

  AppComRelease(Window->Device);
  AppComRelease(Window->Context);
  AppComRelease(Window->SwapChain);
  AppComRelease(Window->Target);
  AppComRelease(Window->InputLayout);
  AppComRelease(Window->PixelShader);
  AppComRelease(Window->VertexShader);
  AppComRelease(Window->VertexBuffer);

  Free(Window);
};


void
WindowResize(window* Window, int Width, int Height)
{
  u32 ShouldResize = (
    Window && Width >= 0 && Height >= 0 &&
    (Window->Width != Width || Window->Height != Height)
  );

  if (!ShouldResize) return;

  AppComRelease(Window->Target);

  ID3D11Texture2D* BackBuffer = NULL;

  Window->SwapChain->lpVtbl->ResizeBuffers(
    Window->SwapChain, 0, Width, Height,
    DXGI_FORMAT_UNKNOWN, 0
  );

  HRESULT Result = Window->SwapChain->lpVtbl->GetBuffer(
    Window->SwapChain, 0, &IID_ID3D11Texture2D,
    (void**)&BackBuffer
  );

  Window->Device->lpVtbl->CreateRenderTargetView(
    Window->Device, (ID3D11Resource*)BackBuffer,
    NULL, &Window->Target
  );

  AppComRelease(BackBuffer);

  Window->Context->lpVtbl->OMSetRenderTargets(
    Window->Context, 1, &Window->Target, NULL
  );

  D3D11_VIEWPORT ViewPort = {0};
  ViewPort.Width = Width;
  ViewPort.Height = Height;
  ViewPort.MaxDepth = 1;

  Window->Context->lpVtbl->RSSetViewports(
    Window->Context, 1, &ViewPort
  );

  Window->Width = Width;
  Window->Height = Height;
};

void
WindowPaint(window* Window)
{
  if (!Window) return;

  FLOAT Clear[4] = {0.2f, 0.15f, 0.11f, 1.0};

  D3D11_VIEWPORT ViewPort = {0};
  ViewPort.Width = Window->Width;
  ViewPort.Height = Window->Height;
  ViewPort.MaxDepth = 1;

  Window->Context->lpVtbl->RSSetViewports(
    Window->Context, 1, &ViewPort
  );


  Window->Context->lpVtbl->ClearRenderTargetView(
    Window->Context, Window->Target,
    Clear
  );

  Window->Context->lpVtbl->OMSetRenderTargets(
    Window->Context, 1, &Window->Target,
    NULL
  );

  Window->Context->lpVtbl->IASetPrimitiveTopology(
    Window->Context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
  );

  Window->Context->lpVtbl->IASetInputLayout(
    Window->Context, Window->InputLayout
  );

  UINT Stride = sizeof(vertex);
  UINT Offset = 0;

  
  Window->Context->lpVtbl->IASetVertexBuffers(
    Window->Context, 0, 1, &Window->VertexBuffer,
    &Stride, &Offset
  );

  Window->Context->lpVtbl->PSSetShader(
    Window->Context, Window->PixelShader, NULL,
    0
  );

  Window->Context->lpVtbl->VSSetShader(
    Window->Context, Window->VertexShader, NULL,
    0
  );

  Window->Context->lpVtbl->Draw(Window->Context, 3, 0);

  Window->SwapChain->lpVtbl->Present(Window->SwapChain, 1, 0);
};


LRESULT CALLBACK
WindowProc(HWND Hwnd, UINT Msg, WPARAM Wparam, LPARAM Lparam)
{
  window* Window = (window*)GetWindowLongPtrW(Hwnd, GWLP_USERDATA);

  switch (Msg)
  {
    case WM_CREATE:
    {
      Window = WindowMake(Hwnd);
      SetWindowLongPtrW(Hwnd, GWLP_USERDATA, (LONG_PTR)Window);
      BOOL Dark = TRUE;
      DwmSetWindowAttribute(Hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &Dark, sizeof(Dark));
    } return 0;
    case WM_DESTROY:
    {
      PostQuitMessage(0);
      WindowTake(Window);
    } return 0;
    case WM_SIZE:
    {
      WindowResize(Window, LOWORD(Lparam), HIWORD(Lparam));
    } return 0;
    case WM_PAINT:
    {
      WindowPaint(Window);
      ValidateRect(Hwnd, 0);
    } return 0;
  };
  return DefWindowProcW(Hwnd, Msg, Wparam, Lparam);
};

void
AppDisableDpiScaling(void)
{
  typedef BOOL WINAPI 
  set_process_dpi_awareness_context(_In_ DPI_AWARENESS_CONTEXT value);

  typedef BOOL WINAPI
  set_process_dpi_aware(VOID);

  HMODULE User32 = LoadLibraryA("User32.dll");

  set_process_dpi_awareness_context* SetProcessDpiAwarenessContext = (void*)GetProcAddress(User32, "SetProcessDpiAwarenessContext");
  set_process_dpi_aware* SetProcessDPIAware = (void*)GetProcAddress(User32, "SetProcessDPIAware");

  if (!SetProcessDpiAwarenessContext || !SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) && SetProcessDPIAware)
  {
    SetProcessDPIAware();
  };
};

window*
WindowOpen(const char* Title, int Width, int Height)
{
  LPWSTR ClassName = L"Triangle.Window";
  HINSTANCE Module = GetModuleHandleA(0);
  WNDCLASSEXW Wc = {0};
  Wc.cbSize = sizeof(Wc);
  Wc.lpfnWndProc = WindowProc;
  Wc.lpszClassName = ClassName;
  Wc.hInstance = Module;
  Wc.style = CS_HREDRAW;
  Wc.hCursor = LoadCursorA(0, IDC_ARROW);
  Wc.hIcon = LoadIconA(Module, MAKEINTRESOURCEA(101));
  RegisterClassExW(&Wc);

  LPWSTR Name = AppEncodeTitle(Title);

  RECT Client = {0, 0, Width, Height};
  AdjustWindowRect(&Client, WS_OVERLAPPEDWINDOW, TRUE);
  Width = Client.right - Client.left;
  Height = Client.bottom - Client.top;

  HWND Hwnd = CreateWindowExW(
    WS_EX_APPWINDOW, ClassName, Name, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, Width, Height,
    NULL, NULL, Module, NULL
  );

  Free(Name);

  window* Window = (window*)GetWindowLongPtrW(Hwnd, GWLP_USERDATA);

  return Window;
};

void
WindowShow(window* Window)
{
  if (Window) ShowWindow(Window->Hwnd, SW_SHOWDEFAULT);
};

int wWinMain(HINSTANCE x, HINSTANCE y, LPWSTR z, int w)
{
  window* Window = WindowOpen("Triangle in D3D11!", 1200, 600);

  WindowShow(Window);

  MSG Msg = {0};

  while (GetMessageW(&Msg, 0, 0, 0))
  {
    TranslateMessage(&Msg);
    DispatchMessageW(&Msg);
  };

  return 0;
};

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

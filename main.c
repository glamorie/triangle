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

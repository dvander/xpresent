// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include "console.h"
#include "winutils.h"

template <typename T>
void CacheFunction(T& t, const char* name)
{
  if (t)
    return;

  HMODULE module = ::LoadLibraryA("kernel32.dll");
  if (!module)
    AbortWithGLE("LoadLibrary kernel32.dll");
  t = reinterpret_cast<T>(::GetProcAddress(module, name));
  FreeLibrary(module);
}

bool InitializeProcThreadAttributeListFn(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
                                         DWORD dwAttributeCount,
                                         DWORD dwFlags,
                                         PSIZE_T lpSize)
{
  static decltype(InitializeProcThreadAttributeList)* fn;
  CacheFunction(fn, "InitializeProcThreadAttributeList");
  return !!fn(lpAttributeList, dwAttributeCount, dwFlags, lpSize);
}

bool UpdateProcThreadAttributeFn(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
                                 DWORD                        dwFlags,
                                 DWORD_PTR                    Attribute,
                                 PVOID                        lpValue,
                                 SIZE_T                       cbSize,
                                 PVOID                        lpPreviousValue,
                                 PSIZE_T                      lpReturnSize)
{
  static decltype(UpdateProcThreadAttribute)* fn;
  CacheFunction(fn, "UpdateProcThreadAttribute");
  return !!fn(lpAttributeList, dwFlags, Attribute, lpValue, cbSize, lpPreviousValue, lpReturnSize);
}

void DeleteProcThreadAttributeListFn(_Inout_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList)
{
  static decltype(DeleteProcThreadAttributeList)* fn;
  CacheFunction(fn, "DeleteProcThreadAttributeList");
  fn(lpAttributeList);
}

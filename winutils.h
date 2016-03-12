// vim: set ts=2 sts=2 sw=2 tw=99 et:
#ifndef _xpresent_winutils_h_
#define _xpresent_winutils_h_

#include <Windows.h>

bool InitializeProcThreadAttributeListFn(
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  DWORD dwAttributeCount,
  DWORD dwFlags,
  PSIZE_T lpSize);

bool UpdateProcThreadAttributeFn(
  _Inout_   LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  _In_      DWORD                        dwFlags,
  _In_      DWORD_PTR                    Attribute,
  _In_      PVOID                        lpValue,
  _In_      SIZE_T                       cbSize,
  _Out_opt_ PVOID                        lpPreviousValue,
  _In_opt_  PSIZE_T                      lpReturnSize);

void DeleteProcThreadAttributeListFn(
  _Inout_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList);

#endif // _xpresent_winutils_h_
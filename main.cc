// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include "console.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>
#include "parent.h"
#include "child.h"

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
  if (FAILED(CoInitialize(nullptr)))
    AbortWithGLE("CoInitialize");

  srand((int)time(NULL));

  if (__argc >= 2 && strcmp(__argv[1], "child") == 0) {
    DWORD key = atoi(__argv[2]);
    ChildMain(hInstance, key, __argc - 3, &__argv[3]);
  } else {
    ParentMain(hInstance, nCmdShow, __argc, __argv);
  }

  CoUninitialize();
  return 0;
}
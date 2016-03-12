// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include "console.h"
#include <stdio.h>
#include "pipe.h"

Pipe::Pipe()
 : pipe_(nullptr)
{
}

Pipe::~Pipe()
{
  CloseHandle(pipe_);
}

void
Pipe::Send(char c, uintptr_t message)
{
  msg_t msg = {
    c, message
  };

  DWORD written;
  BOOL rv = ::WriteFile(
    pipe_,
    &msg,
    sizeof(msg),
    &written,
    nullptr);
  if (!rv)
    AbortWithGLE("WriteFile pipe");
}

bool
Pipe::Read(char& c, uintptr_t& message)
{
  msg_t msg;

  DWORD read;
  BOOL rv = ::ReadFile(
    pipe_,
    &msg,
    sizeof(msg),
    &read,
    nullptr);
  if (!rv || read != sizeof(msg))
    return false;

  c = msg.prefix;
  message = msg.data;
  return true;
}

void
Pipe::FormatName(char* buffer, size_t length, DWORD key)
{
  _snprintf(buffer, length, "\\\\.\\pipe\\xpresent.%d", key);
}

ParentPipe::ParentPipe()
{
  FormatName(name_, sizeof(name_), Key());

  pipe_ = ::CreateNamedPipeA(
    name_,
    PIPE_ACCESS_DUPLEX |
      FILE_FLAG_FIRST_PIPE_INSTANCE,
    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
    1,
    256, 256,
    0,
    nullptr);
  if (pipe_ == INVALID_HANDLE_VALUE)
    AbortWithGLE("CreateNamedPipeA");
}

bool
ParentPipe::Wait()
{
  if (!::ConnectNamedPipe(pipe_, nullptr) &&
      GetLastError() != ERROR_PIPE_CONNECTED)
  {
    return false;
  }
  return true;
}

DWORD
ParentPipe::Key()
{
  return GetCurrentProcessId();
}

ChildPipe::ChildPipe(DWORD key)
{
  FormatName(name_, sizeof(name_), key);

  pipe_ = ::CreateFileA(
    name_,
    GENERIC_READ|GENERIC_WRITE,
    0,
    nullptr,
    OPEN_EXISTING,
    0,
    nullptr);
  if (pipe_ == INVALID_HANDLE_VALUE)
    AbortWithGLE("CreateFile");
}
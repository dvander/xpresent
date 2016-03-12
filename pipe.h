// vim: set ts=2 sts=2 sw=2 tw=99 et:
#ifndef _xpresent_pipe_h_
#define _xpresent_pipe_h_

#include <Windows.h>

static const char kPipeHwndMsg = 'w';
static const DWORD kWM_PRESENT = WM_USER + 1;
static const DWORD kWM_CHANGE_DEVICE = WM_USER + 2;

class Pipe
{
 public:
  Pipe();
  virtual ~Pipe();

  void Send(char c, uintptr_t message);
  bool Read(char& c, uintptr_t& message);

 protected:
  static void FormatName(char* buffer, size_t length, DWORD key);

 private:
  struct msg_t {
    char prefix;
    uintptr_t data;
  };

 protected:
  HANDLE pipe_;
  char name_[256];
};

class ParentPipe : public Pipe
{
 public:
  ParentPipe();

  bool Wait();

  DWORD Key();
};

class ChildPipe : public Pipe
{
 public:
  ChildPipe(DWORD key);
};

#endif // _xpresent_pipe_h_
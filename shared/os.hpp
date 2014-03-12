// wraps misc. POSIX vs. Windows facilities
#ifndef GRAEHL_SHARED__OS_HPP
#define GRAEHL_SHARED__OS_HPP

#include <fstream>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <stdlib.h>
#include <cstring>
#include <sstream>
#include <graehl/shared/atoi_fast.hpp>
// see also shell.hpp for things relying on unix or cygwin shell

#if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32)) && !defined(__CYGWIN__)
# define OS_WINDOWS
# include <io.h>
# include <windows.h>
#else
# include <sys/types.h>
# include <fcntl.h>
# include <signal.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

#if !defined( MEMMAP_IO_WINDOWS ) && !defined( MEMMAP_IO_POSIX )
# if defined(OS_WINDOWS) || defined(__CYGWIN__)
# define MEMMAP_IO_WINDOWS
# ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0500
# endif
# else
# define MEMMAP_IO_POSIX
# endif
#endif

#ifdef MEMMAP_IO_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef VC_EXTRALEAN
# define VC_EXTRALEAN 1
#endif
#ifndef NOMINMAX
# define NOMINMAX 1
#endif
#include <windows.h>

#undef min
#undef max
#undef DELETE

#endif

#ifdef OS_WINDOWS
# include <direct.h>
# ifdef max
#  undef max
# endif
# ifdef min
#  undef min
# endif
#endif

//static local var means env var is checked once (like singleton)
#define DECLARE_ENV(fn,var) static int fn() {static const int l=graehl::getenv_int(#var);return l;}
#define DECLARE_ENV_C(n,f,v) DECLARE_ENV(f,v) static const int n = f();
#define DECLARE_ENV_C_LEVEL(n,f,v) DECLARE_ENV(f,v) DECLARE_ENV(f##_LEVEL,v##_LEVEL) static const int n##_1 = f(); static const int n##_2 = f##_LEVEL(); static const int n=n##_1>n##_2?n##_1:n##_2; //


#ifdef OS_WINDOWS
const DWORD getenv_maxch = 65535; // //Limit according to http://msdn.microsoft.com/en-us/library/ms683188.aspx
static char getenv_buf[getenv_maxch];
inline char * getenv(char const* key) {
  return GetEnvironmentVariableA(key,getenv_buf,getenv_maxch) ? getenv_buf : NULL;
}

namespace graehl {
typedef DWORD Error;
inline long get_process_id() {
  return GetCurrentProcessId();
}
}//ns

#else

# include <unistd.h>
namespace graehl {
typedef int Error;
inline long get_process_id() {
  return getpid();
}
}//ns
# include <errno.h>
# include <string.h>
#endif

namespace graehl {

inline int getenv_int(char const* key) {
  char const* s = getenv(key);
  return s ? atoi_nows(s) : 0;
}

inline char * getenv_str(char const* key) {
  return getenv(key);
}

inline int system_safe(const std::string &cmd)
{
  int ret=::system(cmd.c_str());
  if (ret!=0)
    throw std::runtime_error(cmd);
  return ret;
}


inline std::string get_current_dir() {
#ifdef OS_WINDOWS
  char *malloced=_getcwd(NULL,0);
#else
  char *malloced=::getcwd(NULL,0);
#endif
  if (!malloced)
    throw std::runtime_error("Couldn't get current working directory");
  std::string ret(malloced);
  free(malloced);
  return ret;
}

template <class O>
void print_current_dir(O&o,const char*header="### CURRENT DIR: ")
{
  if (header)
    o << header;
  o << get_current_dir();
  if (header)
    o << std::endl;
}

inline Error last_error() {
#ifdef MEMMAP_IO_WINDOWS
  return ::GetLastError();
#else
  return errno;
#endif
}

inline std::string error_string(Error err) {
#ifdef MEMMAP_IO_WINDOWS
  LPVOID lpMsgBuf;
  if (::FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_SYSTEM,
          NULL,
          err,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR) &lpMsgBuf,
          0, NULL ) == 0)
    throw std::runtime_error("couldn't generate Windows error message string");
  std::string ret((char*) lpMsgBuf);
  ::LocalFree(lpMsgBuf);
  return ret;
#else
  return strerror(err);
#endif
}

inline std::string last_error_string() {
  return error_string(last_error());
}


inline bool create_file(const std::string& path,std::size_t size) {
#ifdef _WIN32
  HANDLE fh=::CreateFileA( path.c_str(),GENERIC_WRITE,FILE_SHARE_DELETE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
  if (fh == INVALID_HANDLE_VALUE)
    return false;
  if(::SetFilePointer(fh,(LONG)size,NULL,FILE_BEGIN) != size)
    return false;
  if (!::SetEndOfFile(fh))
    return false;
  return ::CloseHandle(fh);
#else
  return ::truncate(path.c_str(),size) != -1;
#endif
}

inline bool remove_file(const std::string &filename) {
  return 0==remove(filename.c_str());
}

struct tmp_fstream
{
  std::string filename;
  std::fstream file;
  bool exists;
  explicit tmp_fstream(const char *c)
  {
    choose_name();
    open();
    file << c;
    // DBP(c);
    reopen();
  }
  tmp_fstream(std::ios::openmode mode=std::ios::in | std::ios::out | std::ios::trunc )
  {
    choose_name();
    open(mode);
  }
  void choose_name()
  {

#include <graehl/shared/warning_push.h>
#if HAVE_GCC_4_6
    GCC_DIAG_IGNORE(deprecated-declarations)
#endif
    CLANG_DIAG_IGNORE(deprecated-declarations)

    filename = std::tmpnam(NULL);

#include <graehl/shared/warning_pop.h>
  }
  void open(std::ios::openmode mode=std::ios::in | std::ios::out | std::ios::trunc) {
    file.open(filename.c_str(),mode);
    if (!file)
      throw std::ios::failure(std::string("couldn't open temporary file ").append(filename));
  }
  void reopen()
  {
    file.flush();
    file.seekg(0);
  }
  void close()
  {
    file.close();
  }
  void remove()
  {
    remove_file(filename);
  }

  ~tmp_fstream()
  {
    close();
    remove();
  }
};


inline void throw_last_error(const std::string &module="ERROR")
{
  throw std::runtime_error(module+": "+last_error_string());
}

#define TMPNAM_SUFFIX "XXXXXX"
#define TMPNAM_SUFFIX_LEN 6

inline bool is_tmpnam_template(const std::string &filename_template)
{
  unsigned len=(unsigned)filename_template.length();
  return !(len<TMPNAM_SUFFIX_LEN || filename_template.substr(len-TMPNAM_SUFFIX_LEN,TMPNAM_SUFFIX_LEN)!=TMPNAM_SUFFIX);
}


//!< file is removed if keepfile==false (dangerous: another program could grab the filename first!). returns filename created. if template is missing XXXXXX, it's appended first.
inline std::string safe_tmpnam(const std::string &filename_template="/tmp/tmp.safe_tmpnam.XXXXXX",
                               bool keepfile=false,
                               bool worldReadable=false)
{
  const unsigned MY_MAX_PATH=1024;
  char tmp[MY_MAX_PATH+1];
  std::strncpy(tmp, filename_template.c_str(),MY_MAX_PATH-TMPNAM_SUFFIX_LEN);

  if (!is_tmpnam_template(filename_template))
    std::strcpy(tmp+filename_template.length(),TMPNAM_SUFFIX);

#ifdef OS_WINDOWS
  int err = ::_mktemp_s(tmp, ::strlen(tmp) + 1); // this does not create the file, sadly. alternative (with many retries: http://stackoverflow.com/questions/6036227/mkstemp-implementation-for-win32/6036308#6036308 )
  if (err)
    throw_last_error(std::string("safe_tmpnam couldn't mkstemp ").append(tmp));
#else
  int fd = ::mkstemp(tmp);

  if (fd==-1)
    throw_last_error(std::string("safe_tmpnam couldn't mkstemp ").append(tmp));

  if (worldReadable)
    ::fchmod(fd, 0644);

  ::close(fd);

  if (!keepfile)
    ::unlink(tmp);
#endif

  return tmp;
}

inline std::string maybe_tmpnam(const std::string &filename_template="/tmp/safe_tmpnam.XXXXXX", bool keepfile=true)
{
  return is_tmpnam_template(filename_template) ?
      safe_tmpnam(filename_template,keepfile) :
      filename_template;
}


inline bool safe_unlink(const std::string &file,bool must_succeed=true)
{
  if (
#ifdef OS_WINDOWS
      std::remove
#else
      ::unlink
#endif
         (file.c_str()) == -1)
  {
    if (must_succeed)
      throw_last_error(std::string("couldn't remove ").append(file));
    return false;
  } else {
    return true;
  }
}

//!< returns dir/name unless dir is empty (just name, then). if name begins with / then just returns name.
inline std::string joined_dir_file(const std::string &basedir,const std::string &name="",char pathsep='/')
{
  if (!name.empty() && name[0]==pathsep) //absolute name
    return name;
  if (basedir.empty()) // relative base dir
    return name;
  if (basedir[basedir.length()-1]!=pathsep) // base dir doesn't already end in pathsep
    return basedir+pathsep+name;
  return basedir+name;
}

//FIXME: test
inline void split_dir_file(const std::string &fullpath,std::string &dir,std::string &file,char pathsep='/')
{
  using namespace std;
  string::size_type p=fullpath.rfind(pathsep);
  if (p==string::npos) {
    dir=".";
    file=fullpath;
  } else {
    dir=fullpath.substr(0,p);
    file=fullpath.substr(p+1,fullpath.length()-(p+1));
  }
}

#ifdef OS_WINDOWS
//TODO: popen2, popen3
#else

/**
   \param out fd[0,1,2] get stdin, stdout, stderr; cmd is 0 terminated. fd[2] is set only if stderrToFd2

   write to fd[0] to send to child stdin, read from fd[1] and fd[2] stdout and stderr

   \return pid on success (in which case caller must close fd[0] fd[1] and if stderrToFd2 fd[2], -1 on failure

   \param stderrToFd2 - if false, just set fd[0] and fd[1] (stderr is parent's original)

   if you want just one of stdin or stdout, use the regular unix popen
*/
inline pid_t popen3(int *fd, const char **const cmd,
                    bool stderrToFd2 = true)
{
  int p[3][2]; // 3 pipes; 0 is read and 1 is write end
  pid_t pid;

  int npipes = 0;
  for (int maxpipes = stderrToFd2 ? 3 : 2; npipes < maxpipes; ++npipes)
    if (pipe(p[npipes]))
      goto fail;

  if ((pid = fork())) {
    if (-1 == pid)
      goto fail;
    // parent process:
    // write to fd[0] to send to child stdin, read from fd[1] and fd[2] stdout and stderr
    fd[STDIN_FILENO] = p[STDIN_FILENO][1];
    close(p[STDIN_FILENO][0]);
    fd[STDOUT_FILENO] = p[STDOUT_FILENO][0];
    close(p[STDOUT_FILENO][1]);
    if (stderrToFd2) {
      fd[STDERR_FILENO] = p[STDERR_FILENO][0];
      close(p[STDERR_FILENO][1]);
    }
    return pid;
  } else {
    // child:
    dup2(p[STDIN_FILENO][0], STDIN_FILENO);
    close(p[STDIN_FILENO][1]);
    dup2(p[STDOUT_FILENO][1], STDOUT_FILENO);
    close(p[STDOUT_FILENO][0]);
    if (stderrToFd2) {
      dup2(p[STDERR_FILENO][1], STDERR_FILENO);
      close(p[STDERR_FILENO][0]);
    }

    execvp(*cmd, const_cast<char*const*>(cmd)); // doesn't return (normally)

    perror("Couldn't execvp command");
    fprintf(stderr, " \"%s\"\n", *cmd);
    _exit(EXIT_FAILURE);
  }
fail:
  int save_errno = errno;
  for (int i = 0; i < npipes; i++) {
    close(p[i][0]);
    close(p[i][1]);
  }
  errno = save_errno;
  return (pid_t)-1;
}

/**
   \param[out] fd[0] gets stdin of child (write to this), fd[1] gets stdout (read from this).

   \param cmd is 0 terminated execvp
*/
inline pid_t popen2(int *fd, const char **const cmd) {
  return popen3(fd, cmd, false);
}
#endif

} // graehl


#endif

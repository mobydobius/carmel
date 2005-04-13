// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;;
// ignacio

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <string>
#include <iostream>
#include <sstream>

#ifndef __NO_GNU_NAMESPACE__
using namespace __gnu_cxx;
#endif
using namespace std;

#ifndef INFO_LEVEL
# define INFO_LEVEL 9999
#endif
#ifndef ASSERT_LEVEL
# define ASSERT_LEVEL 9999
#endif

#define COND_INFO(level,op) (INFO_LEVEL op level)
#define COND_INFO_RUNTIME(level,op) (INFO_LEVEL op level && dbg->runtime_info_level op level)
#define COND_INFO_RUNTIME_EQUAL(level) (COND_INFO(level,>=) && dbg->runtime_info_level == level)
#define IF_INFO_RUNTIME(level) if(COND_INFO_RUNTIME(level,>=))
#define UNLESS_INFO_RUNTIME(level) if(COND_INFO_RUNTIME(level,<))
#define IF_INFO(level) if(COND_INFO(level,>=))
#define UNLESS_INFO(level) if(COND_INFO(level,<))
#define IF_ASSERT(level) if(ASSERT_LEVEL>=level)
#define UNLESS_ASSERT(level) if(ASSERT_LEVEL<level)

#define assertlvl(level,assertion) IF_ASSERT(level) {assert(assertion);}

// from makestr.hpp (GraehlCVS)
#define MAKESTR_STRINGIZE(str) #str
#define MAKESTR_EXPAND_STRINGIZE(expr) MAKESTR_STRINGIZE(expr)
#define MAKESTR_FILE_LINE "(" __FILE__  ":" MAKESTR_EXPAND_STRINGIZE(__LINE__)  ")"
#define MAKESTR_DATE __DATE__ " " __TIME__
// __FILE__  ":"  __LINE__  "): failed to write " #expr
// usage: MAKESTRS(str,os << 1;os << 2) => str="12"
#define MAKESTRS(string,expr) do {std::ostringstream os;expr;if (!os) throw std::runtime_error(MAKESTR_FILE_LINE ": failed to write" #expr);string=os.str();}while(0)
// usage: MAKESTR(str,1 << 2) => str="12"
#define MAKESTR(string,expr) MAKESTRS(string,os << expr)
#define MAKESTRFL(string,expr) MAKESTR(string,MAKESTR_FILE_LINE << expr)


#ifdef NO_INFO
# define DBG_OP_F_NL(lvl,pDbg,op,module,msg,file,line,newline)
# define DBG_OP_F(lvl,pDbg,op,module,msg,file,line)
#else

# define DBG_OP_F(lvl,pDbg,op,module,msg,file,line) DBG_OP_F_NL(lvl,pDbg,op,module,msg,file,line,true)

# define DBG_OP_F_NL(lvl,pDbg,op,module,msg,file,line,newline) do {               \
        if (INFO_LEVEL >= lvl && (pDbg)->runtime_info_level >= lvl) {   \
   ostringstream os; \
   os << msg; \
   if(!os) throw std::runtime_error(MAKESTR_FILE_LINE ": failed to write " #module " : " #msg); \
   (pDbg)->op(module,os.str(),file,line,newline);      \
} } while(0)

#endif

//!< Q: no FILE/LINE included in output
//!< L: specify verbosity level as first argument
#define DBG_OP(pDbg,op,module,msg) DBG_OP_L(0,pDbg,op,module,msg)
#define DBG_OP_Q(pDbg,op,module,msg) DBG_OP_LQ(0,pDbg,op,module,msg)
#define DBG_OP_L(lvl,pDbg,op,module,msg) DBG_OP_F(lvl,pDbg,op,module,msg,__FILE__,__LINE__)
#define DBG_OP_LQ_NEWLINE(lvl,pDbg,op,module,msg,newline) DBG_OP_F_NL(lvl,pDbg,op,module,msg,"",0,newline)
#define DBG_OP_LQ(lvl,pDbg,op,module,msg) DBG_OP_F(lvl,pDbg,op,module,msg,"",0)
#define NESTINFO_GUARD(pDbg) ns_decoder_global::Debug::Nest debug_nest_guard_ ## __LINE__ (pDbg)

// some of these names might be used, e.g. in windows.h, but seems ok on linux.  compilation will fail/warn if they are in conflict.
#define NESTINFO NESTINFO_GUARD(dbg)
#define INFO(module,msg) DBG_OP(dbg,info,module,msg)
#define WARNING(module,msg) DBG_OP(dbg,warning,module,msg)
#define ERROR(module,msg) DBG_OP(dbg,error,module,msg)
#define FATAL(module,msg) DBG_OP(dbg,fatalError,module,msg)
#define INFOQ(module,msg) DBG_OP_Q(dbg,info,module,msg)
#define INFOB(module) dbg->info_begin(module,__FILE__,__LINE__)
#define WARNINGB(module) dbg->warning_begin(module,__FILE__,__LINE__)
#define ERRORB(module) dbg->error_begin(module,__FILE__,__LINE__)
#define INFOBQ(module) dbg->info_begin(module,__FILE__,0)
#define INFOENDL dbg->info_endl()
#define WARNINGBQ(module) dbg->warning_begin(module,__FILE__,0)
#define ERRORBQ(module) dbg->error_begin(module,__FILE__,0)

#define INFOQSAMELINE(module,msg) DBG_OP_LQ_NEWLINE(0,dbg,info,module,msg,false)
#define INFOSTREAM dbg->info_sameline()
#define INFOSTREAM_NL dbg->info_startline()
#define WARNINGLQ(lvl,module,msg) DBG_OP_LQ(lvl,dbg,warning,module,msg)
#define WARNINGL(lvl,module,msg) DBG_OP_L(lvl,dbg,warning,module,msg)
#define WARNINGQ(module,msg) DBG_OP_Q(dbg,warning,module,msg)
#define ERRORQ(module,msg) DBG_OP_Q(dbg,error,module,msg)
#define FATALQ(module,msg) DBG_OP_Q(dbg,fatalError,module,msg)
#define INFOLQ(lvl,module,msg) DBG_OP_LQ(lvl,dbg,info,module,msg)
#define INFOL(lvl,module,msg) DBG_OP_L(lvl,dbg,info,module,msg)
#if INFO_LEVEL >=9
#define INF9(module,msg) INFOLQ(9,module,msg)
#define INF9IN dbg->increase_depth()
#define INF9OUT dbg->decrease_depth()
#define INF9NEST NESTINFO_GUARD(dbg)
#else
#define INF9(module,msg)
#define INF9IN
#define INF9OUT
#define INF9NEST
#endif

#if INFO_LEVEL >=99
#define INF99(module,msg) INFOLQ(99,module,msg)
#define INF99IN dbg->increase_depth()
#define INF99OUT dbg->decrease_depth()
#define INF99NEST NESTINFO_GUARD(dbg)
#else
#define INF99(module,msg)
#define INF99IN
#define INF99OUT
#define INF99NEST
#endif

#if (defined(TEST) && !defined(QUIET_TEST) )
#define INFOT(msg) DBG_OP(&test_dbg,info,"TEST",msg)
#define WARNT(msg) DBG_OP(&test_dbg,warning,"TEST",msg)
#define NESTT NESTINFO_GUARD(&test_dbg)
#else
#define INFOT(msg) DBG_OP_L(99,dbg,info,"TEST",msg)
#define WARNT(msg) DBG_OP_L(99,dbg,warning,"TEST",msg)
#if INFO_LEVEL >= 99
#define NESTT NESTINFO_GUARD(dbg)
#else
#define NESTT
#endif

#endif

namespace ns_decoder_global {

//! Debug: This is a class to print out debugging information
/*! This should be used for most communication to the user. The reason for this
  is to make sure that all of our debugging messages are consistent (esp
  important for perl readability), and that if for some reason we want to
  redirect error/warning messages to different files, this can be done easily.
*/

class Debug {
 public:
    int runtime_info_level;
    unsigned info_outline_depth;
    unsigned debug_outline_depth;
    void increase_depth() {
        ++info_outline_depth;
    }
    void decrease_depth() {
        if (info_outline_depth == 0)
            warning("Debug","decrease_depth called more times than increase_depth - clamping at 0");
        else
            --info_outline_depth;
    }
    void increase_debug_depth() {
        ++debug_outline_depth;
    }
    void decrease_debug_depth() {
        if (debug_outline_depth == 0)
            warning("Debug","decrease_debug_depth called more times than increase_debug_depth - clamping at 0");
        else
            --debug_outline_depth;
    }
    struct Nest {
        Debug *dbg;
        Nest(Debug *_dbg) : dbg(_dbg) {
            dbg->increase_depth();
        }
        Nest(const Nest &o) : dbg(o.dbg) {
            dbg->increase_depth();
        }
        ~Nest() {
            dbg->decrease_depth();
        }
    };

    Debug() : runtime_info_level(INFO_LEVEL), debugOS(&cerr), infoOS(&cerr),info_outline_depth(0),debug_outline_depth(0),info_atnewline(true) {}

    inline ostream &getDebugOutput() {                     //!< Get the strream to which debugging output is written
        return *debugOS;
    }

    inline ostream &getInfoOutput() {                      //!< Get the stream to which informational output is written
        return *infoOS;
    }
    inline void setDebugOutput(ostream &o) {                     //!< Set the strream to which debugging output is written
        debugOS=&o;
    }

    inline void setInfoOutput(ostream &o) {                      //!< Set the stream to which informational output is written
        infoOS=&o;
    }
    void error(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints an error
        error_begin(module, file, line) << info << endl;
    }

    ostream & error_begin(const string &module, const string &file="", const int line=0)
    {
        getDebugOutput() << "ERROR: " << module;
        if (line)
            getDebugOutput() << "(" << file << ":" << line << ")";
        return getDebugOutput() << ": ";
    }

    void fatalError(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints an error and dies
        error(module,info,file,line,endline);
        assert(0);
        exit(-1);
    }

    ostream & warning_begin(const string &module, const string &file="", const int line=0)
    {
        sync();
        getDebugOutput() << "WARNING: " << module;
        if (line)
            getDebugOutput() << "(" << file << ":" << line << ")";
        return getDebugOutput() << ": ";
    }

    void warning(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints a warning message
        warning_begin(module,file,line) << info << endl;
    }

    bool info_atnewline; // at fresh newline if true, midline if false

    void sync() const
    {
        std::cout.flush();
    }

    inline void info_endl()
    {
        *infoOS << std::endl;
        info_atnewline=true;
    }

    //post: state=midline
    inline ostream &info_sameline() {
        info_atnewline=false;
        return *infoOS;
    }

    //post: state=midline, after printing a fresh newline (if weren't already at one)
    inline ostream &info_startline() {
        sync();
        if (!info_atnewline) {
            info_endl();
        }
        info_atnewline=false;
        return *infoOS;
    }

    //post: state=newline (printing one out if weren't already)
    // this would never be necessary to use if everyone always used info_startline() (except at the very end when closing stream)
    inline ostream &info_endline() {
        if (!info_atnewline) {
            info_endl();
        }
        return *infoOS;
    }

    //!< note: should close this with info_endl() and not just "\n" or endl
    ostream & info_begin(const string &module, const string &file="", const int line=0) { //!< prints an informational message
        const char OUTLINE_CHAR='*';
        info_startline();
        for (unsigned depth=info_outline_depth;depth>0;--depth)
            getInfoOutput() << OUTLINE_CHAR;
        getInfoOutput() << module;

        if (line) {
            getInfoOutput() << "(" << file << ":" << line << ")";
        }
        return getInfoOutput() << ": ";
    }

    void info(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints an informational message
        info_begin(module,file,line) << info;
        if (endline)
            info_endl();
    }

    void set_info_level(int lvl) {
        runtime_info_level=lvl;
    }
    int get_info_level() const {
        return runtime_info_level;
    }
 private:

    ostream *debugOS;                                      //!< output stream where error/WARNING messages are sent
    ostream *infoOS;                                       //!< output stream where debugging information is sent
};
}

#ifdef TEST
# ifdef MAIN
ns_decoder_global::Debug test_dbg;
# endif
#endif


#endif


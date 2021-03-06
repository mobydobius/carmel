// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// wraps some memory-leak-detection facilities of Microsoft C++.  you're better off just using valgrind.
#ifndef MEMLEAK_HPP
#define MEMLEAK_HPP

#ifndef PLACEMENT_NEW
#define PLACEMENT_NEW new
#endif

#if defined(DEBUG) && defined(_MSC_VER)
#  define MEMDEBUG // link to MSVCRT
#  define _CRTDBG_MAP_ALLOC
#  define CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
#  define INITLEAK_DECL _CrtMemState s1, s2, s3;
#  define INITLEAK_DO do { _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE ); _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR ); _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); } while (0)
#  define CHECKLEAK(i) do {  _CrtMemCheckpoint( (((i)%2)?&s1:&s2) ); if ((i)>0) { _CrtMemDifference( &s3, (((i)%2)?&s2:&s1), (((i)%2)?&s1:&s2) ); _CrtMemDumpStatistics( &s3 ); } } while (0)
#  define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)

# else

#  define NEW new
#  define INITLEAK_DECL
#  define INITLEAK_DO
#  define CHECKLEAK(i)
# endif

#  define INITLEAK INITLEAK_DECL INITLEAK_DO

#endif

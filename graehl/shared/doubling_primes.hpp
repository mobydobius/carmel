// Copyright 2014 Jonathan Graehl-http://graehl.org/
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
#ifndef GRAEHL__SHARED__DOUBLING_PRIMES_HPP
#define GRAEHL__SHARED__DOUBLING_PRIMES_HPP
#pragma once

#include <algorithm>

namespace graehl {

namespace detail {


static const unsigned doubling_prime_list[]
    = {53u,        97u,        193u,       389u,       769u,        1543u,       3079u,
       6151u,      12289u,     24593u,     49157u,     98317u,      196613u,     393241u,
       786433u,    1572869u,   3145739u,   6291469u,   12582917u,   25165843u,   50331653u,
       100663319u, 201326611u, 402653189u, 805306457u, 1610612741u, 3221225473u, 4294967291u};

enum { doubling_num_primes = sizeof(doubling_prime_list) / sizeof(doubling_prime_list[0]) };

unsigned const* const last_doubling_prime = doubling_prime_list + doubling_num_primes - 1;
};

// return next doubled (roughly) prime larger than or equal to n
inline unsigned prime_upper_bound(unsigned n) {
  unsigned const* i = detail::doubling_prime_list;
  for (;; ++i)
    if (i == detail::last_doubling_prime || *i >= n) return *i;
}

inline unsigned prime_upper_bound_twice(unsigned n) {
  unsigned const* i = detail::doubling_prime_list;
  for (;; ++i)
    if (i == detail::last_doubling_prime)
      return *i;
    else if (*i >= n)
      return i[1];
}


}

#endif

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
#ifndef GRAEHL_SHARED__DELTA_SUM_REMEMBER_HPP
#define GRAEHL_SHARED__DELTA_SUM_REMEMBER_HPP

#include <graehl/shared/delta_sum.hpp>
#include <graehl/shared/dynamic_array.hpp>

namespace graehl {

// can get any [tmin,tmax] sum - see delta_sum.hpp
//TESTME:
struct delta_sum_remember
{
  typedef dynamic_array<delta> deltas;
  deltas ds;
  double x0, x, tmax;
  double s;
  double sum(double T) const
  {
    if (T>=tmax)
      return s+(tmax-t)*x;
    double s = x0*T;
    for (deltas::const_iterator i = ds.begin(), e = ds.end(); i!=e; ++i) {
      delta const& d=*i;
      double tleft = T-d.t;
      if (tleft>=0)
        s += tleft*d;
    }
  }
  double sum(double t0, double tmax) const
  {
    return sum(tmax)-sum(t0); // FIXME: can just loop once
  }
  double add_delta(delta const& d)
  {
    deltas.push_back(d);
    x += d.d;
    if (tmax<d.t)
      tmax = d.t;
  }
  double add_delta(double d, double t)
  {
    add_delta(delta(d, t));
  }
  double add_val(double v, double t)
  {
    add_delta(v-x, t);
  }
  delta_sum_remember(double x0 = 0.) :x0(x0), x(x0), tlast(0), s(0) {  }
};

}

#endif

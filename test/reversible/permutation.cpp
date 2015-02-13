/* CirKit: A circuit toolkit
 * Copyright (C) 2009-2015  University of Bremen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE permutation

#include <boost/test/unit_test.hpp>

#include <core/utils/bdd_utils.hpp>

#include <reversible/circuit.hpp>
#include <reversible/rcbdd.hpp>
#include <reversible/io/print_circuit.hpp>
#include <reversible/utils/foreach_vshape.hpp>
#include <reversible/utils/permutation.hpp>

using namespace cirkit;

BOOST_AUTO_TEST_CASE(simple)
{
  /* number of lines */
  auto n = 3u;

  /* for RCBDDs */
  rcbdd cf;
  cf.initialize_manager();
  cf.create_variables( n );

  /* statistics */
  auto total    = 0u;
  auto selfinv  = 0u;
  auto selfdual = 0u;
  auto monotone = 0u;
  auto unate    = 0u;
  auto horn     = 0u;

  std::vector<int> ps; /* for unateness test */

  foreach_vshape( n, [&]( const circuit& circ ) {
      permutation_t perm = circuit_to_permutation( circ );
      BDD func = cf.create_from_circuit( circ );

      ++total;
      if ( cf.is_self_inverse( func ) )         { ++selfinv;  }
      if ( is_selfdual( cf.manager(), func ) )  { ++selfdual; }
      if ( is_monotone( cf.manager(), func ) )  { ++monotone; }
      if ( is_unate( cf.manager(), func, ps ) ) { ++unate;    }
      if ( is_horn( cf.manager(), func ) )      { ++horn;     }
    });

  std::cout << "[i] circuits investigated: " << total << std::endl
            << "[i] - self inverse:        " << selfinv << std::endl
            << "[i] - self dual:           " << selfdual << std::endl
            << "[i] - monotone:            " << monotone << std::endl
            << "[i] - unate:               " << unate << std::endl
            << "[i] - horn:                " << horn << std::endl;
}

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:






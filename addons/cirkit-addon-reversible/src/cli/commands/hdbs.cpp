/* CirKit: A circuit toolkit
 * Copyright (C) 2009-2015  University of Bremen
 * Copyright (C) 2015-2017  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hdbs.hpp"

#include <alice/rules.hpp>

#include <core/utils/bdd_utils.hpp>
#include <cli/stores.hpp>
#include <core/utils/timer.hpp>
#include <reversible/circuit.hpp>
#include <cli/reversible_stores.hpp>
#include <reversible/synthesis/dd_synthesis_p.hpp>

namespace cirkit
{

hdbs_command::hdbs_command( const environment::ptr& env )
  : cirkit_command( env, "Hierarchical DD-based synthesis" )
{
  add_option( "--complemented_edges", complemented_edges, "use complemented edges in BDD", true );
  add_option( "--reordering", reordering, "0: CUDD_REORDER_SAME, 1: CUDD_REORDER_NONE, 2: CUDD_REORDER_RANDOM, 3: CUDD_REORDER_RANDOM_PIVOT, 4: CUDD_REORDER_SIFT, 5: CUDD_REORDER_SIFT_CONVERGE, 6: CUDD_REORDER_SYMM_SIFT, 7: CUDD_REORDER_SYMM_SIFT_CONV, 8: CUDD_REORDER_WINDOW2, 9: CUDD_REORDER_WINDOW3, 10: CUDD_REORDER_WINDOW4, 11: CUDD_REORDER_WINDOW2_CONV, 12: CUDD_REORDER_WINDOW3_CONV, 13: CUDD_REORDER_WINDOW4_CONV, 14: CUDD_REORDER_GROUP_SIFT, 15: CUDD_REORDER_GROUP_SIFT_CONV, 16: CUDD_REORDER_ANNEALING, 17: CUDD_REORDER_GENETIC, 18: CUDD_REORDER_LINEAR, 19: CUDD_REORDER_LINEAR_CONVERGE, 20: CUDD_REORDER_LAZY_SIFT, 21: CUDD_REORDER_EXACT", true );
  add_new_option();
}

command::rules hdbs_command::valididity_rules() const
{
  return { has_store_element<bdd_function_t>( env ) };
}

void hdbs_command::execute()
{
  auto& bdd = env->store<bdd_function_t>().current();

  circuit circ;

  {
    properties_timer t( statistics );
    using namespace internal;
    dd graph;
    dd_from_bdd_settings settings;
    settings.complemented_edges = complemented_edges;
    settings.reordering = reordering;
    dd_from_bdd( graph, bdd, settings );
    dd_synthesis( circ, graph );
  }

  print_runtime();

  auto& circuits = env->store<circuit>();
  extend_if_new( circuits );
  circuits.current() = circ;
}

nlohmann::json hdbs_command::log() const
{
  return nlohmann::json( {{"runtime", statistics->get<double>( "runtime" )}} );
}

}

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:

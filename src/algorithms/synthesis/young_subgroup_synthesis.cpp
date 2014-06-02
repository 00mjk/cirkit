/* RevKit: A Toolkit for Reversible Circuit Design (www.revkit.org)
 * Copyright (C) 2009-2011  The RevKit Developers <revkit@informatik.uni-bremen.de>
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

#include "young_subgroup_synthesis.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

#include <boost/assign/std/vector.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/irange.hpp>

#include <core/circuit.hpp>
#include <core/truth_table.hpp>
#include <core/functions/add_circuit.hpp>
#include <core/functions/add_gates.hpp>
#include <core/functions/clear_circuit.hpp>
#include <core/functions/copy_metadata.hpp>
#include <core/functions/fully_specified.hpp>
#include <core/io/write_pla.hpp>
#include <core/utils/timer.hpp>

#include <classical/optimization/esop_minimization.hpp>

#include <cuddObj.hh>

using namespace boost::assign;

namespace revkit
{

typedef std::vector<std::vector<int> > truth_table_column_t;

class young_subgroup_synthesis_manager
{
public:
  young_subgroup_synthesis_manager( circuit& circ, const binary_truth_table& spec ) : circ( circ ), spec( spec ), start( 0u )
  {
    /* initialize BDD variables */
    for ( unsigned i = 0u; i < circ.lines(); ++i )
    {
      cudd.bddVar();
    }
  }

  void add_gate_for_cube( circuit& gatecirc, unsigned target, const cube_t& cube )
  {
    gate::control_container controls;

    for ( unsigned i = 0u; i < circ.lines(); ++i )
    {
      if ( !cube.second[i] ) continue;
      controls += make_var( i, cube.first[i] );
    }

    append_toffoli( gatecirc, controls, target );
  }

  void synthesis_gate( BDD control_function, unsigned target, unsigned& start )
  {
    circuit gatecirc( circ.lines() );

    properties::ptr settings( new properties() );
    settings->set( "on_cube", cube_function_t( [this, &gatecirc, &target]( const cube_t& c ) { add_gate_for_cube( gatecirc, target, c ); } ) );
    settings->set( "verify", true );
    esop_minimization( control_function.manager(), control_function.getNode(), settings );

    if ( gatecirc.num_gates() )
    {
      insert_circuit( circ, start, gatecirc );
      start += gatecirc.num_gates();
    }
  }

  void basic_first_step( unsigned pos )
  {
    unsigned bw = spec.num_inputs();
    for (binary_truth_table::const_iterator it = spec.begin(); it != spec.end(); ++it)
    {
      std::vector<int> in_cube, out_cube;
      binary_truth_table::in_const_iterator c = it->first.first;
      binary_truth_table::out_const_iterator ci = it->second.first;
      for (unsigned i = 0; i < bw; ++i)
      {
        in_cube.push_back(**(c + i));
        out_cube.push_back(**(ci + i));
      }
      vf_in += in_cube;
      vb_in += out_cube;
      out_cube[pos] = -1;
      in_cube[pos] = -1;
      vf_out += in_cube;
      vb_out += out_cube;
    }
  }

  int find(const truth_table_column_t& vect, const truth_table_column_t::value_type& v)
  {
    unsigned i = 0u;
    while (i < vect.size() && !(std::equal(vect[i].begin(), vect[i].end(), v.begin())))
    {
      i++;
    }
    return i;
  }

  BDD get_control_function( const truth_table_column_t& in,
                            const truth_table_column_t& out,
                            unsigned pos )
  {
    BDD bdd = cudd.bddZero();

    for ( unsigned i = 0u; i < in.size(); ++i )
    {
      if ( in[i][pos] == 0u && out[i][pos] == 1u )
      {
        BDD cube = cudd.bddOne();

        for ( unsigned j = 0; j < circ.lines(); ++j )
        {
          if ( j == pos ) continue;
          cube &= in[i][j] == 1 ? cudd.bddVar( j ) : !cudd.bddVar( j );
        }

        bdd |= cube;
      }
    }

    return bdd;
  }

  void add_gates( unsigned pos,
                  unsigned& start)
  {
    BDD bdd_front = get_control_function( vf_in, vf_out, pos );
    BDD bdd_back  = get_control_function( vb_in, vb_out, pos );

    if ( verbose )
    {
      std::cout << "BDD for front:" << std::endl;
      bdd_front.PrintMinterm();
    }
    synthesis_gate(bdd_front, pos, start);
    unsigned back = start;

    if ( verbose )
    {
      std::cout << "BDD for back:" << std::endl;
      bdd_back.PrintMinterm();
    }
    synthesis_gate(bdd_back, pos, back);
  }

  void add_last_gate( unsigned pos, unsigned& start )
  {
    BDD bdd = get_control_function( vf_in, vb_in, pos );

    if ( verbose )
    {
      std::cout << "BDD for middle:" << std::endl;
      bdd.PrintMinterm();
    }
    synthesis_gate( bdd, pos, start);
  }

  void next_step( unsigned pos)
  {
    vf_in = vf_out;
    vb_in = vb_out;
    for (unsigned i = 0; i < vf_in.size(); ++i) {
      vf_out[i][pos] = -1;
      vb_out[i][pos] = -1;

    }
  }

  void build_shape( unsigned pos )
  {
    unsigned j = 0u, nb_cubes = 0u;
    while (j < vf_out.size())
    {
      unsigned k = j;
      while (k < vf_out.size() && vf_out[k][pos] != -1)
      {
        k++;
      }

      if (k < vf_out.size())
      {
        std::vector<int> v = vf_out[k];
        vf_out[k][pos] = 0;
        unsigned index = find(vf_out, v);
        vf_out[index][pos] = 1;
        v = vb_out[index];
        vb_out[index][pos] = 1;
        index = find(vb_out, v);
        vb_out[index][pos] = 0;
        j = index;
        nb_cubes++;
      }
      else
      {
        break;
      }
    }
  }

  void add_gates_for_line( unsigned pos )
  {
    assert( boost::find( adjusted_lines, pos ) == adjusted_lines.end() );

    // preperation of truth table columns
    if ( adjusted_lines.empty() )
    {
      basic_first_step( pos );
    }
    else
    {
      next_step( pos );
    }

    // add gates
    if ( adjusted_lines.size() + 1u < circ.lines() ) /* not the last gate? */
    {
      build_shape( pos );
      add_gates( pos, start );
    }
    else
    {
      add_last_gate( pos, start );
    }

    adjusted_lines += pos;
  }

  Cudd cudd;
  circuit& circ;
  const binary_truth_table& spec;
  std::vector<unsigned> adjusted_lines;
  unsigned start;
  bool verbose;

  truth_table_column_t vf_in, vf_out, vb_in, vb_out;
};


  bool young_subgroup_synthesis(circuit& circ, const binary_truth_table& spec, properties::ptr settings, properties::ptr statistics)
  {
    /* Settings */
    bool                  verbose  = get( settings, "verbose",  false                   );
    std::vector<unsigned> ordering = get( settings, "ordering", std::vector<unsigned>() );

    timer<properties_timer> t;

    if (statistics) {
      properties_timer rt(statistics);
      t.start(rt);
    }

    // circuit has to be empty
    clear_circuit(circ);

    // truth table has to be fully specified
    if (!fully_specified(spec)) {
      set_error_message(statistics, "truth table `spec` is not fully specified.");
      return false;
    }

    circ.set_lines(spec.num_inputs());

    // copy metadata
    copy_metadata(spec, circ);

    // manager
    young_subgroup_synthesis_manager mgr( circ, spec );
    mgr.verbose = verbose;

    // variable ordering
    if ( ordering.empty() )
    {
      boost::push_back( ordering, boost::irange( 0u, spec.num_inputs() ) );
    }

    for ( auto i : ordering )
    {
      mgr.add_gates_for_line( i );
    }

    return true;
  }

  truth_table_synthesis_func young_subgroup_synthesis_func(properties::ptr settings, properties::ptr statistics)
  {
    truth_table_synthesis_func f = [&settings, &statistics]( circuit& circ, const binary_truth_table& spec ) {
      return young_subgroup_synthesis( circ, spec, settings, statistics );
    };
    f.init( settings, statistics );
    return f;
  }

}

// Local Variables:
// c-basic-offset: 2
// End:
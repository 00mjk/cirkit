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

#include "cuts.hpp"

#include <iostream>

#include <boost/range/iterator_range.hpp>

#include <core/utils/bitset_utils.hpp>
#include <core/utils/range_utils.hpp>
#include <classical/functions/cuts/paged.hpp>
#include <classical/mig/mig_cuts_paged.hpp>
#include <classical/functions/cuts/traits.hpp>
#include <classical/utils/cut_enumeration.hpp>
#include <classical/utils/truth_table_utils.hpp>

#include <fmt/format.h>

namespace cirkit
{

cuts_command::cuts_command( const environment::ptr& env ) : aig_mig_command( env, "Computes cuts of an AIG", "Enumerate cuts for %s" )
{
  add_option( "--node_count,-k", node_count, "number of nodes in a cut", true );
  add_flag( "--truthtable,-t", "prints truth tables when verbose" );
  add_flag( "--cone_count,-c", "prints nodes in cut cone when verbose" );
  add_flag( "--depth,-d", "prints depth of cut when verbose " );
  add_flag( "--parallel", "parallel cut enumeration for AIGs" );
  be_verbose();
}

void cuts_command::execute_aig()
{
  paged_aig_cuts cuts( aig(), node_count, is_set( "parallel" ) );
  std::cout << fmt::format( "[i] found {} cuts in {:.2f} secs ({} KB)", cuts.total_cut_count(), cuts.enumeration_time(), cuts.memory() >> 10u ) << std::endl;

  if ( is_verbose() )
  {
    for ( const auto& p : boost::make_iterator_range( vertices( aig() ) ) )
    {
      std::cout << fmt::format( "[i] node {} has {} cuts", p, cuts.count( p ) ) << std::endl;
      for ( const auto& cut : cuts.cuts( p ) )
      {
        std::cout << "[i] - {" << any_join( cut.range(), ", " ) << "}";

        if ( is_set( "cone_count" ) )
        {
          std::cout << fmt::format( " (size: {})", cut.size() );
        }

        if ( is_set( "truthtable" ) )
        {
          std::cout << " " << tt_to_hex( cuts.simulate( p, cut ) );
        }

        std::cout << std::endl;
      }
    }
  }
}

void cuts_command::execute_mig()
{
  mig_cuts_paged cuts( mig(), node_count );
  std::cout << fmt::format( "[i] found {} cuts in {:.2f} secs ({} KB)", cuts.total_cut_count(), cuts.enumeration_time(), cuts.memory() >> 10u ) << std::endl;

  if ( is_verbose() )
  {
    for ( const auto& p : boost::make_iterator_range( vertices( mig() ) ) )
    {
      std::cout << fmt::format( "[i] node {} has {} cuts", p, cuts.count( p ) ) << std::endl;
      for ( const auto& cut : cuts.cuts( p ) )
      {
        std::cout << "[i] - {" << any_join( cut.range(), ", " ) << "}";

        if ( is_set( "cone_count" ) )
        {
          std::cout << fmt::format( " (size: {})", cuts.size( p, cut ) );
        }

        if ( is_set( "depth" ) )
        {
          std::cout << fmt::format( " (depth: {})", cuts.depth( p, cut ) );
        }

        if ( is_set( "truthtable" ) )
        {
          std::cout << " " << tt_to_hex( cuts.simulate( p, cut ) );
        }

        std::cout << std::endl;
      }
    }
  }
}

}

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:

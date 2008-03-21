/*!
 * @author Nick Kidd
 */

#include "wali/Common.hpp"
#include "wali/witness/WitnessTrans.hpp"
#include "wali/witness/Visitor.hpp"

namespace wali
{
  namespace witness
  {
    WitnessTrans::WitnessTrans( const wfa::ITrans& t_ ) : Witness(t_.weight()),t(t_)
    {
    }

    // Destructor does nothing.
    WitnessTrans::~WitnessTrans()
    {
    }

    //
    // Override Witness::accept
    //
    void WitnessTrans::accept( Visitor& v, bool visitOnce ATTR_UNUSED )
    {
      // TODO how does marking work...need a flag maybe
      mark();
      v.visitTrans(this);
    }

    // Overrides Witness::prettyPrint
    std::ostream& WitnessTrans::prettyPrint( std::ostream& o,size_t depth ) const
    {
      formatDepth(o,depth);
      o << "WitnessTrans: ";
      t.print(o) << std::endl;
      return o;
    }

    const wfa::Trans& WitnessTrans::getTrans() const
    {
      return t;
    }

  } // namespace witness

} // namespace wali

/*
 * $Id$
 */

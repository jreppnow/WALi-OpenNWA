/* GT_EXTERNAL_LEGEND(2010) */

#ifndef WALI_DOMAINS_GENKILL_GEN_KILL_BASE_INCLUDED
#define WALI_DOMAINS_GENKILL_GEN_KILL_BASE_INCLUDED

#include <iostream>
#include <climits>
#include <cassert>

#include "wali/SemElem.hpp"

namespace wali {
  namespace domains {
    namespace genkill {

      /*!
       * @class GenKillBase
       *
       * This is a templated class that implements the semiring needed for a
       * gen-kill dataflow-analysis problem. The template parameter must
       * implement the "Set" interface defined below
       *

       class Set {
         public:
           // Only Set constructor GenKillBase invokes
           Set(Set const &);

           static bool Eq(Set const & x, Set const & y);

           static Set Diff(Set const & x,
                           Set const & y,
                           bool normalizing = false);

           static Set Union(Set const & x, Set const & y);

           static Set Intersect(Set const & x, Set const & y);

           static Set const & EmptySet();

           std::ostream & print(std::ostream & o);
       };

       The above exact signatures are only recommended; for instance, the
       functions could also take non-const references (but why would you do
       that to yourself?) or by value; or they could take other optional
       arguments; Diff could be overloaded instead of using a default
       parameter, etc.

       For normal elements, a semiring element is represented by a pair of
       sets (k,g), which have the meaning \x.(x - k) union g.

       Note that if k and g were allowed to be arbitrary sets, it would
       introduce redundant elements into the domain: e.g., ({a,b}, {c,d})
       would have the same meaning as ({a,b,c,d}, {c,d}).  Therefore,
       GenKillBase maintains an invarient that (k intersect g = empty),
       and the operation that builds a semiring element performs the
       normalization (k, g) |-> (k-g, g). When computing (k-g) for purposes
       of this normalization, the Set::Diff function is called with
       'normalizing' equal to 'true.' (At other times, e.g. in 'extend', this
       parameter is left out.) See GenKillWeight for commentary about why
       this might be useful.

       The special elements are
           1. zero
           2. one = (emptyset, emptyset) = \x.x
       Note that zero is a different element from the element (Univ,
       emptyset); it is not just a transformer with specific gen & kill sets,
       it is a completely special value.

       The implementation maintains unique representations for zero and one.
      */
      template<typename ConcreteGenKillType, typename Set>
      class GenKillBase : public wali::SemElem
      {
        typedef ref_ptr<ConcreteGenKillType> ConcreteRefPtr;

      public: // methods

        /// A client uses 'make()' to create a GenKillBase instead
        /// of calling the constructor directly.
        ///
        /// 'make()' normalizes the kill and gen sets before storing them so
        /// that (kill intersect gen == emptyset). It also ensures that there
        /// is a unique representitive of semiring 1 (gen = kill = empty).
        static
        ConcreteRefPtr
        make(Set const & k, Set const & g)
        {
          Set k_normalized = Set::Diff(k, g, true);
          if (shouldMakeOne(k_normalized, g)) {
            return makeOne();
          }
          else {
            return new ConcreteGenKillType(k_normalized, g);
          }
        }

  
        static
        ConcreteRefPtr
        makeZero()
        {
          // Uses a method-static variable to avoid
          // problems with static-initialization order
          static ConcreteRefPtr the_zero = new ConcreteGenKillType(1);
          return the_zero;
        }
  

        static
        ConcreteRefPtr
        makeOne()
        {
          // Uses a method-static variable to avoid problems with
          // static-initialization order
          static ConcreteRefPtr the_one =
            new ConcreteGenKillType(Set::EmptySet(), Set::EmptySet(), 1);
          return the_one;
        }
  


        /// This is a "virtual constructor" for whatever kind of GenKill
        /// weight is being used here.
        virtual
        ConcreteRefPtr
        create(Set const & kill, Set const & gen)
        {
          return make(kill, gen);
        }
        
        
        bool IsOne() const
        {
          if(this == makeOne().get_ptr()) {
            return true;
          }

          assert(!Set::Eq(kill, Set::EmptySet())
                 || !Set::Eq(gen, Set::EmptySet()));

          return false;
        }

        // Zero is a special value that doesn't map to any gen/kill pair, so
        // all we really want out of this is a unique representative.  The
        // gen/kill sets with which it is initialized are arbitrary.
        bool IsZero() const { return is_zero; }


        //-------------------------------------------------
        // Semiring methods
        //-------------------------------------------------

        wali::sem_elem_t one()  const { return makeOne();  }
        wali::sem_elem_t zero() const { return makeZero(); }


        /// Return the extend of x (this) and y.
        ///
        /// Considering x and y as functions, x extend y = y o x,
        /// where (g o f)(v) = g(f(v)).
        ///
        /// FIXME: const: wali::SemElem::extend is not declared as const
        wali::sem_elem_t
        extend( wali::SemElem* _y )
        {
          // Handle special case for either argument being zero():
          // zero extend _y = zero; this extend zero = zero
          if( this->equal(zero()) || _y->equal(zero()) ) {
            return zero();
          }

          // Handle special case for either argument being one()
          if( this->equal(one()) ) {
            // one extend _y = _y
            return _y;
          }
          else if( _y->equal(one()) ) {
            // this extend one = this
            return this;
          }

          const GenKillBase* y = dynamic_cast<GenKillBase*>(_y);

          Set temp_k( Set::Union( kill, y->kill ) );
          Set temp_g( Set::Union( Set::Diff(gen,y->kill), y->gen) );

          return make( temp_k,temp_g );
        }


        // FIXME: const: wali::SemElem::combine is not declared as const
        wali::sem_elem_t
        combine( wali::SemElem* _y )
        {
          // Handle special case for either argument being zero()
          if( this->equal(zero()) ) {
            // zero combine _y = _y
            return _y;
          }
          if( _y->equal(zero()) ) {
            // this combine zero = this
            return this;
          }

          const GenKillBase* y = dynamic_cast<GenKillBase*>(_y);

          Set temp_k( Set::Intersect( kill, y->kill ) );
          Set temp_g( Set::Union( gen, y->gen ) );

          return make( temp_k,temp_g );
        }


        wali::sem_elem_t
        quasiOne() const
        {
          return one();
        }



        // Zero is a special representative that must be compared by address
        // rather by its contained Gen/Kill sets.
        bool isEqual(const GenKillBase* y) const
        {
          // Check for identical arguments: could be two special values
          // (i.e., two zeros or two ones) or two identical instances of a
          // non-special semiring value.
          if(this == y) {
            return true;
          }

          // Return false if any argument is zero.  Zero has a unique
          // representative, and thus the return value could only be true via
          // the preceding check for identicalness.  The same approach could
          // be taken for one, but the extra tests are not worth it.
          if(this->IsZero() || y->IsZero()) {
            return false;
          }

          return Set::Eq(kill,y->kill) && Set::Eq(gen,y->gen);
        }


        bool equal(wali::SemElem* _y) const
        {
          GenKillBase const * y = dynamic_cast<GenKillBase*>(_y);
          return this->isEqual(y);
        }


        bool equal(wali::sem_elem_t _y) const
        {
          return this->equal(_y.get_ptr());
        }


        std::ostream& print( std::ostream& o ) const 
        {
          if(this->IsZero()) {
            return o << "<zero>";
          }
          if(this->IsOne()) {
            return o << "<one>";
          }

          o << "<\\S.(S - {";
          kill.print(o);
          o << "}) U {";
          gen.print(o);
          o << "}>";
          return o;
        }


        std::ostream& prettyPrint( std::ostream& o ) const
        {
          return this->print(o);
        }

        //-------------------------------------------------
        // Other useful methods
        //-------------------------------------------------

        Set apply( const Set & input ) const
        {
          assert(!this->IsZero());
          return Set::Union( Set::Diff(input,kill), gen );
        }

        const Set& getKill() const
        {
          assert(!this->IsZero());
          return kill;
        }


        const Set& getGen() const
        {
          assert(!this->IsZero());
          return gen;
        }


        static std::ostream& print_static_transformers( std::ostream& o )
        {
          o << "ONE\t=\t";    one()->print(o);  o << std::endl;
          o << "ZERO\t=\t";   zero()->print(o); o << std::endl;
          return o;
        }

      protected: // methods -----------------------------------------------------------

        // Constructors
        // The constructors are private to ensure uniqueness of one, zero, and bottom

        // Constructor for legitimate values
        GenKillBase(const Set& k, const Set& g, unsigned int c=0)
          : wali::SemElem()
          , kill(k)
          , gen(g)
          , is_zero(false)
        {
          count = c;
        }

        // Constructor for zero
        GenKillBase(unsigned int c=0)
          : wali::SemElem()
          , is_zero(true)
        {
          count = c;
        }


        // Helpers
        static
        bool
        shouldMakeOne(Set const & k, Set const & g)
        {
          return Set::Eq(k, Set::EmptySet())
            && Set::Eq(g, Set::EmptySet());
        }

      private:
        // members -----------------------------------------------------------
        Set kill, gen;   // Used to represent the function \S.(S - kill) U gen
        bool is_zero;    // True for the zero element, False for all other values
      };


      template< typename Concrete, typename Set >
      std::ostream &
      operator<< (std::ostream& out, GenKillBase<Concrete, Set> const & t)
      {
        t.print(out);
        return out;
      }


    } // namespace genkill
  }
}

// Yo, Emacs!
// Local Variables:
//   c-file-style: "ellemtel"
//   c-basic-offset: 2
// End:

#endif
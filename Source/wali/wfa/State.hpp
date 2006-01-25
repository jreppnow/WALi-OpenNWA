#ifndef wpds_STATE_GUARD
#define wpds_STATE_GUARD 1

/*!
 * @author Nick Kidd
 */

#include "wali/Common.hpp"
#include "wali/Printable.hpp"
#include "wali/Markable.hpp"
#include "wali/Countable.hpp"
#include "wali/SemElem.hpp"
#include <list>

namespace wali
{
    namespace wpds
    {
        class WPDS;
        namespace ewpds 
        {
            class EWPDS;
        }
    }

    namespace wfa
    {
        class CA;
        class Trans;
        class State;
        //typedef State* state_t;
        //typedef ref_ptr<State> state_t;

        /*!
         * @class State
         *
         * This class represents a state in a CA. It extends
         * Markable so States can be in a Worklist for querying
         * a WFA.
         *
         * @see WFA
         * @see SemElem
         */
        class State : public Printable, public Markable, public Countable
        {
            public: // friends
                friend class WFA;
                friend class wali::wpds::WPDS;
                friend class wali::wpds::ewpds::EWPDS;

            public: // typedefs
                typedef std::list< Trans* > trans_list_t;
                typedef trans_list_t::iterator iterator;
                typedef trans_list_t::const_iterator const_iterator;

            public: // static vars
                static int numStates;
                static const std::string XMLTag;
                static const std::string XMLInitialTag;
                static const std::string XMLFinalTag;
                static const std::string XMLNameTag;

            public:

                State( wali_key_t name, sem_elem_t W );

                virtual ~State();

                /*!
                 * overrides (implements) Printable::print method
                 *
                 * @param o the std::ostream this is written to
                 * @return std::ostream passed in
                 *
                 * @see Printable
                 */
                virtual std::ostream & print( std::ostream & o ) const;

                /*!
                 * Add Trans* t to the set of transitions that lead into this
                 * State.
                 */
                void add_rev_trans( Trans * t );

                /*!
                 * Return a reference to the State's weight
                 */
                sem_elem_t& weight() {
                    return se;
                }

                const sem_elem_t& weight() const {
                    return se;
                }

                /*!
                 * Return reference to the State's delta
                 */
                sem_elem_t& delta() {
                    return delta_se;
                }

                /*!
                 * Return the key associated with this State
                 */
                wali_key_t name() const {
                    return key;
                }

                iterator rbegin()
                {
                    return rev_trans_ls.begin();
                }

                iterator rend()
                {
                    return rev_trans_ls.end();
                }

                bool eraseTransFromReverseList(
                        Key from,
                        Key stack,
                        Key to );

                void clearTransLists();

            protected:
                bool eraseTransFromList(
                        Key from,
                        Key stack,
                        Key to,
                        trans_list_t& ls );

            protected:
                wali_key_t key;
                sem_elem_t se;
                sem_elem_t delta_se;
                sem_elem_t quasi;
                trans_list_t rev_trans_ls;

        }; //class State

    } // namespace wfa

} // namespace wali

#endif  // wpds_STATE_GUARD

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */

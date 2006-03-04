/*!
 * @author Nick Kidd
 */

#include "wali/Common.hpp"
#include "wali/DefaultWorklist.hpp"
#include "wali/wfa/WFA.hpp"
#include "wali/wfa/State.hpp"
#include "wali/wfa/TransFunctor.hpp"
#include "wali/wfa/Trans.hpp"
#include "wali/wfa/WeightMaker.hpp"
#include <iostream>

#define FOR_EACH_STATE(name)                        \
    State* name;                                    \
    state_map_t::iterator it = state_map.begin();   \
    state_map_t::iterator itEND = state_map.end();  \
    for( ; it != itEND && (0 != (name = it->second)) ; it++ )

namespace wali
{
    namespace wfa
    {
        const std::string WFA::XMLTag("WFA");
        const std::string WFA::XMLQueryTag("query");
        const std::string WFA::XMLInorderTag("INORDER");
        const std::string WFA::XMLReverseTag("REVERSE");

        WFA::WFA( query_t q ) : init_state( WALI_EPSILON ),query(q)
        {
            if( query == MAX ) {
                std::cerr << "[WARNING] Invalid WFA::query. Resetting to INORDER.\n";
                query = INORDER;
            }
        }

        WFA::WFA( const WFA & rhs ) : Printable()
        {
            operator=(rhs);
        }

        WFA& WFA::operator=( const WFA & rhs )
        {
            if( this != &rhs )
            {
                clear();

                // Copy important state information
                init_state = rhs.init_state;
                F = rhs.F;
                query = rhs.query;

                // Be sure to copy the state_map. Otherwise, 
                // some states that are in rhs but not actually apart
                // of a transition will be lost.
                state_map_t::const_iterator it = rhs.state_map.begin();
                for( ; it != rhs.state_map.end(); it++ ) {
                    addState( it->first, it->second->weight()->zero() );
                }

                // This will populate all maps
                TransCopier copier(*this);
                rhs.for_each( copier );
            }
            return *this;
        }

        WFA::~WFA()
        {
            clear();
        }

        void WFA::clear()
        {
            /* Must manually delete all Trans objects. If reference
             * counting is used this code can be removed
             */
            TransDeleter td;
            for_each(td);


            /* Must manually delete all State objects. If reference
             * counting is used this code can be removed
             */
            state_map_t::iterator it = state_map.begin();
            state_map_t::iterator itEND = state_map.end();
            for( ; it != itEND ; it++ )
            {
                delete it->second;
                it->second = 0;
            }

            /* Clear all of the maps to release references to stale
             * objects */
            kpmap.clear();
            eps_map.clear();
            state_map.clear();
            F.clear();
            Q.clear();
            init_state = WALI_EPSILON;
        }

        //!
        // @brief set initial state
        //
        Key WFA::set_initial_state( Key key )
        {
            Key isold = init_state;
            // TODO : Add debug check to verify key exists
            init_state = key;

            return isold;
        }

        //!
        // @brief set initial state
        //
        Key WFA::setInitialState( Key key )
        {
            Key isold = init_state;
            // TODO : Add debug check to verify key exists
            init_state = key;

            return isold;
        }

        //!
        // Add parameter key to the set of final states
        //
        void WFA::add_final_state( Key key )
        {
            F.insert(key);
        }

        //!
        // Add parameter key to the set of final states
        //
        void WFA::addFinalState( Key key )
        {
            F.insert(key);
        }

        //!
        // Return true if parameter key is a final state
        //
        bool WFA::is_final_state( Key key ) const
        {
            return (F.find(key) != F.end());
        }

        /*!
         * Return true if parameter key is a final state
         */
        bool WFA::isFinalState( Key key ) const
        {
            return (F.find(key) != F.end());
        }

        //
        // @brief Return the initial state
        //
        Key WFA::initial_state() const
        {
            return init_state;
        }

        //
        //  @brief Return the initial state
        //
        Key WFA::getInitialState() const
        {
            return init_state;
        }

        //
        // Test if param key is the initial state.
        // 
        bool WFA::is_initial_state( Key key ) const
        {
            return key == init_state;
        }

        //
        // Set the query
        //
        WFA::query_t WFA::setQuery( WFA::query_t newQuery )
        {
            query_t old = query;
            query = newQuery;
            return old;
        }

        //
        // get the query
        //
        WFA::query_t WFA::getQuery() const
        {
            return query;
        }

        //!
        // @brief add trans (p,g,q,se) to WFA
        //
        void WFA::addTrans(
                Key p,
                Key g,
                Key q,
                sem_elem_t se )
        {
            addTrans( new Trans(p,g,q,se) );
        }

        //!
        // @brief add Trans *t to WFA
        // Takes care of adding states and calling insert. This
        // method (actually insert) assumes ownership of the memory
        // pointed to by the Trans* t.
        //
        void WFA::addTrans( Trans * t )
        {
            //t->print( std::cerr << "Inserting '" ) << "' into:\n";
            //print( std::cerr );
            //ZERO->print( std::cerr << "ZERO: " ) << std::endl;
            sem_elem_t ZERO = t->weight()->zero();
            { // BEGIN DEBUGGING
                assert(ZERO.is_valid());
            } // END DEBUGGING

            //std::cerr << "\tAdding 'from' state'" << key2str(t->from()) << "'\n";
            addState( t->from(), ZERO );

            //std::cerr << "\tAdding 'to' state '" << key2str(t->to()) << "'\n";
            addState( t->to(), ZERO );

            //t->print( std::cerr << "\tInserting Trans" ) << std::endl;
            insert( t );
        }

        //
        // Erase a trans
        // Must remove from kpmap, eps_map, and State('from')
        //
        void WFA::erase(
                Key from,
                Key stack,
                Key to )
        {
            // Remove from kpmap
            Trans* tKp = eraseTransFromKpMap(from,stack,to);

            if( tKp != NULL )
            {
                Trans* tEps = eraseTransFromEpsMap(tKp);

                { // BEGIN DEBUGGING
                    assert( tKp == tEps );
                } // END DEBUGGING

                State* state = state_map.find(from)->second;
                state->eraseTrans(tKp);
                delete tKp;
            }
        }

        //
        // Removes State q from the WFA and any transitions leading
        // to and from q.
        //
        bool WFA::eraseState(
                Key q
                )
        {
            bool eraseSuccess = false;
            state_map_t::iterator it = state_map.find(q);
            if( it == state_map.end() ) {
                return false;
            }
            return eraseState( it->second );
        }

        //
        // Finds Trans(p,g,q). Invokes copy constructor on parameter t.
        // Returns true if find was successful.
        //
        bool WFA::find( 
                Key p,
                Key g,
                Key q,
                Trans & t )
        {
            bool rc = false;
            Trans *tret = 0;
            KeyPair kp(p,g);
            kp_map_t::iterator it = kpmap.find(kp);
            if( it != kpmap.end() )
            {
                TransSet& transSet = it->second;
                TransSet::iterator tsit= transSet.find(p,g,q);
                if( tsit != transSet.end() ) {
                    tret = *tsit;
                }

                /*
                trans_list_t &ls = it->second;
                trans_list_t::iterator lit = ls.begin();
                for( ; lit != ls.end() ; lit ++ )
                {
                    Trans *ttmp = *lit;
                    // already matched (p,g,*) so just
                    // check q
                    if( q == ttmp->to() )
                    {
                        tret = ttmp;
                        break;
                    }
                }
                */
            }
            if( 0 != tret ) {
                rc = true;
                t = *tret;
            }
            return rc;
        }

        void WFA::for_each( ConstTransFunctor & tf ) const
        {
            kp_map_t::const_iterator it = kpmap.begin();
            kp_map_t::const_iterator itEND = kpmap.end();
            for( ; it != itEND ; it++ )
            {
                const TransSet & transSet = it->second;
                transSet.each(tf);
            }
        }

        void WFA::for_each( TransFunctor& tf )
        {
            kp_map_t::iterator it = kpmap.begin();
            kp_map_t::iterator itEND = kpmap.end();
            for( ; it != itEND ; it++ )
            {
                TransSet & transSet = it->second;
                transSet.each(tf);
            }
        }

        // TODO : move this class somewhere else
        class StackHasher : public TransFunctor
        {
            public:
                typedef wali::HashMap< Key , TransSet > stackmap_t;
                stackmap_t stackmap;

                virtual ~StackHasher() {}

                virtual void operator()( Trans * t )
                {
                    Key stack = t->stack();
                    stackmap_t::iterator it = stackmap.find( stack );
                    if( it == stackmap.end() )
                    {
                        TransSet transSet;
                        it = stackmap.insert(stack,transSet).first;
                    }
                    it->second.insert(t);
                }
        };

        /*!
         * Intersect this with parameter fa. This is a wrapper
         * for intersect( WeightMaker&,WFA& ) that passes
         * the WeightMaker KeepBoth.
         *
         * @see WeightMaker
         */
        WFA WFA::intersect( WFA& fa )
        {
            KeepBoth wmaker;
            return intersect(wmaker,fa);
        }

        void WFA::intersect( WFA& fa, WFA& dest )
        {
            KeepBoth wmaker;
            intersect(wmaker,fa,dest);
        }

        /* 
         * Intersect this and fa, returning the result
         */
        WFA WFA::intersect( WeightMaker& wmaker , WFA& fa )
        {
            WFA dest;
            intersect(wmaker,fa,dest);
            return dest;
        }

        //
        // Intersect this and fa, storing the result in dest
        // TODO: Note: if this == dest there might be a problem
        //
        void WFA::intersect(
                WeightMaker& wmaker
                , WFA& fa
                , WFA& dest )
        {
            // Hash transitions of fa on stack symbol. Then probe the hash
            // with this's transitions to add transitions to dest
            StackHasher hashfa;
            fa.for_each(hashfa);

            // Store init state
            Key dest_init_state = getKey(initial_state(),fa.initial_state());

            // Store final states
            // Pairwise cross of the sets
            std::set< Key > dest_final_states;
            std::set< Key >::iterator keyit = F.begin();
            std::set< Key >::iterator keyitEND = F.end();
            for( ; keyit != keyitEND ; keyit++ )
            {
                std::set< Key >::iterator fait = fa.F.begin();
                std::set< Key >::iterator faitEND = fa.F.end();
                for( ; fait != faitEND ; fait++ )
                {
                    dest_final_states.insert( getKey(*keyit,*fait) );
                }
            }

            // Do this here in case dest == this
            sem_elem_t stateWeight = 
                wmaker.make_weight( getState(initial_state())->weight()->zero()
                        , fa.getState(fa.initial_state())->weight()->zero() ); 

            // Reset dest
            dest.clear();

            // Note: We need to make sure the state exists b/c
            // set_initial_state cannot call addState because there is no
            // weight to call it with
            dest.addState( dest_init_state, stateWeight->zero() );
            // Set dest init state
            dest.set_initial_state( dest_init_state );

            // Set dest final states
            keyit = dest_final_states.begin();
            keyitEND = dest_final_states.end();
            for( ; keyit != keyitEND ; keyit++ )
            {
                Key f = *keyit;
                dest.addState(f,stateWeight->zero());
                dest.add_final_state(f);
            }

            // Perform intersection
            kp_map_t::iterator kpit = kpmap.begin();
            kp_map_t::iterator kpitEND = kpmap.end();
            for( ; kpit != kpitEND ; kpit++ )
            {
                TransSet & transSet = kpit->second;
                if( transSet.empty() )
                {
                    continue;
                }
                // Probe StackHasher outside of the inner
                // for loop b/c the kp_map_t hashes on (p,g)
                // so each trans in the list has stack symbol g
                TransSet::iterator tsit = transSet.begin();
                Trans *tprobe = *tsit;
                StackHasher::stackmap_t::iterator stkit;
                stkit = hashfa.stackmap.find( tprobe->stack() );

                if( stkit == hashfa.stackmap.end() )
                {
                    continue;
                }
                for( ; tsit != transSet.end() ; tsit++ )
                {
                    // Probe StackHasher::stackmap with each trans
                    Trans *t = *tsit;
                    // match t w/ all trans in the stkit->second
                    TransSet::iterator stklsit = stkit->second.begin();
                    TransSet::iterator stklsitEND = stkit->second.end();
                    for( ; stklsit != stklsitEND ; stklsit++ )
                    {
                        Trans *t2 = *stklsit;
                        Key fromkey = getKey( t->from(),t2->from() );
                        Key tokey = getKey( t->to(),t2->to() );
                        sem_elem_t W = wmaker.make_weight(t->weight(),t2->weight());
                        dest.addTrans( new Trans(fromkey,t->stack(),tokey,W) );
                    }
                }
            }
            // TODO: should the query be set by the user?
            dest.setQuery( getQuery() );
        }

        /*
        void WFA::do_fixpoint( Worklist& wl, FixpointLogic& logic )
        {
            setupFixpoint(wl,logic);
            while( !wl.empty() )
            {
                State* q = wl.get();
                sem_elem_t the_delta = q->delta();
                q->delta() = the_delta->zero();

                logic.process(q,the_delta,wl);
            }
        }
        */

        //
        // Calls path_summary with default Worklist
        //
        void WFA::path_summary()
        {
            DefaultWorklist<State> wl;
            path_summary(wl);
        }


        //
        // Computes path_summary
        //
        void WFA::path_summary( Worklist<State>& wl )
        {
            // BEGIN DEBUGGING
            int numPops = 0;
            // END DEBUGGING
            PredHash_t preds;
            setupFixpoint( wl,preds );
            while( !wl.empty() )
            {
                { // BEGIN DEBUGGING
                    numPops++;
                } // END DEBUGGING
                State* q = wl.get();
                sem_elem_t the_delta = q->delta();
                q->delta() = the_delta->zero();

                // Get a handle on ZERO b/c we use it alot
                sem_elem_t ZERO = q->weight()->zero();

                // Find predecessor set
                PredHash_t::iterator predHashIt = preds.find(q->name());

                // Some states may have no predecessors, like
                // the initial state
                if(  predHashIt == preds.end() )
                {
                    continue;
                }

                // Tell predecessors we have changed
                StateSet_t& stateSet = predHashIt->second;
                StateSet_t::iterator stateit = stateSet.begin();
                for( ; stateit != stateSet.end() ; stateit++ )
                {
                    State* qprime = *stateit;

                    // For each Trans (q',x,q)
                    State::iterator tit = qprime->begin();
                    State::iterator titEND = qprime->end();
                    // the new W(q')
                    sem_elem_t newW = qprime->weight()->zero();
                    // For each (q',_,q)
                    for( ; tit != titEND ; tit++ )
                    {
                        Trans* t = *tit; // (q',_,q)
                        //t->print( std::cerr << "\t++ Popped " ) << std::endl;
                        if( t->to() == q->name() ) {

                            sem_elem_t extended;
                            if( query == INORDER )
                                extended = t->weight()->extend( the_delta );
                            else
                                extended = the_delta->extend( t->weight() );
                            newW = newW->combine(extended);
                        }

                    }
                    // delta => (w+se,w-se)
                    // Use extended->delta b/c we want the diff b/w the new
                    // weight (extended) and what was there before
                    std::pair< sem_elem_t,sem_elem_t > p =
                        newW->delta( qprime->weight() );

                    //p.first->print( std::cerr << "\t++ p.first " ) << std::endl;
                    //p.second->print( std::cerr << "\t++ p.second " ) << std::endl;
                    // Sets qprime's new weight
                    // p.first == (l(t) X the_delta) + W(qprime)
                    qprime->weight() = p.first;

                    // on the worklist?
                    if( qprime->marked() ) {
                        qprime->delta() = qprime->delta()->combine(p.second);
                    }
                    else {
                        // not on the worklist means its delta is zero
                        qprime->delta() = p.second;

                        // add to worklist if not zero
                        if( !qprime->delta()->equal(ZERO) ) {
                            wl.put(qprime);
                        }
                    }
                }
            }
            { // BEGIN DEBUGGING
                std::cerr << "WFA::path_summary needed " << numPops << " pops\n";
                std::cerr << "WFA state labels:\n";
                FOR_EACH_STATE( st ) {
                    std::cerr << "\t" << key2str(st->name()) << ": ";
                    st->weight()->print( std::cerr ) << std::endl;
                }
            } // END DEBUGGING
        }

        //
        // Removes all transitions in the (init_state,F) chop
        // TODO: broken
        //
        void WFA::prune()
        {

            DefaultWorklist<State> wl;
            PredHash_t preds;
            setupFixpoint( wl,preds );
            // first backwards prune
            while( !wl.empty() )
            {
                State* q = wl.get();
                PredHash_t::iterator predHashIt = preds.find(q->name());
                if( predHashIt == preds.end() ) {
                    continue;
                }
                StateSet_t& stateSet = predHashIt->second;
                StateSet_t::iterator stateIt = stateSet.begin();
                StateSet_t::iterator stateItEND = stateSet.end();
                sem_elem_t ONE = q->weight()->one();
                sem_elem_t ZERO = q->weight()->zero();

                for( ; stateIt != stateItEND ; stateIt++ )
                {
                    State* p = *stateIt;
                    if( p->weight()->equal(ZERO) ) {
                        p->weight() = ONE;
                        wl.put(p);
                    }
                }
            }

            //
            // States that have a weight of 1 are reachable from
            // the accepting states. If the State has a weight of
            // zero it can be erased.
            //
            state_map_t::iterator it = state_map.begin();
            state_map_t::iterator itEND = state_map.end();
            for( ; it != itEND ; it++ )
            {
                State* q = it->second;
                if( q->weight()->equal( q->weight()->zero() ) ) {
                    { // BEGIN DEBUGGING
                        //std::cerr << "Erasing State '" << key2str(q->name()) << "'\n";
                    } // END DEBUGGING
                    eraseState(q);
                }
            }

            //
            // second forward prune
            // TODO: implement this later
            //
        }

        //
        // @brief print WFA to param o
        //
        std::ostream & WFA::print( std::ostream & o ) const
        {
            o << "WFA -\n";
            o << "  Initial State : ";
            o << key2str(init_state) << std::endl;

            // Q
            o << "  Q: {";
            std::set< Key >::const_iterator cit = Q.begin();
            std::set< Key >::const_iterator citEND = Q.end();
            for( bool first=true; cit != citEND ; cit++,first=false )
            {
                if(!first)
                    o << ", ";
                o << key2str( *cit );
            }
            o << "}\n";

            // F
            o << "  F: {";
            cit = F.begin();
            citEND = F.end();
            for( bool first=true; cit != citEND ; cit++,first=false )
            {
                if(!first)
                    o << ", ";
                o << key2str( *cit );
            }
            o << "}\n";

            TransPrinter printer( o );
            for_each( printer );
            return o;
        }

        //
        // @brief Print WFA in dot format
        //
        std::ostream& WFA::print_dot(
                std::ostream& o,
                bool print_weights ) const
        {
            o << "digraph \"WFA@" << std::hex << (void*)this << std::dec << "\" {\n";
            TransDotty dotter( o, print_weights );
            for_each(dotter);
            state_map_t::const_iterator stit = state_map.begin();
            state_map_t::const_iterator stitEND = state_map.end();
            for( ; stit != stitEND ; stit++ )
            {
                Key key = stit->first;
                o << "\t" << key << " [label=\"";
                o << key2str(key);
                o << "\"";
                if( is_initial_state(key) ) {
                    o  << ",color=green,style=filled";
                }
                else if( is_final_state(key) ) {
                    o  << ",color=light_blue,style=filled";
                }
                o << "];\n";
            }

            o << "}\n";
            return o;
            
        }

        //
        // @brief marshall WFA in XML 
        //
        std::ostream& WFA::marshall( std::ostream& o ) const
        {
            // TODO : add "forward" or "backward" attribute ... like
            // o << "<WFA direction="forward">\n";
            o << "<" << XMLTag;
            switch( query ) {
                case REVERSE:
                    o << " " << XMLQueryTag << "='REVERSE'>\n";
                    break;
                case INORDER:
                case MAX:
                    o << " " << XMLQueryTag << "='INORDER'>\n";
                    break;
            }

            // do initial state
            marshallState(o,init_state);

            // do final states
            std::set< Key >::const_iterator Fit = F.begin();
            std::set< Key >::const_iterator FitEND = F.end();
            for( ; Fit != FitEND ; Fit++ )
            {
                marshallState(o,*Fit);
            }
            TransMarshaller marsh(o);
            for_each(marsh);
            o << "</" << XMLTag << ">";
            return o;
        }

        std::ostream& WFA::marshallState( std::ostream& o,Key key ) const
        {
            // <State
            o << "\t<" << State::XMLTag;
            // _Name='<name>'
            o << " " << State::XMLNameTag << "='" << key2str( key ) << "'";
            if( is_initial_state(key) ) {
                // _initial='TRUE'
                o << " " << State::XMLInitialTag << "='TRUE'";
            }
            if( is_final_state(key) ) {
                // if is final, then _final='TRUE'
                o << " " << State::XMLFinalTag << "='TRUE'";
            }
            getState(key)->weight()->marshall(o << ">");
            o << "</" << State::XMLTag << ">\n";
            return o;
        }

        //
        // Inserts tnew into the WFA. If a transition matching tnew
        // exists, tnew is deleted.
        //
        Trans * WFA::insert( Trans * tnew )
        {
            ////
            // WFA::find code duplicated to keep
            // a handle on the kp_map_t::iterator
            ////
            Trans * told = 0;
            kp_map_t::iterator it = kpmap.find(tnew->keypair());
            if( it != kpmap.end() )
            {
                TransSet& transSet = it->second;
                TransSet::iterator tsit= transSet.find(tnew);
                if( tsit != transSet.end() ) {
                    told = *tsit;
                }
                /*
                trans_list_t &ls = it->second;
                trans_list_t::iterator lit = ls.begin();
                trans_list_t::iterator litEND = ls.end();
                for( ; lit != litEND ; lit++ )
                {
                    Trans *ttmp = *lit;
                    // already matched (p,g,*) so just
                    // check ->to() field 
                    if( tnew->to() == ttmp->to() )
                    {
                        told = ttmp;
                        break;
                    }
                }
                */
            }
            ////
            // Cases
            //  if 0 == told (told does not exist)
            //      if it == kpmap.end
            //          (p,g,*) does not exist
            //      else
            //          just add (p,g,q) to it.second
            //  else (0 != told)
            //      combine new val w/ old
            //
            ////
            if( 0 == told )
            {
                if( it == kpmap.end() )
                {
                    TransSet transSet;
                    //trans_list_t ls;
                    it = kpmap.insert(tnew->keypair(),transSet).first;
                }
                it->second.insert(tnew);
                //it->second.push_back(tnew);

                // Set told to tnew for the return statement at end of 
                // method
                told = tnew;

                // Add tnew to the 'to' State's reverse trans list
                state_map_t::iterator from_stit = state_map.find( tnew->from() );

                { // BEGIN DEBUGGING
                    if( from_stit == state_map.end() ) {
                        tnew->print( std::cerr << "\n\n+++ WTF +++\n" ) << std::endl;
                        assert( Q.find(tnew->from()) != Q.end() );
                        assert( from_stit != state_map.end() );
                    }
                } // END DEBUGGING

                from_stit->second->addTrans( tnew );

                // if tnew is an eps transition add to eps_map
                if( tnew->stack() == WALI_EPSILON )
                {
                    eps_map_t::iterator epsit = eps_map.find( tnew->to() );
                    if( epsit == eps_map.end() ) {
                        TransSet transSet;
                        epsit = eps_map.insert( tnew->to(),transSet ).first;
                        // TODO: ERASE
                        //trans_list_t ls;
                        //epsit = eps_map.insert( tnew->to(),ls ).first;
                    }
                    epsit->second.insert( tnew );
                    // TODO: ERASE
                    //epsit->second.push_back( tnew );
                }
            }
            else {
                // Safety check. If told == tnew then the combine
                // is a waste of effort and the deletion is just plain
                // wrong.
                if( told != tnew ) {
                    // combine new into old
                    combineTrans( told,tnew );
                    delete tnew;
                }
                else {
                    std::cerr << "[WARNING - WFA::insert]\n";
                    std::cerr << "\tAttempt to insert 'Trans' already owned by WFA.\n";
                }
            }
            return told;
        }

        //
        // Add a state to the state map
        //
        void WFA::addState( Key key , sem_elem_t zero )
        {
            if( state_map.find( key ) == state_map.end() ) {
                State* state = new State(key,zero);
                state_map.insert( key , state );
                Q.insert(key);
            }
        }

        const State* WFA::getState( Key name ) const
        {
            state_map_t::const_iterator stit = state_map.find( name );
            if( stit == state_map.end() ) {
                //throw NoSuchStateException( name );
                return NULL;
            }
            else {
                const State * state = stit->second;
                //return *state;
                return state;
            }
        }

        const std::set< Key >& WFA::getStates() const
        {
            return Q;
        }

        const std::set< Key >& WFA::getFinalStates() const
        {
            return F;
        }

        ///////////////////////
        // Begin protected WFA
        ///////////////////////

        //
        // This is a virtual method.
        //
        void WFA::combineTrans( Trans * told, Trans * tnew )
        {
            told->combine_weight( tnew->weight() );
        }

        //
        // Return State * corresponding to the key
        //
        State * WFA::getState( Key name )
        {
            state_map_t::iterator stit = state_map.find( name );
            if( stit == state_map.end() ) {
                //throw NoSuchStateException( name );
                return NULL;
            }
            else {
                State * state = stit->second;
                //return *state;
                return state;
            }
        }

        //
        // Place WFA in state ready for fixpoint
        //
        void WFA::setupFixpoint( Worklist<State>& wl, PredHash_t& preds )
        {
            state_map_t::iterator it = state_map.begin();
            state_map_t::iterator itEND = state_map.end();
            for( ; it != itEND ; it++ )
            {
                // State p
                State* st = it->second;
                st->unmark();
                if( isFinalState( st->name() ) ) {
                    st->weight() = st->weight()->one();
                    st->delta() = st->weight();
                    wl.put( st );
                }
                else {
                    st->weight() = st->weight()->zero();
                    st->delta() = st->weight();
                }

                State::iterator stit = st->begin();
                State::iterator stEnd = st->end();

                for( ; stit != stEnd; stit++ ) {
                    // (p,_,q)
                    Trans* t = *stit;
                    Key toKey = t->to();
                    PredHash_t::iterator predIt = preds.find(toKey);
                    if( predIt == preds.end() ) {
                        StateSet_t stateSet;
                        predIt = preds.insert(toKey,stateSet).first;
                    }
                    //std::cerr << "Adding '" << key2str(st->name()) << "' to pred of '";
                    //std::cerr << key2str(toKey) << "'\n";
                    predIt->second.insert( st );
                }
            }
        }

        //
        // Removes Trans(from,stack,to) from the kpmap. Returns true
        // if the Trans was erased, i.e., the Trans existed.
        //
        Trans* WFA::eraseTransFromKpMap(
                Key from,
                Key stack,
                Key to )
        {
            Trans* tret = NULL;

            // ignore weight on Trans
            Trans terase(from,stack,to,0);
            kp_map_t::iterator kpit = kpmap.find( terase.keypair() );

            // remove from kpmap
            if( kpit != kpmap.end() ) {
                tret = kpit->second.erase(&terase);
            }
            return tret;
        }

        Trans* WFA::eraseTransFromEpsMap( Trans* terase )
        {
            Trans* tret = NULL;
            if( terase->stack() == WALI_EPSILON )
            {
                // remove from epsmap
                // This loop could be moved to its own method
                eps_map_t::iterator epit = eps_map.find( terase->to() );
                if( epit != eps_map.end() ) {
                    tret = epit->second.erase(terase);
                }
            }
            return tret;
        }

        //
        // Removes State state from the WFA and any transitions leading
        // to and from state.
        //
        bool WFA::eraseState(
                State* state
                )
        {
            State::iterator it = state->begin();
            State::iterator itEND = state->end();
            for( ; it != itEND ; it++ )
            {
                Trans* stateTrans = *it;
                Key from = stateTrans->from();
                Key stack = stateTrans->stack();
                Key to = stateTrans->to();
                // TODO: is this necessary
                // eraseTransFromEpsMap( Trans* )?
                Trans* tKp = eraseTransFromKpMap( from,stack,to );

                { // BEGIN DEBUGGING
                    if( tKp != NULL && tKp != stateTrans ) {
                        std::cerr << "[ERROR] WFA::eraseState\n";
                        tKp->print( std::cerr << "\tKpMap Trans: " ) << "\n";
                        stateTrans->print( std::cerr << "\tState Trans: " ) << "\n";
                        // Make sure we fail 
                        assert( tKp != NULL && stateTrans == tKp );
                    }
                    else {
                        // Fail if tKp == NULL
                        assert(tKp);
                    }
                } // END DEBUGGING

                Trans* teps = eraseTransFromEpsMap(tKp);

                { // BEGIN DEBUGGING
                    if( teps != NULL ) {
                        assert( tKp == teps );
                    }
                } // END DEBUGGING

                delete tKp;
            }

            // Remove from state map
            // TODO: Is this necessary?
            //state_map.erase( state->name() );
            //Q.erase(state->name());
            //F.erase(state->name());
            //if( state->name() == getInitialState() ) {
            //    setInitialState( WALI_EPSILON );
            //}

            // Since we are not deleting the State, we need
            // to clear its TransLists
            state->clearTransSet();

            // delete the memory
            //delete state;

            return true;
        }

    } // namespace wfa

} // namespace wali

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */

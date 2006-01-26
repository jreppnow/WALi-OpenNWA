/*!
 * $Id: Reach.cpp,v 1.2 2005/06/10 02:10:15 kidd Exp $
 *
 * @author Nick Kidd
 * @version $Revision: 1.2 $
 */

#include "Reach.hpp"

int Reach::numReaches = 0;

Reach::Reach( bool b ) :
    isreached(b)
{
    numReaches++;
    //std::cerr << "Reach(...) :" << numReaches << std::endl;
}

Reach::~Reach()
{
    numReaches--;
    //std::cerr << "~Reach()   :" << numReaches << std::endl;
}

sem_elem_t Reach::one() const
{
    return new Reach(true);
}

sem_elem_t Reach::zero() const
{
    return new Reach(false);
}

// zero is the annihilator for extend
sem_elem_t Reach::extend( SemElem* se )
{
    Reach* rhs = static_cast< Reach* >(se);
    // this and rhs are one()
    if( isreached && rhs->isreached )
        return one();
    else
        return zero();
}

// zero is neutral for combine
sem_elem_t Reach::combine( SemElem* se )
{
    Reach* rhs = static_cast< Reach* >(se);
    if( isreached || rhs->isreached )
        return one();
    else
        return zero();
}

bool Reach::equal( SemElem* se ) const
{
    Reach* rhs = static_cast< Reach* >(se);
    return ( isreached == rhs->isreached );
}

std::ostream & Reach::print( std::ostream & o ) const
{
    return (isreached) ? o << "ONE" : o << "ZERO";
}

/*
 * $Log $
 */

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */


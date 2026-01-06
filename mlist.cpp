#include <stdlib.h>
#include <string.h>
#include "mlist.h"

// -------------------- constructores -------------------------//

MLIST::MLIST ()
{
    current = first = last = NULL;
    lastpos = 0;
    dimension = 0;
}

MLIST::~MLIST ( void )
{
    first = last = NULL;
    dimension = 0;
    lastpos = 0;
}

MLISTNODE::MLISTNODE ( void )
{
    next = NULL;
    previous = NULL;
    data = NULL;
}


//-------------------------------------------------------//
void MLIST::Release ()
{
    MLISTNODE* n, * b;

    n = first;
    while ( n )
    {
        b = n;
        n = n->next;
        delete b;
    }
    first = NULL;
    last = NULL;
    current = NULL;
    dimension = 0;
    lastpos = 0;
}


unsigned long MLIST::GetDimension ()
{
    return dimension;
}
/////////////////////////////////////////////////////////////////

MLISTNODE* MLIST::Insert ( MLISTDATA* d )
{
    MLISTNODE* n;

    n = new MLISTNODE;
    if ( n == NULL ) return NULL;

    n->data = d;
    InsertNode ( n );

    return n;
}

MLISTNODE* MLIST::InsertBefore ( MLISTDATA* d, MLISTNODE* pos )
{
    MLISTNODE* n;

    n = new MLISTNODE;
    if ( n == NULL ) return NULL;

    n->data = d;
    InsertNodeBefore ( n, pos );

    return n;
}

MLISTNODE* MLIST::InsertAfter ( MLISTDATA* d, MLISTNODE* pos )
{
    MLISTNODE* n;

    n = new MLISTNODE;
    if ( n == NULL ) return NULL;

    n->data = d;
    InsertNodeAfter ( n, pos );

    return n;
}

MLISTNODE* MLIST::GetNextNode ()
{
    if ( current->next )
    {
        current = current->next;
        return current;
    }
    else return NULL;
}

MLISTNODE* MLIST::GetPreviousNode ()
{
    if ( current->previous )
    {
        current = current->previous;
        return current;
    }
    else return NULL;
}


MLISTNODE* MLIST::GetNextNode ( MLISTNODE* n )
{
    return n->next;
}

MLISTNODE* MLIST::GetPreviousNode ( MLISTNODE* n )
{
    return n->previous;
}

MLISTNODE* MLIST::GetFirstNode ( void )
{
    return first;
}

MLISTNODE* MLIST::GetLastNode ( void )
{
    return last;
}

MLISTDATA MLIST::GetNext ( MLISTNODE* n )
{
    n = GetNextNode ( n );
    if ( n ) return n->data;
    else return NULL;
}

MLISTDATA MLIST::GetPrevious ( MLISTNODE* n )
{
    n = GetPreviousNode ( n );
    if ( n ) return n->data;
    else return NULL;
}

MLISTDATA MLIST::GetNext ()
{
    MLISTNODE* n;
    n = GetNextNode ();
    if ( n ) return n->data;
    else return NULL;
}

MLISTDATA MLIST::GetPrevious ()
{
    MLISTNODE* n;
    n = GetPreviousNode ();
    if ( n ) return n->data;
    else return NULL;
}

MLISTDATA MLIST::GetFirst ( void )
{
    MLISTNODE* n = GetFirstNode ();
    current = n;
    if ( n ) return n->data;
    else return NULL;
}

MLISTDATA MLIST::GetLast ( void )
{
    MLISTNODE* n = GetLastNode ();
    current = n;
    if ( n ) return n->data;
    else return NULL;
}

MLISTDATA MLIST::Get ( unsigned long index )
{
    MLISTNODE* n;

    n = GetNode ( index );
    return n->data;
}

MLISTNODE* MLIST::GetNode ( unsigned long index )
{
    MLISTNODE* n;

    n = first;
    while ( n && index )
    {
        index--;
        n = n->next;
    }
    current = n;
    return n;
}

MLISTNODE* MLIST::Find ( MLISTDATA data )
{
    MLISTNODE* n;

    n = first;
    while ( n )
    {
        if ( n->data == data ) return n;
        n = n->next;
    }
    return NULL;
}

MLISTDATA MLIST::Find ( MLISTNODE* fn )
{
    MLISTNODE* n;

    n = first;
    while ( n )
    {
        if ( n == fn ) return n->data;
        n = n->next;
    }
    return NULL;
}

void MLIST::InsertNode ( MLISTNODE* n )
{
    if ( first == NULL ) // es el 1er elemento
    {
        first = n;
        last = n;
        n->next = NULL;
        n->previous = NULL;
        current = n;
    }
    else
    {
        last->next = n;
        n->previous = last;
        n->next = NULL;
        last = n;
    }
    dimension++;
}

void MLIST::InsertNodeBefore ( MLISTNODE* n, MLISTNODE* pos )
{
    if ( first == NULL )
    {
        first = n;
        last = n;
        n->next = NULL;
        n->previous = NULL;
        current = n;
    }
    else
    {
        n->previous = pos->previous;
        if ( pos->previous != NULL ) pos->previous->next = n;
        else first = n;
        pos->previous = n;
        n->next = pos;
    }
    dimension++;
}

void MLIST::InsertNodeAfter ( MLISTNODE* n, MLISTNODE* pos )
{
    if ( first == NULL )
    {
        first = n;
        last = n;
        n->next = NULL;
        n->previous = NULL;
        current = n;
    }
    else
    {
        n->next = pos->next;
        if ( pos->next != NULL ) pos->next->previous = n;
        else last = n;
        pos->next = n;
        n->previous = pos;
    }
    dimension++;
}

void MLIST::DeleteNode ( MLISTNODE* n )
{
    if ( n == first )
    {
        if ( n->next == NULL ) first = last = NULL;
        else
        {
            first = n->next;
            first->previous = NULL;
            current = NULL;
        }
    }
    else if ( n == last )
    {
        last = n->previous;
        last->next = NULL;
    }
    else
    {
        n->previous->next = n->next;
        n->next->previous = n->previous;
    }
    dimension--;
    delete n;
}


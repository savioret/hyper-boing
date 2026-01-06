#ifndef MLIST_H_
#define MLIST_H_


typedef void* MLISTDATA;

class MLISTNODE
{
public:
    MLISTNODE* next, * previous;
    MLISTDATA data; // void*

    MLISTNODE ( void ); // constructor
    ~MLISTNODE ( void ) { delete data; }
};



/********************************************************
 clase MLIST

  clase para usar listas enlazadas genericas.
********************************************************/
class MLIST
{
    MLISTNODE* first, * last;
    MLISTNODE* current;
    unsigned long lastpos;
    unsigned long dimension;

public:
    MLIST ();
    ~MLIST ( void );

    MLISTNODE* Insert ( MLISTDATA* d );
    MLISTNODE* InsertBefore ( MLISTDATA* d, MLISTNODE* pos );
    MLISTNODE* InsertAfter ( MLISTDATA* d, MLISTNODE* pos );
    void Delete ( MLISTDATA* d );

    void InsertNode ( MLISTNODE* n );
    void InsertNodeBefore ( MLISTNODE* d, MLISTNODE* pos );
    void InsertNodeAfter ( MLISTNODE* d, MLISTNODE* pos );
    void DeleteNode ( MLISTNODE* n );

    MLISTNODE* GetNextNode ( MLISTNODE* n );
    MLISTNODE* GetPreviousNode ( MLISTNODE* n );
    MLISTNODE* GetFirstNode ( void );
    MLISTNODE* GetLastNode ( void );
    MLISTNODE* GetPreviousNode ();
    MLISTNODE* GetNextNode ();


    MLISTDATA GetNext ( MLISTNODE* n );
    MLISTDATA GetPrevious ( MLISTNODE* n );
    MLISTDATA GetFirst ( void );
    MLISTDATA GetLast ( void );
    MLISTDATA GetPrevious ();
    MLISTDATA GetNext ();

    MLISTDATA Get ( unsigned long index );
    MLISTNODE* GetNode ( unsigned long index );
    MLISTNODE* Find ( MLISTDATA data );
    MLISTDATA Find ( MLISTNODE* fn );

    void Release ();
    unsigned long GetDimension ();
};

#endif
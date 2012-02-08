#include "List.h"

#include "process.h"

// --------------------------------------------------------------------------------------
//
//    Data structures.
//
// --------------------------------------------------------------------------------------

struct NodeStruct_ {
    struct NodeStruct_ * prev;        // NULL for the first node in the list.
    struct NodeStruct_ * next;        // NULL for the last  node in the list.
    void               * dPtr;        // Address of a datum in the list; may not be NULL.
};

typedef struct NodeStruct_ Node, * NodePtr;

struct ListStruct_ {
    int                  length;            // Number of nodes in the list (ie if two nodes wrap the same data, they both count).
    struct NodeStruct_ * firstNode;         // Address of the first node in the list (the "front" of the list)
    struct NodeStruct_ * lastNode;          // Address of the last  node in the list (the "back"  of the list)
};

// -------------------------------------------------

typedef enum { LLI_FORWARD, LLI_REVERSE } LLI_Direction;

struct ListIteratorStruct_ {
    LLI_Direction        direction;
    struct ListStruct_ * list;
    struct NodeStruct_ * currentNode;
};

typedef struct ListIteratorStruct_ ListIterator;

// --------------------------------------------------------------------------------------
//
//   Prototypes for helpers (definitions located at the end of the file).
//
// --------------------------------------------------------------------------------------

static NodePtr ll_nodeAtIndexOfList( int idx, ListPointer theList );


// --------------------------------------------------------------------------------------
//
//    Constructors.
//
// --------------------------------------------------------------------------------------

ListPointer _newEmptyList() {
    
    ListPointer list = (ListPointer) malloc( sizeof(struct ListStruct_) );		 // TODO: not use malloc, use request_memory_block
    list->length         = 0;
    list->firstNode      = NULL;
    list->lastNode       = NULL;
    return list;
}

// --------------------------------------------------------------------------------------
//
//    Public routines.
//
// --------------------------------------------------------------------------------------

void _insertPtrAtBeginningOfList(void * thePtr, ListPointer theList )
{
// TODO: replace with assert
    ll_crashIfPtrIsNULL(fileName, lineNumber, thePtr,  "insertPtrAtBeginningOfList's element pointer");
    ll_crashIfPtrIsNULL(fileName, lineNumber, theList, "insertPtrAtBeginningOfList's list pointer");
   

    NodePtr newNode = (NodePtr) malloc( sizeof(struct NodeStruct_) );	  // replace with request_mem_blcock
    newNode->dPtr   = thePtr;
    if( theList->length == 0 ) {
        newNode->prev      = NULL;
        newNode->next      = NULL;
        theList->firstNode = newNode;
        theList->lastNode  = newNode;
    } else {
        newNode->prev            = NULL;
        newNode->next            = theList->firstNode;
        theList->firstNode->prev = newNode;
        theList->firstNode       = newNode;
    }
    theList->length++;
    return;
}

void _insertPtrIntoListAtIndex(void * ptr, ListPointer theList, int index ) {	 
// TODO replace with assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber, ptr,     "insertPtrIntoListAtIndex's element pointer"                                 );
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "insertPtrIntoListAtIndex's list parameter"                                  );

    if( index == 0 ) {
        return _insertPtrAtBeginningOfList( fileName, lineNumber, ptr, theList );
    } else if( index == theList->length ) {
        return _addPtrToEndOfList( fileName, lineNumber, ptr, theList );
    } else {
	//TODO: assert
        ll_crashIfIndexNotInRange( fileName, lineNumber, index, theList, "function insertPtrIntoListAtIndex" );
        // There's at least one node to both the left and right of where the new node goes.
        NodePtr newNode = (NodePtr) malloc( sizeof(struct NodeStruct_) );			   // TODO: req mem block
        NodePtr nodeToInsertBefore = ll_nodeAtIndexOfList( index, theList );
        NodePtr nodeToInsertAfter  = nodeToInsertBefore->prev;
        newNode->next              = nodeToInsertBefore;
        newNode->prev              = nodeToInsertAfter;
        newNode->dPtr              = ptr;
        nodeToInsertAfter->next    = newNode;
        nodeToInsertBefore->prev   = newNode;
        theList->length++;
        return;
    }
}

void _addPtrToEndOfList( void * thePtr, ListPointer theList )
{
//TODO assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber, thePtr,  "addPtrToEndOfList's element parameter"                               );
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "addPtrToEndOfList's list parameter"                                  );

//TODO req mem block
    NodePtr newNode = (NodePtr) malloc( sizeof(struct NodeStruct_) );
    newNode->dPtr   = thePtr;
    if( theList->length == 0 ) {
        newNode->prev      = NULL;
        newNode->next      = NULL;
        theList->firstNode = newNode;
        theList->lastNode  = newNode;
    } else {
        newNode->prev           = theList->lastNode;
        newNode->next           = NULL;
        theList->lastNode->next = newNode;
        theList->lastNode       = newNode;
    }
    theList->length++;
    return;
}

void *  _getPtrAtIndexInList(int idx, ListPointer theList )
{
// TODO assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber,      theList, "getPtrAtIndexInList's list parameter"                                  );
    ll_crashIfIndexNotInRange(       fileName, lineNumber, idx, theList, "function getPtrAtIndexInList"                                          );

    NodePtr node = ll_nodeAtIndexOfList( idx, theList );
    return node->dPtr;  
}

// frees node, returns contained data
void * _removeAndReturnPtrAtIndexFromList(int index, ListPointer theList )
{
	//TODO assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber,        theList, "removeAndReturnPtrAtIndexFromList's list parameter"                                  );
    ll_crashIfIndexNotInRange(       fileName, lineNumber, index, theList, "function removeAndReturnPtrAtIndexFromList"                                          );

    NodePtr node;
    if( index == 0 ) {
        // Deleting the first node on the list.
        node = theList->firstNode;
        if( theList->length == 1 ) {
            // We're left with an empty list. theList->length will be decremented just before returning.
            theList->firstNode = NULL;
            theList->lastNode  = NULL;
        } else {
            // There are at least two nodes, so node->next != NULL.
            theList->firstNode = node->next;
            node->next->prev   = NULL;
        }
    } else if( index == theList->length-1 ) {
        // Deleting the last node on the list.
        // At this point we know that index > 0, so lastNode->prev != NULL.
        node               = theList->lastNode;
        theList->lastNode  = node->prev;
        node->prev->next   = NULL;
    } else {
        // Deleting an interior node from a list containing at least 3 elements.
        node = ll_nodeAtIndexOfList( index, theList );
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    void * ptr = node->dPtr;
    free( node );  //TODO release mem block
    theList->length--;
    return ptr;
}

int _lengthOfList(ListPointer theList ) {
//TODO assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "lengthOfList"                                                   );
    
    return theList->length;
}

			 //TODO no bool
bool _isListEmpty( char * fileName, int lineNumber, ListPointer theList ) {
//TODO assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "isListEmpty"                                                   );
    
    return theList->length == 0;
}

void _makeListEmpty(  ListPointer theList, FreeDataFcn * fcn ) {
//todo
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "makeListEmpty"                                                   );
    
    NodePtr curr = theList->firstNode;
    NodePtr next = NULL;
    while( curr != NULL ) {
        next = curr->next;
        if( fcn != NULL ) fcn( curr->dPtr );  // Call user function to free heap memory for user's data (including the dPtr).
        free( curr );		 //todo USE RELEASE
        curr = next;
    }
    theList->length    = 0;
    theList->firstNode = NULL;
    theList->lastNode  = NULL;
    return;
}

void _destroyList( ListPointer theList, FreeDataFcn * fcn ) {
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "destroyList"                                                   );

    _makeListEmpty( theList, fcn );
    free( theList );   //todo
}

// --------------------------------------------------------------------------------------
//
//    Iterators.
//
// --------------------------------------------------------------------------------------

ListIteratorPtr _newForwardListIterator( ListPointer theList ) {
//TODO use our assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "newForwardListIterator's list parameter is NULL"                          );
    
    ListIteratorPtr newIterator = (ListIteratorPtr) malloc( sizeof(ListIterator) );			 // TODO use our malloc
    newIterator->cookie.intVal  = ll_Iter_cookieValue.intVal;
    newIterator->direction      = LLI_FORWARD;
    newIterator->list           = theList;
    newIterator->currentNode    = theList->firstNode;
    return newIterator;
}

ListIteratorPtr _newBackwardListIterator(  ListPointer theList ) {
//TODO use our assert
    ll_crashIfPtrIsNULL(             fileName, lineNumber, theList, "newBackwardListIterator's list parameter is NULL"                         );
    
    ListIteratorPtr newIterator = (ListIteratorPtr) malloc( sizeof(ListIterator) );		//TODO use our malloc
    newIterator->cookie.intVal  = ll_Iter_cookieValue.intVal;
    newIterator->direction      = LLI_REVERSE;
    newIterator->list           = theList;
    newIterator->currentNode    = theList->lastNode;
    return newIterator;
}

bool _listHasNextElement( ListIteratorPtr theIterator ) {
//TODO use our assert
    ll_crashIfPtrIsNULL(           fileName, lineNumber, theIterator, "listHasNextElement's iterator parameter is NULL" );
    
    return theIterator->currentNode != NULL;
}

void * _nextListElement( ListIteratorPtr theIterator ) {
    NodePtr nextNode;

	//TODO use our assert
    ll_crashIfPtrIsNULL(           fileName, lineNumber, theIterator, "nextListElement's iterator parameter is NULL" );
    
    nextNode = theIterator->currentNode;
    if(        theIterator->direction == LLI_FORWARD ) {
        if( theIterator->currentNode != NULL ) theIterator->currentNode = theIterator->currentNode->next;
        return nextNode->dPtr;
    } else if( theIterator->direction == LLI_REVERSE ) {
        if( theIterator->currentNode != NULL ) theIterator->currentNode = theIterator->currentNode->prev;
        return nextNode->dPtr;
    } else {
	//TODO use OS printing routines
        fprintf( stderr, "ERROR in nextListElement(...), called from line %d of file %s.\n", lineNumber, fileName     );
        fprintf( stderr, "The direction for this list is neither FORWARD or BACKWARD, which shouldn't ever happen!\n" );
        exit(1);
    }
}

void _destroyIterator( ListIteratorPtr theIterator ) {
// TODO use our assert
    ll_crashIfPtrIsNULL( fileName, lineNumber, theIterator, "destroyIterator's iterator parameter is NULL" );
    				  //TODO use our release
    free( theIterator );
    return;
}

// --------------------------------------------------------------------------------------
//
//    Helpers.
//
// --------------------------------------------------------------------------------------

//TODO cleanup

static
void ll_crashIfIndexNotInRange( char * fileName, int lineNumber, int idx, ListPointer theList, char * msg ) {
    if( (0 <= idx) && (idx < theList->length) ) {
        return;
    } else {
        fprintf( stderr, "ERROR: out-of-range index %d on line %d in file %s (%s). Aborting.\n", idx, lineNumber, fileName, msg );
        exit(1);
    }
}

static    // Accessible only from other routines in this file.
void ll_crashIfPtrIsNULL( char * fileName, int lineNumber, void * ptr, char * msg ) {
    if( ptr != NULL ) {
        return;
    } else {
        fprintf( stderr, "ERROR: attempt to insert a NULL pointer into the list on line %d of file %s (%s).\n", lineNumber, fileName, msg );
        exit(1);
    }
}

static
NodePtr ll_nodeAtIndexOfList( int idx, ListPointer theList ) {
    // Assumes theList != NULL and 0 <= idx < theList->length.
    // Should check to see which end is closer and count from that. Oh well...
    int ctr = 0;
    NodePtr cn = theList->firstNode;
    while( ctr < idx ) {
        ctr++;
        cn = cn->next;
    }
    return cn;
}

static   // Used to verify that the magic cookie with which initialized ListStructs are tagged is correct.
void ll_crashIfListLinkedCookieIsBad( char * fileName, int lineNumber, ListPointer ptr, char * msg ) {
    if( ptr->cookie.intVal != ll_CookieValue.intVal ) {
        fprintf( stderr, "ERROR on line %d of file %s: %s\n", lineNumber, fileName, msg );
        exit(2);
    }
}


#ifndef SE350_LIST
#define SE350_LIST

#define newEmptyList(implementation)                      _newEmptyList()       
#define insertPtrAtBeginningOfList(ptr,theList)           _insertPtrAtBeginningOfList(ptr, theList)         
#define insertPtrIntoListAtIndex(ptr,theList,idx)         _insertPtrIntoListAtIndex(ptr, theList, idx)         
#define addPtrToEndOfList(ptr,theList)                    _addPtrToEndOfList(ptr, theList)         
#define getPtrAtIndexInList(idx,theList)                  _getPtrAtIndexInList(idx, theList)         
#define removeAndReturnPtrAtIndexFromList(idx,theList)    _removeAndReturnPtrAtIndexFromList(idx, theList)         
        
#define isListEmpty(theList)                              _isListEmpty(theList)         
#define makeListEmpty(theList,fcn )                       _makeListEmpty(theList, fcn)               
#define destroyList(theList,fcn )                         _destroyList(theList, fcn)  
       
#define lengthOfList(theList)                             _lengthOfList(theList)         
                             
#define newForwardListIterator(theList)                   _newForwardListIterator(theList)         
#define newBackwardListIterator(theList)                  _newBackwardListIterator(theList)         
#define nextListElement(theIterator)                      _nextListElement(theIterator)         
#define listHasNextElement(theIterator)                   _listHasNextElement(theIterator)         
#define destroyIterator(theIterator)                      _destroyIterator(theIterator)

#endif    // SE350_LIST


// Arrange that the contents of this file only be compiled once,
// even if directly or indirectly #included more than once.
#ifndef SE350_LIST_MODULE
#define SE350_LIST_MODULE

// ListStruct_ will be *defined* in the implementation - ie in ListLinked.c.
// The compiler's happy with our use here of ListPointer because it knows how large a pointer is;
// it wouldn't be happy if we said something like sizeof(ListStruct) because it doesn't yet know 
// the sizeof a ListStruct. We define both ListPointer and ListPtr so we don't have to remember
// which is correct :-).
typedef struct ListStruct_ ListStruct, * ListPointer, * ListPtr;


// -----------------------------------------------------------------------------------------------------------------------------
//
//     Constructors
//
// -----------------------------------------------------------------------------------------------------------------------------

/*   Create and return a new, empty List.
** 
**       Pre:  none additional - implementation must be a ListImplementationName.
** 
**       Post: returns a new, empty list. If more than one implementation is included in the program
**             then the implementation requested must be one of those included; otherwise the value
**             of implementation must be either UseUseDefaultList or match the implementation available.
*/

ListPointer _newEmptyList(void);


// -----------------------------------------------------------------------------------------------------------------------------
//
//     Public routines.
//
// -----------------------------------------------------------------------------------------------------------------------------


// -----------------------------------------------------------------------------------------------------------------------------

/*   Insert an element at the beginning of the list, ahead of all present contents.
** 
**       Pre:  ptr != NULL; theList != NULL.
** 
**       Post: the existing contents of the list move right one index position,
**             ptr becomes the entry at index 0, and
**             the list's size is increased by 1.
*/

void _insertPtrAtBeginningOfList( void * ptr, ListPointer theList );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Insert an element at the specified index, moving the existing entry at that index and all subsequent entries right one position.
 ** 
 **       Pre:  ptr != NULL; theList != NULL; 0 <= idx <= theList->length.
 ** 
 **       Post: the existing contents of the list at indicese [idx,theList->length-1] move right one index position,
 **             ptr becomes the entry at index idx, and
 **             the list's size is increased by 1.
 **
 **   Note that idx is allowed to be as large as the initial size of the List, in which case calling this routine
 **   is equivalent to calling _addPtrToEndOfList(...).
 */

void _insertPtrIntoListAtIndex(void * ptr, ListPointer theList, int idx );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Append an element to the end of the list, behind all present contents.
** 
**       Pre:  ptr != NULL; theList != NULL.
** 
**       Post: the list's size is increased by 1, with
**             ptr as the last entry.
*/

void _addPtrToEndOfList( void * ptr, ListPointer theList );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Return the pointer curerntly at index idx in the list.
** 
**       Pre:  theList != NULL.
** 
**       Post: returns the pointer at index idx in the list.
**             The list itself is unchanged.
*/

void *  _getPtrAtIndexInList( int idx, ListPointer theList );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Return the pointer curerntly at index idx in the list.
** 
**       Pre:  theList != NULL; 0 <= idx < theList->length.
** 
**       Post: returns the pointer at index idx in the list, which is deleted from list.
**             Pointers at positions idx+1 through theList->length move left one index.
**             The size of the list decreases by one.
*/

void * _removeAndReturnPtrAtIndexFromList( int idx, ListPointer theList );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Reports whether theList is or is not empty.
** 
**      Pre:  theList != NULL.
** 
**      Post: Returns 1 or 0 depending on whether theList is or is not empty
*/

int _isListEmpty(ListPointer theList );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Removes all existing entries from the list.
**   If a user-defined function is supplied, it will be called once on each entry's data pointer.
** 
**       Pre:  theList != NULL.
** 
**       Post: All memory previously allocated by routines of the List implementation has been free'd,
**             except that required to represent an empty list. The user-supplied function has been called 
**             once for each entry with the entry's data pointer as argument.
*/

typedef void FreeDataFcn( void * arg );

void _makeListEmpty( ListPointer theList, FreeDataFcn * fcn );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Frees all the memory allocated by the List implementation.
**   If a user-defined function is supplied, it will call that once on each data pointer.
** 
**       Pre:  theList != NULL.
**             If fcn != NULL, it will be called once for each node with the node's data pointer as argument.
** 
**       Post: All memory previously allocated by routines of the List implementation has been free'd.
**             The user-supplied function has been called once for each node with the node's data pointer
**             as argument.
*/

void _destroyList( ListPointer theList, FreeDataFcn * fcn );

// -----------------------------------------------------------------------------------------------------------------------------

/*   Returns the size of the list.
 ** 
 **      theList != NULL.
 ** 
 **      Post: returns the number of elements in the list, some of which may wrap the same pointer w/o
 **            altering the reported size of the list.
 */

int _lengthOfList(  ListPointer theList );

                     

// -----------------------------------------------------------------------------------------------------------------------------
//
//     Iterators
//
// -----------------------------------------------------------------------------------------------------------------------------

// ListIteratorStruct is defined by the implementation.
typedef struct ListIteratorStruct_* ListIteratorPtr;

ListIteratorPtr _newForwardListIterator(ListPointer theList);
ListIteratorPtr _newBackwardListIterator(ListPointer theList);
void*           _nextListElement(ListIteratorPtr theIterator);
int             _listHasNextElement(ListIteratorPtr theIterator);
void            _destroyIterator(ListIteratorPtr theIterator);

#endif // of SE350_LIST_MODULE

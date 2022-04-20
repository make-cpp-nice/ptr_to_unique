# ptr_to_unique
A smart pointer to an object already owned by a unique_ptr. It doesn't own the object but it self zeroes when the object is deleted so that it can never dangle.

ptr_to_unique<T> is a smart pointer to an object already owned by a unique_ptr<T> that is guaranteed to read as null if the object has been deleted ensuring that it never dangles. std::unique_ptr is extended to support this by declaring it with a special custom deleter that will notify any ptr_to_uniques that reference it of deletions.

    std::unique_ptr<T, notify_ptrs<T>> //a unique_ptr enabled for use with ptr_to_unique

which can be written more conveniently as 

    notifying_unique_ptr<T> //exactly the same thing courtesy of 'using' declaration

ptr_to_unique can be initialised by a notifying_unique_ptr (the owner), another ptr_to_unique or nullptr.  

Like the unique_ptr it references, it is guaranteed to be valid or read as null.

____________________SPECIFICATION________________________________________________

Requires unique_ptr it is to reference to be declared with the notify_ptrs deleter

    std::unique_ptr<T, notify_ptrs<T>> //a unique_ptr enabled for use with ptr_to_unique

or more concisely 

    notifying_unique_ptr<T> //exactly the same thing courtesy of 'using' declaration

Default constructable:

    std::unique_ptr<T> puT;

Can be initialised on construction and assignment by:
  
	notifying_unique_ptr
  
	another ptr_to_unique
  
	nullptr

Operations
  
	convertible to bool
  
	dereference → and *
  
	equality and non-equality comparisons
  
	extract raw pointer
  
	dynamic_cast

Deleted operations
  
	Construction from a notifying_unique_ptr going out of scope

Free functions
  
	zero_ptrs_to
  
	make_notifying_unique

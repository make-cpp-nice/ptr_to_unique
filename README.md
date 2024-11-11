# ptr_to_unique - A smart pointer to an object already owned by a unique_ptr.

ptr_to_unique<T> is a smart pointer to an object already owned by a unique_ptr<T> that is guaranteed to read as null if the object has been deleted ensuring that it never dangles. std::unique_ptr is extended to support this by declaring it with a special custom deleter that will notify any ptr_to_uniques that reference it of deletions.

    std::unique_ptr<T, notify_ptrs<T>> //a unique_ptr enabled for use with ptr_to_unique

which can be written more conveniently as 

    notifying_unique_ptr<T> //exactly the same thing courtesy of 'using' declaration

ptr_to_unique can be initialised by a notifying_unique_ptr (the owner), another ptr_to_unique or nullptr.  

Like the unique_ptr it references, it is guaranteed to be valid or read as null.
	
________________________________________________________________________________


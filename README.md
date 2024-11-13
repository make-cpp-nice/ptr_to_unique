# ptr_to_unique - A smart pointer to an object already owned by a unique_ptr.

ptr_to_unique<T> is a non owning smart pointer to an object already owned by a unique_ptr<T> that is guaranteed to read as null if the object has been deleted, ensuring that it never dangles. This means that with ptr_to_unique<T>, the non-null test is always a reliable test of its validity.

It is intrusive on the owning unique_ptr declaration requiring required a specialised deletion hook to be inserted as a custom deleter and its use carries some overhead. However it should be considered a safety requirement wherever a secondary pointer may persist beyond the life of its pointee, particularly class members that persist from one event to another or from one function call to another.

Its use provides complete safety to a common idiom whose hazards are usually only partially mitigated and have resulted in many serious dangling pointer errors.
Also its simplicity of use and its universal guarantee of being valid or null opens up new design possibilities allowing much greater proliferation, storage and use of secondary pointers in the form of ptr_to_unique.
________________________________________________________________________________

It is implemented by single header file ptr_to_unique.h which defines two classes: 

ptr_to_unique<T> - the new non-owning smart pointer

notify_ptrs<T, D = default_delete<T>> - a deletion hook required for any unique_ptr that will be referenced by ptr_to_unique.

and a using declaration which conveniently encapsulates the declaration of a unique_ptr enabled for use with ptr_to_unique.

notifying_unique_ptr<T,D= default_delete<T>> 
= unique_ptr<T, notify_ptrs<T, D>>
________________________________________________________________________________



    


	
________________________________________________________________________________


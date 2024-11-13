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

Here is a trivial example of its use:

	xnr::ptr_to_unique<T> pT; //declare a ptr_to_unique – it will initialise to nullptr

	//Declare a  notifying_unique_ptr and make it the owner of a new object.
	notifying_unique_ptr<T> apT = std::make_unique<T>();

	pT =  apT; //point pT at apT

	//Declare another  ptr_to_unique and point it at pT
	xnr::ptr_to_unique<T> pT2 = pT;

	if(pT) //tests as valid
		pT->DoSomething(); //called
	if(pT2) //tests as valid
		pT2->DoSomething(); //called

	apT = nullptr; //delete the object using it owner

	if(pT) //tests as invalid
		pT->DoSomething(); //not called
	if(pT2) //tests as invalid
		pT2->DoSomething(); //not called

In practice it is likely that a ptr_to_unique may exist in a wider scope that the unique_ptr it references. This is where it is most needed and it will work just the same carrying the same guarantees. When the owning unique_ptr falls our of scope, it will delete the object and any ptr_to_uniques that reference it will read as nullptr.


    


	
________________________________________________________________________________


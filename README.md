# ptr_to_unique - a smart pointer to an object already owned by a unique_ptr.

ptr_to_unique&lt;T&gt; is a non owning smart pointer to an object already owned by a unique_ptr&lt;T&gt; that is guaranteed to read as null if the object has been deleted, ensuring that it never dangles. This means that with ptr_to_unique&lt;T&gt;, the non-null test is always a reliable test of its validity.

It is intrusive on the owning unique_ptr declaration requiring required a specialised deletion hook to be inserted as a custom deleter and its use carries some overhead. However it should be considered a safety requirement wherever a secondary pointer may persist beyond the life of its pointee, particularly class members that persist from one event to another or from one function call to another.

Its use provides complete safety to a common idiom whose hazards are usually only partially mitigated and have resulted in many serious dangling pointer errors.
Also its simplicity of use and its universal guarantee of being valid or null opens up new design possibilities allowing much greater proliferation, storage and use of secondary pointers in the form of ptr_to_unique.
________________________________________________________________________________

It is implemented by single header file ptr_to_unique.h which defines two classes: 

ptr_to_unique&lt;T&gt; - the new non-owning smart pointer

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
ptr_to_unique can be initialised on construction and assignment by:

nullptr 

a notifying_unique_ptr

or another ptr_to_unique

but it will not allow the following incorrect assignments to compile:

	ptr_to_unique<T> puT= some_raw_pointer;          //error, source not owned by a unique_ptr
	notifying_ unique_ptr<T> apT =  a_ptr_to_unique; //error, non-owner cannot initialise owner
	ptr_to_unique<T> puT= make_notifying_unique<T>();          //error, ptr_to_unique cannot take ownership

The clarification of owner and non-owner types generates an extra set of grammatical rules that the compiler enforces. This helps to keep code clear and coherent.

ptr_to_unique supports the following pointer emulating operations in the same way as any other smart pointer.

ptr->DoSomething();  //pointer dereference

T& t = *ptr;  //dereference as an object

if(ptr) //boolean non-null test
{
}

if(ptr == another_ptr) and if(ptr != another_ptr)
where another pointer may be a notifying_unique_ptr, a unique_ptr or a raw pointer

Pointer arithmetical comparisons (> and <) are not supported nor are any other pointer arithmetical operations (++, –, + etc.) .

The following dot methods are also supported.

T* p=ptr.get(); //returns the pointee as a raw pointer

ptr_to_unique&lt;U&gt; pU = ptr. dynamic_ptr_cast&lt;U&gt;();

Of course when you get hold of that raw pointer you can do mischief with it but you have to be realistic. Most functions take raw pointers because they can't anticipate what kind of smart pointer you are going to call them with. It can be abused but you are going to need it. 

Only a dynamic cast is permitted because it is run-time checked. Static cast would undermine the guarantee that there can be no incorrect initialisation of a ptr_to_unique even by mistake.

ptr_to_unique can be declared as a const and set to point at a valid object on initialisation but it will still self zero if that object is deleted. Otherwise, it behaves as const – you can never point it anywhere else:
________________________________________________________________________________
There are some considerations with transfer of ownership:

Ownership can be transferred between unique_ptr and notifying_ unique_ptr as long the types are compatible and they have the same deleter. In the case of notifying_ unique_pt that will be the deleter D passed in notify_ptrs, not notify_ptrs itself which is transparent for this evaluation.

If an object is passed from a notifying_unique_ptr  A to another notifying_unique_pt B
then all ptr_to_uniques that were referencing A will remain valid pointing at the same object that is now held by B.

However if an object is passed from a notifying_unique_ptr  A to a unique_pt C
then all ptr_to_uniques that were referencing A will be zeroed because the new owner, a unique_ptr, will not be able to keep them safe by notifying deletions.

If an object is passed from a unique_ptr C to a notifying_unique_ptr  A, there will be no ptr_to_uniques to worry about because the  unique_ptr C doesn't support them and can't accrue them.
________________________________________________________________________________


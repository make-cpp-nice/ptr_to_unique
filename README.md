# ptr_to_unique - UNDER CONSTRUCTION - testing formatting etc.
A smart pointer to an object already owned by a unique_ptr. It doesn't own the object but it self zeroes when the object is deleted so that it can never dangle.

ptr_to_unique<T> is a smart pointer to an object already owned by a unique_ptr<T> that is guaranteed to read as null if the object has been deleted ensuring that it never dangles. std::unique_ptr is extended to support this by declaring it with a special custom deleter that will notify any ptr_to_uniques that reference it of deletions.

    std::unique_ptr<T, notify_ptrs<T>> //a unique_ptr enabled for use with ptr_to_unique

which can be written more conveniently as 

    notifying_unique_ptr<T> //exactly the same thing courtesy of 'using' declaration

ptr_to_unique can be initialised by a notifying_unique_ptr (the owner), another ptr_to_unique or nullptr.  

Like the unique_ptr it references, it is guaranteed to be valid or read as null.
	
________________________________________________________________________________

SPECIFICATION  -  #include "ptr_to_unique.h"  -  namespace xnr::

Requires unique_ptr it is to reference to be declared with the notify_ptrs deleter provided

    std::unique_ptr<T, xnr::notify_ptrs<T>> //a unique_ptr enabled for use with ptr_to_unique

or more concisely 

    xnr::notifying_unique_ptr<T> //exactly the same thing courtesy of 'using' declaration

Default constructable:

    xnr::ptr_to_unique<T> puT;

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
	
More specs and info to follow

ptr_to_unique<T>, extending std::unique_ptr to support weak secondary smart pointers
john morrison leon





5.00/5 (8 votes)
15 Dec 2021
CPOL
27 min read
 15K    200  
A smart pointer to an object already owned by a unique_ptr. It doesn't own the object but it self zeroes when the object is deleted so that it can never dangle.
ptr_to_unique is a smart pointer to an object already owned by a unique_ptr that is guaranteed to read as null if the object has been deleted ensuring that it never dangles. std::unique_ptr is extended to support this by exploiting its provision for custom deleters to allow secondary smart pointers to be informed of deletions. The implementation uses a hidden reference counted control block similar to that of shared_ptr/weak_ptr but more lightweight and tailored to the requirements of single ownership.
Download ptr_to_unique.zip - 3.3 KB
Download VS 2019 demo project - 50.3 KB
Download Windows demo executable - 62.6 KB
Contents
Introduction
Background
Design
notifying_unique_ptr
ptr_to_unique
make_notifying_unique function
Using Your Own Custom Deleter
Safety and Performance
Runtime Error Handling
Using the Code
Top Dog Demo App
The Implementation Code
Summary
Why doesn't unique_ptr already have a smart weak companion like shared_ptr?
History
Introduction
The Standard Library provides three smart pointers, one for single ownership and two for shared ownership:

ownership	 	owning	 	 	 	non-owning
single	 	unique_ptr<T>	 	 	 	nothing here
shared	 	shared_ptr<T>	 	<--	 	weak_ptr<T>
I think there should be four. There is clearly one missing.

Why is there no non owning smart pointer to point at something already owned by unique_ptr with the smart property that it self zeroes if its pointee is deleted?

I can't answer that question and that troubles me, but I can say two things:

It is needed and the lack of it has probably for many years been a major cause of project wrecking dangling pointers and the reputational damage they have done to the language.
It can be done and I present a working implementation here.
Background
It is generallly recommended that any further references to an object owned by unique_ptr should be held as a raw pointer because it has the required property that it doesn't own the object. That is true, but being just a raw pointer, it has the inconvenience that it continues to point at where the object used to be after it has been deleted – it is left dangling. The intuition that the pointer might self-zero when its pointee is deleted is wrong. For that to happen, it needs to be smart.

Consider the following drawing code which has a simple optimisation to avoid having to call the lengthy FindObjectOfInterest function (which returns a reference to a unique_ptr) each time the screen is updated.

C++
object* g_pObjectOfInterest = NULL;
std::vector<unique_ptr<object>>  collection;

void DrawObjectOfInterest()
{
    if(NULL==g_pObjectOfInterest)
         g_pObjectOfInterest = FindObjectOfInterest(collection).get();
     g_pObjectOfInterest->Draw()
}
On the surface, it looks well designed but what happens if the object gets deleted? Well, the null test won't catch it because g_pObjectOfInterest still points at where the object used to be so Draw() will be called on a dangling pointer.

Don't think that this danger has stopped programmers from writing code like this. It solves a problem and it is a problem that must be solved. Therefore it is done. If the danger is recognised, then some attempt will have been made to mitigate it, but this is a place where a lot of mistakes get made. As there is no test, you can do to see if a non-zero raw pointer is still valid, mitigation is about writing code to reset pointers and making sure that it gets called when objects get deleted. With no particular strategy recommended for this, these mitigations tend to be ad hoc spaghetti code and even if comprehensive can be very fragile.

I am sure I am not the only programmer working in a single ownership context (most code) who has looked longingly at shared_ptr with its non-owning weak_ptr partner. It is tempting to impose shared ownership on even fundamentally single ownership designs just to have that non-owning weak_ptr which 'knows' when its pointee has been deleted. Like this:

C++
weak_ptr<object> g_wpObjectOfInterest = NULL;
std::vector<shared_ptr<object>>  collection;

void DrawObjectOfInterest()
{
    shared_ptr<object> spT(g_wpObjectOfInterest);
    if(NULL==spT)
    {
         spT = FindObjectOfInterest(collection).get();
         g_wpObjectOfInterest =  spT;
    }
    spT->Draw();
}
weak_ptr has the added inconvenience of having to convert it to a shared_ptr every time you want to do anything with it but the main problem is you no longer have unique ownership and that can matter a lot. In this case, it could mean that an object is removed from the collection but lives on because a shared_ptr somewhere still references it. Being no longer in the collection, it would no longer appear in any enumerations (effectively invisible) but some things might still reference it and may still act on it. That is the kind of mess that can be difficult to detect and sort out.

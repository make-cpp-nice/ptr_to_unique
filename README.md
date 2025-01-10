# ptr_to_unique - a smart pointer to an object already owned by a unique_ptr.

## Introduction
```ptr_to_unique<T>;``` is a non owning smart pointer to an object already owned by a ```unique_ptr<T>``` that is guaranteed to read as null if the object has been deleted, ensuring that it never dangles. This means that with ```ptr_to_unique<T>```, the non-null test is always a reliable test of its validity.

It is intrusive on the owning ```unique_ptr``` declaration requiring a specialised deletion hook to be inserted as a custom deleter and its use carries some overhead. However it should be considered a safety requirement wherever a secondary pointer may persist beyond the life of its pointee, particularly class members that persist from one event to another or from one function call to another.

Its use provides complete safety to a common idiom whose hazards are usually only partially mitigated and have resulted in many serious dangling pointer errors.
Also its simplicity of use and its universal guarantee of being valid or null opens up new design possibilities allowing much greater proliferation, storage and use of secondary pointers in the form of ```ptr_to_unique```.

It is complete and ready to download and use. Just download the **ptr_to_unique.h** file and include it.

Please post any feedback or comments on [Comment and discussion](https://github.com/make-cpp-nice/ptr_to_unique/discussions/1)

________________________________________________________________________________
## Classes
It is implemented by single header file **ptr_to_unique.h** which defines two classes: 

+ **```ptr_to_unique<T>```** the  new non-owning smart pointer

+ *```notify_ptrs<T, D = default_delete<T>>```* - a deletion hook required for any unique_ptr that will be referenced by ```ptr_to_unique```.

and a using declaration which conveniently encapsulates the declaration of a ```unique_ptr``` enabled for use with ptr_to_unique.

+ **```notifying_unique_ptr<T,D= default_delete<T>> = unique_ptr<T, notify_ptrs<T, D>>```**
________________________________________________________________________________
## Example
Here is a trivial example of its use:

```C++	
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
```
In practice it is likely that a ```ptr_to_unique``` may exist in a wider scope that the ```unique_ptr``` it references. This is where it is most needed and it will work just the same carrying the same guarantees. When the owning ```notifying_unique_ptr``` falls our of scope, it will delete the object and any ```ptr_to_uniques``` that reference it will read as nullptr.
________________________________________________________________________________
## ptr_to_unique
```ptr_to_unique``` can be initialised on construction and assignment by:

+ ```nullptr```  - setting a ptr_to_unique to null does not delete the object it was referencing.

+ a ```notifying_unique_ptr```

+ or another ```ptr_to_unique```

but it will not allow the following incorrect assignments to compile:
```C++
ptr_to_unique<T> puT= some_raw_pointer;          //error, source not owned by a unique_ptr
notifying_ unique_ptr<T> apT =  a_ptr_to_unique; //error, non-owner cannot initialise owner
ptr_to_unique<T> puT= make_unique<T>(); //error, ptr_to_unique cannot take ownership
```
The clarification of owner and non-owner types generates an extra set of grammatical rules that the compiler enforces. This helps to keep code clear and coherent.

```ptr_to_unique``` supports the following pointer emulating operations in the same way as any other smart pointer.

+ ```ptr->DoSomething();  //pointer dereference```

+ ```T& t = *ptr;  //dereference as an object```

+ ```if(ptr) //boolean non-null test```

+ ```if(ptr == another_ptr) and if(ptr != another_ptr) ```
where another pointer may be a ```notifying_unique_ptr```, a ```unique_ptr``` or a raw pointer

Pointer arithmetical comparisons (> and <) are not supported nor are any other pointer arithmetical operations (++, –, + etc.) .

The following dot methods are also supported.

+ ```T* p=ptr.get(); //returns the pointee as a raw pointer```

+ ```ptr_to_unique<U> pU = ptr. dynamic_ptr_cast<U>();```

Of course when you get hold of that raw pointer you can do mischief with it but you have to be realistic. Most functions take raw pointers because they can't anticipate what kind of smart pointer you are going to call them with. It can be abused but you are going to need it. 

Only a dynamic cast is permitted because it is run-time checked. Static cast would undermine the guarantee that there can be no incorrect initialisation of a ```ptr_to_unique``` even by mistake.

```ptr_to_unique``` can be declared as a ```const``` and set to point at a valid object on initialisation but it will still self zero if that object is deleted. Otherwise, it behaves as ```const``` – you can never point it anywhere else:
________________________________________________________________________________
## Transfer of ownership
There are some considerations with transfer of ownership:

Ownership can be transferred between ```unique_pt```r and ```notifying_ unique_ptr``` as long the types are compatible and they have the same deleter. In the case of ```notifying_ unique_ptr``` that will be the deleter ```D``` passed in ```notify_ptrs```, not ```notify_ptrs``` itself which is transparent for this evaluation.

If an object is passed from a ```notifying_unique_ptr```  A to another ```notifying_unique_ptr``` B
then all ```ptr_to_uniques``` that were referencing A will remain valid pointing at the same object that is now held by B.

However if an object is passed from a ```notifying_unique_ptr```  A to a ```unique_ptr``` C
then all ```ptr_to_unique```s that were referencing A will be zeroed because the new owner, a ```unique_ptr```, will not be able to keep them safe by notifying deletions.

If an object is passed from a ```unique_ptr``` C to a ```notifying_unique_ptr```  A, there will be no ```ptr_to_unique```s to worry about because the ```unique_ptr``` C doesn't support them and can't accrue them.
_______________________________________________________________________________
# Pointing inside an owned object
This is about allowing ```ptr_to_unique``` to be initialised from a ```notifying_unique_ptr``` or another ```ptr_to_unique``` but instead of pointing at the owned object, to point at some item inside of it. This is equally safe and can be more convenient than holding a reference to the larger object and having to dereference down to the desired item each time it is used. It can be critical for keeping modules decoupled.

This can be done using a variadic 'aliasing' constructor (terminology borrowed from shared_ptr)

ptr_to_unique<TargetType>(ptr, inwards_offsets...)

or more conveniently using the point_into free function which will automatically deduce the target type.
```C++
auto point_to(ptr, inwards_offsets...)
```
where 

ptr is the ```notifying_unique_ptr``` or ```ptr_to_unique``` from which it is being initialised

inwards_offsets... is a variadic sequence of inwards offsets that specify the target item to be referenced. Each offset may be:
a class member offset – ```&class_name::member_name``` – (aka pointer to member data)  to enable access to class data members. 
or an integer offset – ```index``` - to enable access to elements of fixed arrays

	This enables any and only items that reside within the larger owned object to be reached.

```point_to``` returns a ```ptr_to_unique``` to the target item. Its type will be that of the target item.
	
As this paradigm of inwards offsets is not part of the mainstream vernacular, an example may help to make it clearer. 

Here is the class structure for an imaginary machine and its components;
```C++
struct Grommet
{
	//Grommet stuff
};
struct Widget
{
	Grommet topGrommet;
	Grommet bottomGrommet;
};
struct Machine
{
	Widget upWidget;
	Widget downWidget;
	Widget stepWidgets[6];
};
```
and we also have two utilities, one monitors the behaviour and performance of a ```Widget``` and the other a ```Grommet```. 

The ```Widget``` monitor only knows about Widgets and holds a pointer to the ```Widget``` it is currently monitoring 
```C++
xnr::ptr_to_unique<Widget> p_curr_widget;
```
the  ```Grommet``` monitor only knows about Grommets. and holds a pointer to the ```Grommet``` it is currently monitoring 
```C++
xnr::ptr_to_unique<Grommet> p_curr_grommet;
```
and we have a Machine owned by a ```notifying_unique_ptr```
```C++
::notifying_unique_ptr<Machine> apMachine = std::make_unique<Machine>();
```
The following are examples of how  ```p_curr_widget``` and  ```p_curr_grommet``` can be initialised  to point at Grommets and Widgets within the ```Machine``` owned by ```apMachine```: 
```C++
curr_widget = xnr::point_to(apMachine, &Machine::upWidget);
curr_widget = xnr::point_to(apMachine, &Machine::stepWidgets, 3);

curr_grommet = xnr::point_to(apMachine, &Machine::upWidget, &Widget::topGrommet);
curr_grommet = xnr::point_to(apMachine, &Machine::stepWidgets, 3, &Widget::bottomGrommet);
```
Index offsets are bounds checked at run-time but if you express them as ```std::integral_constant``` then they will be bounds checked during compilation:
```C++
curr_widget = xnr::point_to(apMachine, &Machine::stepWidgets, std::integral_constant<int, 3>());
```
That is a bit of a handful to write and read but if you download and include literal_integral_constants.h from ….... then the same thing can be written:
```C++
curr_widget = xnr::point_to(apMachine, &Machine::stepWidgets, 3_);//compile time bounds check
```
The ability to point safely inside an owned object greatly increases the range of uses to which ptr_to_unique can be applied. It means you can point it at anything as long as you also have access to the notifying_unique_ptr that owns it. This is potentially powerful and the somewhat painstaking approach involved here is to ensure that it can only be applied correctly. 

_______________________________________________________________________________
## zero_prts_to(notifying_unique_ptr) free function
There is a free function that can be applied to a ```notifying_unique_ptr``` to zero any ```unique_ptr```s that reference it.

```auto& zero_ptrs_to(notifying_unique_ptr<T, D>& ptr) ```

It returns the same ```notifying_unique_ptr``` it was passed so it can be conveniently applied during transfer of ownership where it is most likely needed. e.g. 
```C++
ap2 = std::move(xnr::zero_ptrs_to(ap1));
```
It might be used as a way of making sure that you take your hands off an object (have no further means of referencing it) before passing its owership to another thread.
________________________________________________________________________________
## Custom deleters
If you have your own custom deleter that you want to use, that is not a problem. Simply pass it as the second template parameter to  ```notifying_unique_ptr```,  as you would with a ```unique_ptr```:
```C++
notifying_unique_ptr<T, my_deleter<T>> ptr;
```
There is an issue,with a simple remedy if you want to retrieve your deleter using ```ptr.get_deleter()``` because, by default, it will return the ```notify_ptrs``` deleter. So the following code would not compile: 
```C++
ptr.get_deleter().my_deleter_data = data; //error  notify_ptrs doesn't have a  my_deleter_data member
```
However it will give you your deleter if you explicitly type for it  e.g.
```C++
my_deleter deleter = ptr.get_deleter(); //will use implicit conversion to return your deleter
deleter.my_deleter_data = data;  //ok compiles
```
______________________________________________________________________________

## Thread safety

```ptr_to_unique``` cannot be made thread safe in the same way as ```std::weak_ptr``` because with single ownership there is no way to prevent another thread from deleting the object while you are working with it. You keep it thread safe by never allowing a ```ptr_to_unique``` to point at something whose ownership is controlled by another thread. To ensure this:

+ Don't pass a ```ptr_to_unique``` to another thread to hold and use.

+ Zero all  ```ptr_to_unique```s that reference an object (```zero_ptrs_to```) before passing its ownership to another thread.

The compiler can't enforce this, so you have to. But it isn't hard, you will know when you are mucking around between threads.

You can pass an ownership of an object to another thread to be worked on as long as you have no further contact with that object until the other thread notifies that its processing is done and hands it back. This is a common and safe idiom with single ownership.

If you want access to an object owned by another thread then you must use ```shared_ptr/weak_ptr```. Even then that only solve the pointer validity issue, you will still have to take action to prevent read/write thread clashes on mutable data. As I said, you will know when you are mucking around between threads.
______________________________________________________________________________

## Safety and Performance
Inevitably, there is some trade off between safety and performance and ```ptr_to_unique``` prioritises safety. It will not dangle. It is designed to be above all a reliable infrastructure node that can safely be used in a fairly casual manner allowing you to design and build with it confidently.

Like ```shared_ptr```, there is an overhead when a ```ptr_to_unique``` is initialised, including either allocating a control block or bumping a reference count. Unlike ```shared_ptr``` and ```unique_ptr```, there is also an overhead on every dereference as it checks the control block for validity first.

Of course, if you have written your code correctly, you will have already checked its validity - making all those checks on dereference redundant. The problem is that only you know this and are you absolutely sure? Sometimes, you can be. 

The dereference overhead is only a few instructions and will be negligible unless you are working ```ptr_to_unique``` very hard in a very atomic manner., e.g., using it to dereference class member variables individually and doing very little with them. In these cases, if you are sure you have checked the validity first and that it will stay valid while you work on it, you can simply extract the raw pointer into local scope (having checked its validity as a ```ptr_to_unique```) and work with that instead.

```C++
ptr_to_unique<T> puT;

//some code that may or may not initialise puT

if(puT)
{
         T* pT = puT.get(); // pT does no checking and has no protection against dangling

         //some code that works pT very hard
         //be sure not to call anything that might delete the object
}
```
I am sure that bare metal purists will do this habitually and it is what will happen on most function calls. That is fine as long as it is accompanied by the required due diligence to ensure it is being done safely. Tightly scoping the lifetime of the extracted raw pointer, as happens in a function call, helps greatly with this.

For the most part, it is simpler and safer to work directly with the ```ptr_to_unique```.

```C++
if(puT)
    puT->DoSomthing();
```

______________________________________________________________________________
## Runtime error handling

```ptr_to_unique``` will never give you a dangling pointer to run with but it can give you a null pointer. If you write code that deferences a null pointer then you have a runtime error - one that the compiler cannot know about and catch. Execution cannot proceed as intended and therefore you will want it to stop and flag what has happened. C++ provides exceptions for this, even allowing you to try/catch them and recover execution in a wider scope. 

The scheme for ptr_to_unique is:

+ calling the → operator on a null pointer.
It returns the null pointer which will immediately provoke a null dereference exception in the user code, exactly where the bad dereference was made. This is the same as with ```unique_ptr```. 

+ calling the * operator on a null pointer.
It throws an immediate exception so you don't get as far as symbolising a null object. Stepping back through the call stack will take you directly to where the bad dereference was made This differs from  ```unique_ptr``` which simply returns a null object and relies on an exception being thrown the first time you try to do anything with the null object you have succeeded in symbolising - less convenient but better than breaking the zero overhead of ```unique_ptr``` by impeding a dereference with a non zero test.
```ptr_to_unique``` has to do a test on every dereference anyway so there is no further cost in throwing an exception when an error condition is clearly occurring.

They are designed to bring your attention as quickly as possible to the location of the error in your code.

______________________________________________________________________________
## The Implementation Code
```ptr_to_unique```, like other smart pointers is intended to be placed in a library and usable almost as a language element. That means it must work correctly in any context and this requires a very high level of correctness in how it is coded. I invite anyone to examine ptr_to_unique.h to check that this has been achieved or otherwise. I have tried to present the code so that this is a reasonable task.

What follows is not a complete description. I hope the code largely speaks for itself, but I think it is helpful to give an overview of the architecture and to explain some of the not so obvious design decisions.

![image](https://github.com/user-attachments/assets/68d5a30f-35b2-492b-8688-15868f0b40bf)

Each ```ptr_to_unique``` carries its own local pointer to the object but that pointer is only accessed once the valid count has been checked in the Control Block. If the valid count is zero, then the local pointer is ignored and the ```ptr_to_unique``` reads as null. When the object is deleted, the ```notify_ptrs``` deleter accesses the Control Block and sets the valid count to zero so that all ```ptr_to_unique```s referencing it will now read as null.
The Control Block lives as long as anything is referencing it. The ptr_to_unique reference count keeps track of this. When ```ptr_to_unique``` reference count is zero and the valid count is zero, the Control Block will delete itself.

There is nothing revolutionary about the design. The only thing that is novel is deciding to do it in this context.

#### Division of Labour
```ptr_to_unique``` and the ```notify_ptrs``` deleter are typed (templated) by the object they point at. So any code that doesn't get optimised away could potentially be instantiated multiple times. However, the Control Block and the pointers that reference it are not templated on the object type and therefore code using them should be instantiated once only. To ensure that the compiler sees this, the labour is divided between that which is templated on the object type (blue lines on the diagram) and that which is not (red lines on the diagram).

This is done by creating a wrapper for the pointer to the control block which encapsulates all operations involving the ControlBlock including testing if it exists and allocating it when required. This is called ```_ptr_to_unique_cbx``` (not templated) and its methods are the only access to the control block which is declared privately within it.

Any part of ```ptr_to_unique``` operation that does not depend on the object type is delegated to ```_ptr_to_unique_cbx```.

#### The notify_ptrs deleter
The ```notify_ptrs``` deleter is a key component and also the most innovative. Its design is not obvious and requires some explanation.

The ```notify_ptrs``` deleter must implement ```operator()(T* p)``` , which ```unique_ptr``` will call to do the deletion, and use this to achieve its key functionality which is tentatively represented here as pseudo code.

```C++
void operator()(T* p)
{
    Mark Control Block invalid
    Let passed in deleter do the deleting
}
```
At this stage, I have left it as pseudo code because how ```notify_ptrs``` should carry the passed in deleter ```D``` and the pointer to the control block is inconveniently conditioned by the requirements of transfer of ownership.

```notify_ptrs``` should not impede transfer of ownership between ```notifying_unique_ptr``` and plain ```unique_ptr```. Also in the case of transfer from ```notifying_unique_ptr``` to plain ```unique_ptr```, it must zero ```any ptr_to_unique``` referencing the owned object. This must be done because otherwise those ```ptr_to_unique```s would be left with no notifications of deletion and could be left dangling.

We can get a smooth transfer from ```notifying_unique_ptr``` to plain ```unique_ptr``` simply by deriving ```notify_ptrs``` from the passed in deleter ```D```. This will also give us Empty Base Class Optimisation and ```D```is typically empty as in ```std::default_delete```

```C++
struct notify_ptrs : public D
{
```
But this makes the transfer too smooth. It is so language elemental to take the base class of what is offered that there is nowhere to put code to intercept it and zero those ```ptr_to_unique```s.

So instead ```notify_ptrs``` is not derived from ```D``` and a conversion operator is provided as the only path to achieve the transfer. And it is in this that we can put code to zero those ```ptr_to_unique```s.

```C++
operator D& ()
{
    Mark Control Block invalid
    return passed in deleter D
}
```
The problem now is that if we hold the passed in deleter ```D``` as a member then, even if empty, it will occupy storage just to have a distinct address from the pointer to the control block.

```C++
struct notify_ptrs
{
    D deleter;
    _ptr_to_unique_cbx cbx;
```
We have lost the Empty Base Class Optimisation that would have come from deriving from ```D``` rather than containing it. We want that Empty Base Class Optimisation back and we can have it with the following contrivance which defines how the passed in deleter ```D``` and the pointer to the control block are held.

```C++
struct notify_ptrs // if we derive from D, operator D& () will never be called
{
private:
    //D will typically be dataless so we still need empty base class optimisation
    struct InnerDeleter: public D
    {
        mutable _internal::_ptr_to_unique_cbx cbx;
    }; 
    InnerDeleter inner_deleter; // is a D

    //provide a function to access the _ptr_to_unique_cbx cbx

    inline _ptr_to_unique_cbx& get_cbx() const
    {
        return inner_deleter.cbx;
    }
```
If we consider the case where ```D``` is empty as it is with ```std::default_delete```;

struct ```InnerDeleter``` has just one data member, ```cbx```, so its size is the size of ```cbx```. There is no need for a separate address for ```D``` because ```InnerDeleter``` is a ```D```.

We then give the ```notify_ptrs``` deleter just one data member, an ```InnerDeleter```. This means ```notify_ptrs``` is the same size as ```inner_deleter``` which is the same size as ```cbx```. We now have that Empty Base Class Optimisation for ```D```.

The pointer to the control block is accessed using ```get_cbx()``` and inner_deleter is a ```D```. So now, we can replace pseudo code with real code.

The deletion operator.

```C++
//The functor call to do the deletion
void operator()(T* p)
{
    //zero ref_ptrs that reference this object
    get_cbx().mark_invalid();
    //leave deletion to passed in deleter
    inner_deleter(p);
}
```
The conversion operator that allows and intercepts transfer from notifying_unique_ptr to unique_ptr

```C++
//Permits and intercepts move from notifying_unique_ptr to unique_ptr
operator D& ()
{   ///plain unique_ptr doesn't support ptr_to_unique
    //so any that reference this object must be zeroed
    get_cbx().mark_invalid();
    return inner_deleter; //return the passed in deleter
}
```
We also need a conversion constructor to allow transfer from a plain unique_ptr to a notifying_unique_ptr:

```C++
//permit move from unique_ptr to notifying_unique_ptr
template< class D2, 
class = std::enable_if_t<std::is_convertible<D2, D>::value>>
inline notify_ptrs(const D2& deleter) 
{    
}
```
#### ptr_to_unique
Like most smart pointers, the majority of the code consists of carefully composed conversion constructors and assignments. This is where most of the hard work is and determines its grammar of use. To keep the public interface clear, much of the work is delegated to commonly called private methods. In particular:

+ ```point_to(ptr)``` which is called by most constructors and assignments. If you look at its two versions, one taking ```ptr_to_unique``` and the other taking ```notifying_unique_ptr```, you will see how the work is apportioned between the templated ```ptr_to_unique``` itself and the non-templated ```_ptr_to_unique_cbx cbx```:

+ ```accept_move(ptr) ```which is called when the operand is falling out of scope. Knowing that the operand is going to die, there is no need to bump the reference count on the Control Block (slightly quicker).

+ ```checked_pointer() ```checks the control block before returning the raw pointer.

Some private aliasing constructors are also defined. They are currently not called and are there to support extended functionality which will be published in the near future.

In constructions and assignments, versions containing ```const&&``` arguments are defined to catch occasions when the source is falling out of scope.

Initialisation from a ```ptr_to_unique``` falling out of scope means move semantics can be used which can be more optimal.

Initialisation from a ```notifying_unique_ptr``` falling out of scope is prohibited so that you cannot write:

```C++
ptr_to_unique<T> puT = std::move(a_notifying_unique_ptr); //error - cannot take ownership
```
or:

```C++
ptr_to_unique<T> puT = std::make_unique<T>(); //error - cannot take ownership
```
______________________________________________________________________________
## Summary
The addition of ```ptr_to_unique``` 'upgrades' single ownership to match the pointer safety resources that have long been available for shared ownership. Single ownership is important and remains the correct and natural model for many designs. It deserves proper smart pointer coverage and ```ptr_to_unique``` completes this by acting as a safe refuge for its secondary references.

Please post any feedback or comments on [Comment and discussion](https://github.com/make-cpp-nice/ptr_to_unique/discussions/1)

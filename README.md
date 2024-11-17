# ptr_to_unique - a smart pointer to an object already owned by a unique_ptr.
## Introduction
```ptr_to_unique<T>;``` is a non owning smart pointer to an object already owned by a ```unique_ptr<T>``` that is guaranteed to read as null if the object has been deleted, ensuring that it never dangles. This means that with ```ptr_to_unique<T>```, the non-null test is always a reliable test of its validity.

It is intrusive on the owning ```unique_ptr``` declaration requiring a specialised deletion hook to be inserted as a custom deleter and its use carries some overhead. However it should be considered a safety requirement wherever a secondary pointer may persist beyond the life of its pointee, particularly class members that persist from one event to another or from one function call to another.

Its use provides complete safety to a common idiom whose hazards are usually only partially mitigated and have resulted in many serious dangling pointer errors.
Also its simplicity of use and its universal guarantee of being valid or null opens up new design possibilities allowing much greater proliferation, storage and use of secondary pointers in the form of ```ptr_to_unique```.

It is complete and ready to download and use. Just download the ptr_to_unique.h file.
________________________________________________________________________________
## Classes
It is implemented by single header **file ptr_to_unique.h** which defines two classes: 

**```ptr_to_unique<T>```** the  new non-owning smart pointer

*```notify_ptrs<T, D = default_delete<T>>```* - a deletion hook required for any unique_ptr that will be referenced by ```ptr_to_unique```.

and a using declaration which conveniently encapsulates the declaration of a ```unique_ptr``` enabled for use with ptr_to_unique.

**```notifying_unique_ptr<T,D= default_delete<T>> = unique_ptr<T, notify_ptrs<T, D>>```**
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
ptr_to_unique<T> puT= make_notifying_unique<T>(); //error, ptr_to_unique cannot take ownership
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

If an object is passed from a ```unique_ptr``` C to a ```notifying_unique_ptr```  A, there will be no ptr_to_uniques to worry about because the ```unique_ptr``` C doesn't support them and can't accrue them.
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

# ptr_to_unique
A smart pointer to an object already owned by a unique_ptr. It doesn't own the object but it self zeroes when the object is deleted so that it can never dangle.

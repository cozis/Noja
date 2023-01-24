This folder implements the object model. In the context of this language, 
an object is a virtual class that implements a given set of methods. This 
folder exports functions that transform "raw" data types into objects, 
functions that do the inverse transformation and functions that trigger the 
virtual methods. This folder also contains the implementation of the heap 
and the garbage collector that needs to be tightly coupled with the object 
model.

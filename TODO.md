# TODO List for OcMesh
Everything... or, more in details...

## Octree data structure
### Morton encoding of voxel coordinates.
Since we use 64bits integers we can't encode full 32bit coordinates. Dividing 
evenly in three parts yields to 21bits coordinates, but we also need space to 
encode the level of the cubie in the tree, and the "colour" of the cubie, 
which we call "material" in this application. 
As opposed to what is usually needed, it's not sufficient to us to simply know 
if a cubie is "inside" or "outside" an object, but we need to take into 
account different materials, which can be in the order of the thousands, 
depending on the simulation data. So we need more bits and for performance 
reasons we need to store them together with the cubie's location code. 

Therefore the location code of a cubie is encoded as follows:
* We use 13 bits for each coordinate, so we have 39 bits for xyz 
  coordinate of the cubie, interleaved as usual for the Morton encoding.
  With 13 bits coordinate we have a space of (2^13)^3 = 5*10^11 locations to 
  model the simulation world, which is more than enough, but we'll end up with 
  a tree of maxiumum height of 13 levels.
* So we use other 4 bits to encode the height of the cubie in the tree.
* The remaining 21 bits are free to use. In this case, they're enough to 
  express more than two millions materials, which are more than enough.

It's important to store the Morton code on the highest bits, followed by the 
tree level, followed by the material code. In this way the Morton order follows
from the natural order of the integers.

### Octree subdivision and intersection testing with the CSG
We have to determine which CSG object contains every cubie. The user building 
the CSG have to guarantee that the objects made of different materials do not
overlap in space, so we have a simplifying assumption, i.e. every cubie will 
be either:
* Completely outside of any object, so we can throw it away
* Completely inside of a *single* object, so we can stop subdividing, and 
  assign to it the material of the object that contains it.
* In the middle of two or more objects, in which case we subdivide the cubie
  and recursively proceed.

It can't simply happen that a cubie falls _completely_ inside more than one 
object. This would mean that the user has specified an overlapped set of CSG 
objects, and we can just signal the error.


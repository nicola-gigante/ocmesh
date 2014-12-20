# TODO List for OcMesh
Everything... more or less...

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

### Morton encoding with a pre-computed table 
The interleaving can be achieved with a constant number of
magic bitwise operations. However, an even fastr solution is
based on the pre-computation of a table of bitmasks for 8 bit
values, that are composed to interleave our 16bits words.

Note that this is faster than computing it on the fly
only if we suppose that a lot of morton codes will be calculated in
morton order, as to efficiently use the cache lines for the table data.
This is the case in our code.

Thanks to the C++11 constexpr keyword, we could tell the compiler
to precompute the table and statically put the results directly
as data into the executable.

However, C++11 constexpr functions must be written in purely functional
style, so the implementation would be weird and convoluted, and 
the maintenance burden is not worth the trouble. 

Instead, when we'll be able to compile in C++14 mode, the constexpr
feature would be surely a win, thanks to the relaxed rules of the new
standard. Below is a sketch of the C++14 code that would be required to
use a pre-computed table.

    constexpr uint32_t interleave(uint8_t byte)
    {
        uint32_t x = byte;

        x = (x | x << 16) & 0xFF0000FF;
        x = (x | x <<  8) & 0x0F00F00F;
        x = (x | x <<  4) & 0xC30C30C3;
        x = (x | x <<  2) & 0x49249249;

        return x;
    }

    constexpr auto make_table(coordinate_t coord)
    {
        std::array<uint32_t, 256> table;

        for(uint8_t i = 0; i <= 255; ++i)
            table[i] = interleave(i) << uint8_t(coord);

        return table;
    }

### Managing the tree
The linear octree is an array of ordered location codes. Managing the tree
imply the implementation of functions to recognize neighbors of a cubie from
its location code.

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


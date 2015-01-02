# TODO #
This is an incomplete list of things still to be implemented to make ocmesh usable.

## Features ##
Here the missing features
* Octree balancing
* Hexahedral mesh generation in the correct format
* Portable build-system (probably CMake)

Optionals/to be discussed:
* Better command line options parsing
* Better format for description of CSG scenes.
  Maybe it's worth to simply provide a python binding to the library
* Support for specifying the precision in both percentage (as is now)
  and absolute values, and allow it to be specified from the command line
  and/or from the CSG file.
* Support for units of measure in the CSG file, including radians/degrees
  for the rotate function

## Performance ##
Of course the first thing to do is to measure real cases and profile.
A few possible performance improvements seem already plausible.

### Algorithmic improvements ###
* Investigate the possibility of parallelizing the subdivision.
* Implement the parallel version of the balancing procedure
* Implement optimized neighborhood finding (by starting the binary search
  from the common ancestor instead of the root).

### Code performance improvements ###
* Separate the `voxel` class used to represent voxels in the compact octree
  from the class used to manipulate single voxels offline (for example during
  the intersection testing), to avoid repeating a lot of bitwise operations.
* Measure if it's worth to cache the distance results during the intersection
  testing

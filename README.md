# therionsurface2survex
Convert therion surface meshes to survex

`Usage: [-h] [-o outfile] [-i infile] -- [infile]`

`  -o outfile`    File to write to. Will be derived from infile if not specified.  

`  -i infile`     File to read from, if not given by last parameter. 

`  -h`            Print this help and exit.
 
Example: `./therionsurface2survex examples/surface.th`
results in *surface.th.swx* being generated.


The main intend of this progam is to generate survex surface meshes that will
be processed by survex cavern program. This generates a 3d-file that can be
merged by therion. This way you can use your existing therion digital elevation
model (DEM) and include it in therions 3d-export; for example to get the distance
from the cave passage to the sourface.

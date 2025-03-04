OpenGL Grass renderer, implementation of [Responsive Real-Time Grass Rendering for General 3D Scenes](https://www.cg.tuwien.ac.at/research/publications/2017/JAHRMANN-2017-RRTG/JAHRMANN-2017-RRTG-draft.pdf).
Renders millions of blades of grass individually, and simulates environmental forces on each blade in real-time, using tessellation and compute shaders. 

![](https://github.com/daustria/GrassRenderer/blob/master/resources/grass_gif.gif)

What follows is a brief summary of how the grass is rendered and processed.

### Blade specification  
Each blade of grass is first described as three three-dimensional vertices $v0, v1, v2$ along with some additional information. $v0$ can be thought of
as the root of the blade of grass, while $v1$ and $v2$ are control points above $v0$. $v2$ can be thought of as the 'tip' of the blade of grass,
and $v1$ is positioned somewhere according to $v2$. The points $v1,v2$ are later used to align the unit quad $[0,1]^2$ to a Bezier curve,
by way of [De Casteljau's algorithm](https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm). More detail on this later when we discuss tessellation.

[comment]: # (TODO add a picture for v0, v1, v2 from the paper)

In addition to these vertices, we will also attach information relating to the orientation of the blade of grass (for example, which direction the grass faces, and which direction is up) 
as well as how it is affected by physical forces. We can store all the required information in four 4-dimensional vertices, as shown in `blade.h`.

### Grass physics
 
For each blade of grass, we modify $v1, v2$ in an analytic way that simulates physical forces on the blade. This is done in the compute shader `forces.compute`. 

TODO: finish this summary

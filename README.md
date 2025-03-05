Individual grass rendering in openGL, based on [Responsive Real-Time Grass Rendering for General 3D Scenes](https://www.cg.tuwien.ac.at/research/publications/2017/JAHRMANN-2017-RRTG/JAHRMANN-2017-RRTG-draft.pdf). Renders millions of blades of grass, and simulates environmental forces on each blade in real-time, using tessellation and compute shaders. 

![](https://github.com/daustria/GrassRenderer/blob/master/resources/grass_gif.gif)

What follows is a brief summary of how the grass is rendered and processed.

### Blade specification  
Each blade of grass is first described as three three-dimensional vertices $v0, v1, v2$ along with some additional information. $v0$ can be thought of
as the root of the blade of grass, while $v1$ and $v2$ are control points 'above' $v0$. $v2$ can be thought of as the 'tip' of the blade of grass,
and $v1$ is positioned somewhere according to $v2$. The points $v1,v2$ are later used to align the unit quad $[0,1]^2$ to a Bezier curve,
by way of [De Casteljau's algorithm](https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm). More detail on this later when we discuss tessellation.

In addition to these vertices, we will also attach information relating to the orientation of the blade of grass (for example, which direction the grass faces, and which direction is up) 
as well as how it is affected by physical forces. We can store all the required information in four 4-dimensional vertices, as shown in `blade.h`.

### Grass physics
 
For each blade of grass, we modify $v1, v2$ in an analytic way that simulates physical forces on the blade. This is done in the compute shader `forces.compute`. 
Each force is represented as a three dimensional vector, and we sum them to obtain the total force.

The force applied to the blade on the previous frame is then saved as a 4-dimensional vector in a 2D image. We take this into account when 
computing the force for the new frame.
 
There are some other technical details for how we compute the grass, which can be found in the paper, but 
long story short we add the total force to $v_2$ and then apply some post processing for correcting things like position and grass length.
I can probably make `forces.compute` a lot simpler considering that it does not actually implement too much from the paper. 

#### Gravity

The gravity $g \in \mathbb{R}^3$ is simply computed as some scale factor of $g=|g_E|f$, where $g_E = (0, -9.8, 0)$ and $f$ is a 
three-dimensional unit vector perpendicular to the vector that runs along the width of the blade. 

#### Wind

We compute a real value called the 'wind strength' as $1.2 \sin(t) + 0.2 \cos(t)$ or a variation of this formula. We vary the formula
slightly and 'randomly' for each blade of grass by using its ID assigned to it in the compute shader. We then take this 'strength' value and
multiply it against a three-dimensional unit vector representing the direction of the wind.

#### Stiffness

We compute the stiffness of a blade as $(I_{v_2} - {v_2})$, where $v_2$ is the 'tip' of the blade of grass after previous forces applied to it, and $I_{v_2}$ 
is the initial position of $v_2$ if no forces were applied (that is, $v_2$ points straight up from $v_0$). Indeed our compute shader takes into account the previous iterations.

### Shading and Tessellation

After `forces.compute` runs, we run the vertex and tessellation control shader on each blade. Until the tessellation evaluation shader, the vertices $v0,v1,v2$ are pretty much 'along for the ride' as not much is done to them besides applying the model matrix. In the tessellation control shader, we also specify how much to subdivide a unit quad $[0,1]^2$, based on how far the blade of grass is from the camera.

The tessellation evaluation shader `blade.tes` will run for each new vertex $(u,v)$ on this quad. We use $u,v$ as parameters, as well as the control points $v0, v1, v2$ for invoking
[De Casteljau's algorithm](https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm) so that **each such point $(u,v)$ on the quad is mapped to a point on a degree 2 Bezier curve in three dimensional space**. The precise computations are in `blade.tes` or the research paper. We also output the normal and UV coordinates and pass all this to the fragment shader for lighting.

### Installation

To setup OpenGL on my device I followed learnopengl.com. One should be able to run this code with the provided Makefile after configuring glfw and glad, as well as changing some paths in the Makefile to suit your system. It of course runs on my device but I haven't tested it on another device yet.  

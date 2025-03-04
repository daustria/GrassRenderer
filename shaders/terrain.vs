#version 430 core

// vertex position
layout(location = 5) in vec3 aPos;
// texture coordinate
layout(location = 6) in vec2 aTex;

out vec2 TexCoord;

void main()
{
    // convert XYZ vertex to XYZW homogeneous coordinate
    gl_Position = vec4(aPos, 1.0);
    // pass texture coordinate though
    TexCoord = aTex;
}

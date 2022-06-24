#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoords;

void main()
{
    TexCoords = aPos.xy * 0.5 + 0.5;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  
#version 300 core

precision lowp float;

out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    // RGBA -> RGBA
//    FragColor = texture(ourTexture, vec2(TexCoord.x, 1.0 - TexCoord.y));
    
    // BGRA -> RGBA
    FragColor.rgba = texture(ourTexture, vec2(TexCoord.x, 1.0 - TexCoord.y)).bgra;
}

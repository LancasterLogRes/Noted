varying float texCoord0;

void main(void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	texCoord0 = (1.0 - gl_Position.y) / 6.0;
}

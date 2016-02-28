#version 150
void main(){
	const vec4 positions[4]=vec4[4](
				vec4(-1,1,0,1),
				vec4(-1,-1,0,1),
				vec4(1,1,0,1),
				vec4(1,-1,0,1));
	gl_Position=positions[gl_VertexID];
}

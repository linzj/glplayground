#version 150 core
uniform sampler2DMS tex;
uniform sampler2DMS depth_tex;
out vec4 color;

void sort(in float depth[8],out int[8] sorted_index);
//vec4 texelFetch(sampler2DMS tex,ivec2 pos,int lod);
void main() {
	const vec4 black=vec4(0);
	ivec2 frag_coord=ivec2(gl_FragCoord.xy);
	float depth[8];
	depth[0]=texelFetch(depth_tex, frag_coord,0).r;
	if(depth[0]==1.0){
	  color=vec4(1);
	  return;
	}
//	color=texelFetch(tex,frag_coord,7);
//	if(color.xyz==black.xyz)
//		color=vec4(1);
//	return;
	depth[1]=texelFetch(depth_tex, frag_coord,1).r;
	depth[2]=texelFetch(depth_tex, frag_coord,2).r;
	depth[3]=texelFetch(depth_tex, frag_coord,3).r;
	depth[4]=texelFetch(depth_tex, frag_coord,4).r;
	depth[5]=texelFetch(depth_tex, frag_coord,5).r;
	depth[6]=texelFetch(depth_tex, frag_coord,6).r;
	depth[7]=texelFetch(depth_tex, frag_coord,7).r;
	int sorted_index[8];
	sort(depth,sorted_index);
	
	color=texelFetch(tex,frag_coord ,sorted_index[7]);

	color.a=0.4;
	
	
	for(int i=6;i>=0;--i){
		if(color==black)
			break;
	
		vec4 tmp_color=texelFetch(tex,frag_coord,sorted_index[i]);
		color=vec4(mix(color.rgb,tmp_color.rgb,color.a),color.a);
	}
}

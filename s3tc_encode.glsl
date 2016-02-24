#version 430
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba8) uniform readonly image2D u_Texture;
layout(rg32ui) uniform writeonly uimage2D u_OutputImage;

// find minimum and maximum colors based on bounding box in color space
void FindMinMaxColorsBox(in vec3 block[16], out vec3 mincol, out vec3 maxcol)
{
    mincol = block[0];
    maxcol = block[0];
    
    for (int i = 1; i < 16; i++) {
        mincol = min(mincol, block[i]);
        maxcol = max(maxcol, block[i]);
    }
}

void SelectDiagonal(in vec3 block[16], inout vec3 mincol, inout vec3 maxcol)
{
    vec3 center = (mincol + maxcol) * 0.5;

    vec2 cov = vec2(0.0);
    for (int i = 0; i < 16; i++)
    {
        vec3 t = block[i] - center;
        cov.x += t.x * t.z;
        cov.y += t.y * t.z;
    }

    if (cov.x < 0) {
        float temp = maxcol.x;
        maxcol.x = mincol.x;
        mincol.x = temp;
    }
    if (cov.y < 0) {
        float temp = maxcol.y;
        maxcol.y = mincol.y;
        mincol.y = temp;
    }
}

void InsetBBox(inout vec3 mincol, inout vec3 maxcol)
{
    vec3 inset = (maxcol - mincol) / 16.0 - (8.0 / 255.0) / 16;
    mincol = clamp(mincol + inset, vec3(0.0), vec3(1.0));
    maxcol = clamp(maxcol - inset, vec3(0.0), vec3(1.0));
}

vec3 RoundAndExpand(vec3 v, out uint w)
{
    ivec3 c = ivec3(round(v * vec3(31, 63, 31)));
    w = (c.r << 11) | (c.g << 5) | c.b;

    c.rb = (c.rb << 3) | (c.rb >> 2);
    c.g = (c.g << 2) | (c.g >> 4);

    return vec3(c) * (1.0 / 255.0);
}

uint EmitEndPointsDXT1(inout vec3 mincol, inout vec3 maxcol)
{
    uvec2 myoutput;
    maxcol = RoundAndExpand(maxcol, myoutput.x);
    mincol = RoundAndExpand(mincol, myoutput.y);

    // We have to do this in case we select an alternate diagonal.
    if (myoutput.x < myoutput.y)
    {
        vec3 tmp = mincol;
        mincol = maxcol;
        maxcol = tmp;
        return myoutput.y | (myoutput.x << 16);
    }

    return myoutput.x | (myoutput.y << 16);
}

uint EmitIndicesDXT1(in vec3 block[16], in vec3 mincol, vec3 maxcol)
{
    const float RGB_RANGE = 3;

    vec3 dir = (maxcol - mincol);
    vec3 origin = maxcol + dir / (2.0 * RGB_RANGE);
    dir /= dot(dir, dir);

    // Compute indices
    uint indices = 0;
    for (int i = 0; i < 16; i++)
    {
        uint index = uint(clamp((dot(origin - block[i], dir)), 0.0, 1.0) * RGB_RANGE);
        indices |= index << (i * 2);
    }

    uint i0 = (indices & 0x55555555);
    uint i1 = (indices & 0xAAAAAAAA) >> 1;
    indices = ((i0 ^ i1) << 1) | i1;

    // Output indices
    return indices;
}

void main()
{
  // x, y is the position on the image being encoded.
  int x = int(gl_GlobalInvocationID.x);
  int y = int(gl_GlobalInvocationID.y);
  vec3 rgbmap[16];

  // fill the rgb map
  int i, j;
  for (i = 0; i < 4; ++i) {
      for (j = 0; j < 4; ++j) {
          rgbmap[i * 4 + j] = imageLoad(u_Texture, ivec2(x * 4 + j, y * 4 + i)).rgb;
      }
  }
  // find min and max colors
  vec3 mincol, maxcol;
  FindMinMaxColorsBox(rgbmap, mincol, maxcol);

  SelectDiagonal(rgbmap, mincol, maxcol);

  InsetBBox(mincol, maxcol);

  uint ir = EmitEndPointsDXT1(mincol, maxcol);
  uint ig = EmitIndicesDXT1(rgbmap, mincol, maxcol);
  imageStore(u_OutputImage, ivec2(x, y), uvec4(ir, ig, 0, 1));
}

#version 430
layout(local_size_x = 4, local_size_y = 4) in;
layout(r8ui) uniform readonly uimage2D u_Texture;
layout(rgba8) uniform writeonly image2D u_Output;

const int kLookup[8] = int[8]( 0, 1, 2, 3, -4, -3, -2, -1 );
const int kModifierTable[32] = int[32](
/* 0 */2, 8, -2, -8,
/* 1 */5, 17, -5, -17,
/* 2 */9, 29, -9, -29,
/* 3 */13, 42, -13, -42,
/* 4 */18, 60, -18, -60,
/* 5 */24, 80, -24, -80,
/* 6 */33, 106, -33, -106,
/* 7 */47, 183, -47, -183);

int convert4To8(int b) {
    int c = b & 0xf;
    return (c << 4) | c;
}

int convert5To8(int b) {
    int c = b & 0x1f;
    return (c << 3) | (c >> 2);
}

int convertDiff(int base, int diff) {
    return convert5To8((0x1f & base) + kLookup[0x7 & diff]);
}

void main() {
  int imageHeight = imageSize(u_Output).y;
  int x = int(gl_GlobalInvocationID.x);
  int y = int(gl_GlobalInvocationID.y);
  uint xblockoffset = gl_LocalInvocationID.x;
  uint yblockoffset = gl_LocalInvocationID.y;
  ivec2 blockposition = ivec2((x * 2) & (~7), y / 4);
  int r1, r2, g1, g2, b1, b2;

  uint e1 = imageLoad(u_Texture, blockposition).r;
  uint e2 = imageLoad(u_Texture, blockposition + ivec2(1, 0)).r;
  uint e3 = imageLoad(u_Texture, blockposition + ivec2(2, 0)).r;
  uint e4 = imageLoad(u_Texture, blockposition + ivec2(3, 0)).r;
  uint e5 = imageLoad(u_Texture, blockposition + ivec2(4, 0)).r;
  uint e6 = imageLoad(u_Texture, blockposition + ivec2(5, 0)).r;
  uint e7 = imageLoad(u_Texture, blockposition + ivec2(6, 0)).r;
  uint e8 = int(imageLoad(u_Texture, blockposition + ivec2(7, 0)).r);
  uint high = uint((e1 << 24U) | (e2 << 16U) | (e3 << 8U) | e4);
  uint low = uint((e5 << 24U) | (e6 << 16U) | (e7 << 8U) | e8);
  if ((high & 2U) != 0U) {
      // differential
      int rBase = int(high >> 27U);
      int gBase = int(high >> 19U);
      int bBase = int(high >> 11U);
      r1 = convert5To8(rBase);
      r2 = convertDiff(rBase, int(high >> 24U));
      g1 = convert5To8(gBase);
      g2 = convertDiff(gBase, int(high >> 16U));
      b1 = convert5To8(bBase);
      b2 = convertDiff(bBase, int(high >> 8U));
  } else {
      // not differential
      r1 = convert4To8(int(high >> 28U));
      r2 = convert4To8(int(high >> 24U));
      g1 = convert4To8(int(high >> 20U));
      g2 = convert4To8(int(high >> 16U));
      b1 = convert4To8(int(high >> 12U));
      b2 = convert4To8(int(high >> 8U));
  }
    int tableIndexA = 7 & int(high >> 5U);
    int tableIndexB = 7 & int(high >> 2U);
    bool flipped = (high & 1U) != 0U;
    bool second = false;
    if (!flipped) {
        second = xblockoffset >= 2;
    } else {
        second = yblockoffset >= 2;
    }
    int k = int(yblockoffset + (xblockoffset * 4));
    int offset = int(((low >> uint(k)) & 1U) | ((low >> uint(k + 15)) & 2U));
    int r, g, b;
    if (!second) {
        int delta = kModifierTable[tableIndexA * 4 + offset];
        r = int(r1 + delta);
        g = int(g1 + delta);
        b = int(b1 + delta);
    } else {
        int delta = kModifierTable[tableIndexB * 4 + offset];
        r = int(r2 + delta);
        g = int(g2 + delta);
        b = int(b2 + delta);
    }
    imageStore(u_Output, ivec2(x, imageHeight - y), vec4(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0, 255));
}

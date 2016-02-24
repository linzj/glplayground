#version 140
in vec2 v_Position;
uniform sampler2D u_Texture;
uniform float u_width;
uniform float u_height;
out vec4 fragColor;

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
  float x = ((v_Position.x + 1.0) * u_width) / 2.0;
  float y = u_height - ((v_Position.y + 1.0) * u_height) / 2.0;
  int xblockoffset = int(x) & 3;
  int yblockoffset = int(y) & 3;
  ivec2 blockposition = ivec2(int(x * 2) & (~7), int(y / 4));
  int r1, r2, g1, g2, b1, b2;

  int e1 = int(texelFetch(u_Texture, blockposition, 0).r * 255.0);
  int e2 = int(texelFetch(u_Texture, blockposition + ivec2(1, 0), 0).r * 255.0);
  int e3 = int(texelFetch(u_Texture, blockposition + ivec2(2, 0), 0).r * 255.0);
  int e4 = int(texelFetch(u_Texture, blockposition + ivec2(3, 0), 0).r * 255.0);
  int e5 = int(texelFetch(u_Texture, blockposition + ivec2(4, 0), 0).r * 255.0);
  int e6 = int(texelFetch(u_Texture, blockposition + ivec2(5, 0), 0).r * 255.0);
  int e7 = int(texelFetch(u_Texture, blockposition + ivec2(6, 0), 0).r * 255.0);
  int e8 = int(texelFetch(u_Texture, blockposition + ivec2(7, 0), 0).r * 255.0);
  uint high = uint((e1 << 24) | (e2 << 16) | (e3 << 8) | e4);
  uint low = uint((e5 << 24) | (e6 << 16) | (e7 << 8) | e8);
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
    float r, g, b;
    if (!second) {
        int delta = kModifierTable[tableIndexA * 4 + offset];
        r = float(r1 + delta);
        g = float(g1 + delta);
        b = float(b1 + delta);
    } else {
        int delta = kModifierTable[tableIndexB * 4 + offset];
        r = float(r2 + delta);
        g = float(g2 + delta);
        b = float(b2 + delta);
    }
    r /= 255.0;
    g /= 255.0;
    b /= 255.0;
    fragColor = vec4(r, g, b, 1.0);
}

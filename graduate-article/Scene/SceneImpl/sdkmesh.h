#ifndef _SDKMESH_
#define _SDKMESH_

//--------------------------------------------------------------------------------------
// Hard Defines for the various structures
//--------------------------------------------------------------------------------------
#define SDKMESH_FILE_VERSION 101
#define MAX_VERTEX_ELEMENTS 32
#define MAX_VERTEX_STREAMS 16
#define MAX_FRAME_NAME 100
#define MAX_MESH_NAME 100
#define MAX_SUBSET_NAME 100
#define MAX_MATERIAL_NAME 100
#define MAX_PATH 256
#define MAX_TEXTURE_NAME MAX_PATH
#define MAX_MATERIAL_PATH MAX_PATH
#define INVALID_FRAME ((UINT)-1)
#define INVALID_MESH ((UINT)-1)
#define INVALID_MATERIAL ((UINT)-1)
#define INVALID_SUBSET ((UINT)-1)
#define INVALID_ANIMATION_DATA ((UINT)-1)
#define ERROR_RESOURCE_VALUE 1
#define INVALID_SAMPLER_SLOT ((UINT)-1)

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
#ifdef _WIN32
typedef unsigned long long UINT64;
#else
typedef unsigned long long UINT64;
#endif

typedef struct
{
  float x, y, z;
} D3DXVECTOR3;

typedef struct
{
  float x, y, z;
} D3DXVECTOR4;

typedef struct
{
  D3DXVECTOR4 rows[4];
} D3DXMATRIX;

enum SDKMESH_PRIMITIVE_TYPE
{
  PT_TRIANGLE_LIST = 0,
  PT_TRIANGLE_STRIP,
  PT_LINE_LIST,
  PT_LINE_STRIP,
  PT_POINT_LIST,
  PT_TRIANGLE_LIST_ADJ,
  PT_TRIANGLE_STRIP_ADJ,
  PT_LINE_LIST_ADJ,
  PT_LINE_STRIP_ADJ,
};

enum SDKMESH_INDEX_TYPE
{
  IT_16BIT = 0,
  IT_32BIT,
};

enum FRAME_TRANSFORM_TYPE
{
  FTT_RELATIVE = 0,
  FTT_ABSOLUTE, // This is not currently used but is here to support absolute
                // transformations in the future
};
typedef enum D3DDECLUSAGE {
  D3DDECLUSAGE_POSITION = 0,
  D3DDECLUSAGE_BLENDWEIGHT = 1,
  D3DDECLUSAGE_BLENDINDICES = 2,
  D3DDECLUSAGE_NORMAL = 3,
  D3DDECLUSAGE_PSIZE = 4,
  D3DDECLUSAGE_TEXCOORD = 5,
  D3DDECLUSAGE_TANGENT = 6,
  D3DDECLUSAGE_BINORMAL = 7,
  D3DDECLUSAGE_TESSFACTOR = 8,
  D3DDECLUSAGE_POSITIONT = 9,
  D3DDECLUSAGE_COLOR = 10,
  D3DDECLUSAGE_FOG = 11,
  D3DDECLUSAGE_DEPTH = 12,
  D3DDECLUSAGE_SAMPLE = 13,
} D3DDECLUSAGE,
  *LPD3DDECLUSAGE;

//--------------------------------------------------------------------------------------
// Structures.  Unions with pointers are forced to 64bit.
//--------------------------------------------------------------------------------------
typedef struct D3DVERTEXELEMENT9
{
  WORD Stream;
  WORD Offset;
  BYTE Type;
  BYTE Method;
  BYTE Usage;
  BYTE UsageIndex;
} D3DVERTEXELEMENT9, *LPD3DVERTEXELEMENT9;

struct SDKMESH_HEADER
{
  // Basic Info and sizes
  UINT Version;
  BYTE IsBigEndian;
  UINT64 HeaderSize;
  UINT64 NonBufferDataSize;
  UINT64 BufferDataSize;

  // Stats
  UINT NumVertexBuffers;
  UINT NumIndexBuffers;
  UINT NumMeshes;
  UINT NumTotalSubsets;
  UINT NumFrames;
  UINT NumMaterials;

  // Offsets to Data
  UINT64 VertexStreamHeadersOffset;
  UINT64 IndexStreamHeadersOffset;
  UINT64 MeshDataOffset;
  UINT64 SubsetDataOffset;
  UINT64 FrameDataOffset;
  UINT64 MaterialDataOffset;
};

struct SDKMESH_VERTEX_BUFFER_HEADER
{
  UINT64 NumVertices;
  UINT64 SizeBytes;
  UINT64 StrideBytes;
  D3DVERTEXELEMENT9 Decl[MAX_VERTEX_ELEMENTS];
  union
  {
    UINT64 DataOffset; //(This also forces the union to 64bits)
  };
};

struct SDKMESH_INDEX_BUFFER_HEADER
{
  UINT64 NumIndices;
  UINT64 SizeBytes;
  UINT IndexType;
  union
  {
    UINT64 DataOffset; //(This also forces the union to 64bits)
  };
};

struct SDKMESH_MESH
{
  char Name[MAX_MESH_NAME];
  BYTE NumVertexBuffers;
  UINT VertexBuffers[MAX_VERTEX_STREAMS];
  UINT IndexBuffer;
  UINT NumSubsets;
  UINT NumFrameInfluences; // aka bones

  D3DXVECTOR3 BoundingBoxCenter;
  D3DXVECTOR3 BoundingBoxExtents;

  union
  {
    UINT64 SubsetOffset; // Offset to list of subsets (This also forces the
                         // union to 64bits)
    UINT* pSubsets; // Pointer to list of subsets
  };
  union
  {
    UINT64 FrameInfluenceOffset; // Offset to list of frame influences (This
                                 // also forces the union to 64bits)
    UINT* pFrameInfluences; // Pointer to list of frame influences
  };
};

struct SDKMESH_SUBSET
{
  char Name[MAX_SUBSET_NAME];
  UINT MaterialID;
  UINT PrimitiveType;
  UINT64 IndexStart;
  UINT64 IndexCount;
  UINT64 VertexStart;
  UINT64 VertexCount;
};

struct SDKMESH_FRAME
{
  char Name[MAX_FRAME_NAME];
  UINT Mesh;
  UINT ParentFrame;
  UINT ChildFrame;
  UINT SiblingFrame;
  D3DXMATRIX Matrix;
  UINT AnimationDataIndex; // Used to index which set of keyframes transforms
                           // this frame
};

struct SDKMESH_MATERIAL
{
  char Name[MAX_MATERIAL_NAME];

  // Use MaterialInstancePath
  char MaterialInstancePath[MAX_MATERIAL_PATH];

  // Or fall back to d3d8-type materials
  char DiffuseTexture[MAX_TEXTURE_NAME];
  char NormalTexture[MAX_TEXTURE_NAME];
  char SpecularTexture[MAX_TEXTURE_NAME];

  D3DXVECTOR4 Diffuse;
  D3DXVECTOR4 Ambient;
  D3DXVECTOR4 Specular;
  D3DXVECTOR4 Emissive;
  float Power;

  union
  {
    UINT64 Force64_1; // Force the union to 64bits
  };
  union
  {
    UINT64 Force64_2; // Force the union to 64bits
  };
  union
  {
    UINT64 Force64_3; // Force the union to 64bits
  };

  union
  {
    UINT64 Force64_4; // Force the union to 64bits
  };
  union
  {
    UINT64 Force64_5; // Force the union to 64bits
  };
  union
  {
    UINT64 Force64_6; // Force the union to 64bits
  };
};

struct SDKANIMATION_FILE_HEADER
{
  UINT Version;
  BYTE IsBigEndian;
  UINT FrameTransformType;
  UINT NumFrames;
  UINT NumAnimationKeys;
  UINT AnimationFPS;
  UINT64 AnimationDataSize;
  UINT64 AnimationDataOffset;
};

struct SDKANIMATION_DATA
{
  D3DXVECTOR3 Translation;
  D3DXVECTOR4 Orientation;
  D3DXVECTOR3 Scaling;
};

struct SDKANIMATION_FRAME_DATA
{
  char FrameName[MAX_FRAME_NAME];
  union
  {
    UINT64 DataOffset;
    SDKANIMATION_DATA* pAnimationData;
  };
};

#endif //_SDKMESH_
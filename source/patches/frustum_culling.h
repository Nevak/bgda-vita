#ifndef FRUSTUM_CULLING_H
#define FRUSTUM_CULLING_H
#include <so_util/so_util.h>

// make sure it's packed and not padded

#define NAN(x) ((x) != (x)) // check if x is NaN
struct __attribute__((packed)) FrustumIdx {
	uint8_t xsel;
    uint8_t ysel;
    uint8_t zsel;
};
typedef struct FrustumIdx FrustumIdx;

struct FrustumIdx *g_frustumVertexIndices;
float *g_worldFrustum;

_Static_assert(sizeof(struct FrustumIdx) == 3, "FrustumIdx must be packed to 3 bytes");


so_hook worldClipCubeToFrustum_hook;
uint32_t worldClipCubeToFrustum(float *cubeVertices, int clippedPlanes)
{
    //Profiler_BeginSample("worldClipCubeToFrustum");

    // Rename cryptic variables to meaningful names
    int planeNumber;             // Better than planeIndex starting at -6
    FrustumIdx *planeIndices;
    float planeDistance;         
    float planeNormalY;            
    float planeNormalZ;
    
    /* Frustum culling function: tests if a 3D cube intersects with viewing frustum.
       Returns bitfield of planes that clip the cube (0 = fully inside frustum) */
    
    /* Loop through 6 frustum planes (left, right, top, bottom, near, far) */
    float *currentPlane = (float *)g_worldFrustum;
    planeIndices = g_frustumVertexIndices;
    
    for (planeNumber = 0; planeNumber < 6; planeNumber++) {
        uint32_t planeBitMask = 1 << planeNumber;
        
        /* Skip planes already marked as clipped */
        if ((clippedPlanes & planeBitMask) == 0) {
            // Access frustum plane data: each plane has 4 floats (nx, ny, nz, d)
            float planeNormalX = currentPlane[0];
            planeNormalY = currentPlane[1];
            planeNormalZ = currentPlane[2];
            planeDistance = currentPlane[3];
            
			uint8_t zsel = planeIndices[-1].zsel;
			uint8_t xsel = planeIndices->xsel;
			uint8_t ysel = planeIndices->ysel;

            /* Test cube's "far" vertex against plane: if behind plane, mark this plane as clipping */
            if (planeDistance + planeNormalX *
                cubeVertices[zsel] +
                planeNormalY * cubeVertices[xsel + 2] +
                planeNormalZ * cubeVertices[ysel + 4] < 0.0) {
                
                //Profiler_EndSample();
                return 0;
            }
            
            float farVertexDistance = planeDistance + planeNormalX *
                          cubeVertices[zsel ^ 1] +
                          planeNormalY * cubeVertices[(xsel ^ 1U) + 2] +
                          planeNormalZ * cubeVertices[(ysel ^ 1U) + 4];
            
            if (farVertexDistance < 0.0 == NAN(farVertexDistance)) {
                clippedPlanes = clippedPlanes | planeBitMask;
            }
        }
        
        /* Move to next frustum plane */
        currentPlane += 4;  // Each plane has 4 floats
        planeIndices++;
    }
    
    //Profiler_EndSample();
    return clippedPlanes;
}


so_hook worldClipCubeToClipFrustum_hook;
uint32_t worldClipCubeToClipFrustum(float *cubeVertices, int clippedPlanes)
{	
	//Profiler_BeginSample("worldClipCubeToClipFrustum");
	uint32_t x = SO_CONTINUE(uint32_t, worldClipCubeToClipFrustum_hook, cubeVertices, clippedPlanes);
	//Profiler_EndSample();
	//log_error("worldClipCubeToClipFrustum finished\n");
	return x;
}

so_hook worldClipCubeToFrustumOnce_hook;
uint32_t worldClipCubeToFrustumOnce(float *cubeVertices, int clippedPlanes)
{
	//Profiler_BeginSample("worldClipCubeToFrustumOnce");
	uint32_t x = SO_CONTINUE(uint32_t, worldClipCubeToFrustumOnce_hook, cubeVertices, clippedPlanes);
	//Profiler_EndSample();
	return x;
}

#endif
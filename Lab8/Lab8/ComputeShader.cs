cbuffer FrustumPlanes : register(b0)
{
    float4 planes[6];
};

struct InstanceData
{
    float4x4 model;
    uint texInd;
    uint countInstance;
    float2 padding;
};

StructuredBuffer<InstanceData> instanceData : register(t0);
RWByteAddressBuffer indirectArgs : register(u0);
RWStructuredBuffer<uint> objectIds : register(u1);

bool IsAABBInFrustum(in float3 center, in float size)
{
    for (int i = 0; i < 6; i++)
    {
        float d = dot(planes[i].xyz, center) + planes[i].w;
        float r = size * (abs(planes[i].x) + abs(planes[i].y) + abs(planes[i].z));

        if (d + r < 0)
        {
            return false;
        }
    }
    return true;
}

[numthreads(64, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    if (threadID.x >= instanceData[0].countInstance)
        return;

    float3 pos = instanceData[threadID.x].model._m03_m13_m23;

    float size = 0.5f * 0.95f;

    if (IsAABBInFrustum(pos, size)) 
    {
        uint index;
        indirectArgs.InterlockedAdd(4, 1, index);
        objectIds[index] = threadID.x;
    }
}
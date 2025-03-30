#ifndef RENDER_CLASS_H
#define RENDER_CLASS_H

#include <dxgi.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

class RenderClass
{
public:
    RenderClass() :
        m_pDevice(nullptr),
        m_pDeviceContext(nullptr),
        m_pSwapChain(nullptr),
        m_pRenderTargetView(nullptr),
        m_pVertexBuffer(nullptr),
        m_pIndexBuffer(nullptr),
        m_pPixelShader(nullptr),
        m_pVertexShader(nullptr),
        m_pLayout(nullptr),
        m_pModelBuffer(nullptr),
        m_pVPBuffer(nullptr),
        m_szTitle(nullptr),
        m_szWindowClass(nullptr),
        m_pTextureView(nullptr),
        m_pSamplerState(nullptr),
        m_pSkyboxSRV(nullptr),
        m_pSkyboxVB(nullptr),
        m_pSkyboxVPBuffer(nullptr),
        m_pSkyboxVS(nullptr),
        m_pSkyboxPS(nullptr),
        m_pSkyboxLayout(nullptr),
        m_pDepthView(nullptr),
        m_pColorBuffer(nullptr),
        m_ParallelogramVertexBuffer(nullptr),
        m_pParallelogramIndexBuffer(nullptr),
        m_pParallelogramPS(nullptr),
        m_pParallelogramVS(nullptr),
        m_pParallelogramLayout(nullptr),
        m_pBlendState(nullptr),
        m_pStateParallelogram(nullptr),
        m_pLightBuffer(nullptr),
        m_pLightPixelShader(nullptr),
        m_pNormalMapView(nullptr),
        m_pPostProcessTexture(nullptr),
        m_pPostProcessRTV(nullptr),
        m_pPostProcessSRV(nullptr),
        m_pPostProcessVS(nullptr),
        m_pPostProcessPS(nullptr),
        m_pFullScreenVB(nullptr),
        m_pFullScreenLayout(nullptr),
        m_pModelBufferInst(nullptr),
        m_CameraPosition(0.0f, 0.0f, -16.0f), 
        m_CameraSpeed(0.1f),
        m_LRAngle(0.0f), 
        m_UDAngle(0.0f),
        m_frustumPlanes{}
    {}

    HRESULT Init(HWND hWnd, WCHAR szTitle[], WCHAR szWindowClass[]);
    void Terminate();

    HRESULT InitBufferShader();
    void TerminateBufferShader();

    HRESULT Init2DArray();

    HRESULT InitFullScreenTriangle();

    HRESULT InitSkybox();
    void TerminateSkybox();

    HRESULT InitParallelogram();
    void TerminateParallelogram();

    HRESULT CompileShader(const std::wstring& path, ID3D11VertexShader** ppVertexShader, ID3D11PixelShader** ppPixelShader, ID3DBlob** pCodeShader=nullptr);

    void UpdateFrustum(const XMMATRIX& viewProjMatrix);
    bool IsAABBInFrustum(const XMFLOAT3& center, float size) const;

    void Render();
    void RenderSkybox(XMMATRIX proj);
    void RenderCubes(XMMATRIX view, XMMATRIX proj);
    void RenderParallelogram(XMVECTOR eyePos);

    void InitImGui(HWND hWnd);
    void RenderImGui();

    void Resize(HWND hWnd);
    void MoveCamera(float dx, float dy, float dz); 
    void RotateCamera(float yaw, float pitch);

private:
    struct CubeVertex
    {
        XMFLOAT3 xyz;
        XMFLOAT3 normal;
        XMFLOAT2 uv;
    };

    struct SkyboxVertex
    {
        float x, y, z;
    };

    struct ParallelogramVertex
    {
        float x, y, z;
    };

    struct MatrixBuffer
    {
        XMMATRIX m;
    };

    struct CameraBuffer
    {
        XMMATRIX vp;
        XMFLOAT3 cameraPos;
    };

    struct ColorBuffer
    {
        XMFLOAT4 color;
    };

    struct PointLight
    {
        XMFLOAT3 Position;
        float Range;
        XMFLOAT3 Color;
        float Intensity;
    };

    struct FullScreenVertex 
    {
        float x, y, z, w;
        float u, v;
    };

    struct InstanceData
    {
        XMMATRIX model;
        uint32_t texInd;  // 0 or 1  (cat.dds or textile.dds)
        float padding[3];
    };

    HRESULT ConfigureBackBuffer(UINT width, UINT height);

    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pDeviceContext;

    IDXGISwapChain* m_pSwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;

    ID3D11Buffer* m_pModelBuffer;
    ID3D11Buffer* m_pVPBuffer;

    ID3D11Buffer* m_pVertexBuffer;
    ID3D11Buffer* m_pIndexBuffer;

    ID3D11PixelShader* m_pPixelShader;
    ID3D11VertexShader* m_pVertexShader;
    ID3D11InputLayout* m_pLayout;

    ID3D11ShaderResourceView* m_pTextureView;
    ID3D11SamplerState* m_pSamplerState;

    ID3D11ShaderResourceView* m_pNormalMapView;

    ID3D11ShaderResourceView* m_pSkyboxSRV;
    ID3D11Buffer* m_pSkyboxVB;
    ID3D11Buffer* m_pSkyboxVPBuffer;
    ID3D11VertexShader* m_pSkyboxVS;
    ID3D11PixelShader* m_pSkyboxPS;
    ID3D11InputLayout* m_pSkyboxLayout;
    ID3D11DepthStencilView* m_pDepthView;

    ID3D11Buffer* m_pColorBuffer;
    ID3D11Buffer* m_ParallelogramVertexBuffer;
    ID3D11Buffer* m_pParallelogramIndexBuffer;

    ID3D11PixelShader* m_pParallelogramPS;
    ID3D11VertexShader* m_pParallelogramVS;
    ID3D11InputLayout* m_pParallelogramLayout;

    ID3D11BlendState* m_pBlendState;
    ID3D11DepthStencilState* m_pStateParallelogram;

    ID3D11Buffer* m_pLightBuffer;
    ID3D11PixelShader* m_pLightPixelShader;

    ID3D11Texture2D* m_pPostProcessTexture;
    ID3D11RenderTargetView* m_pPostProcessRTV;
    ID3D11ShaderResourceView* m_pPostProcessSRV;
    ID3D11VertexShader* m_pPostProcessVS;
    ID3D11PixelShader* m_pPostProcessPS;
    ID3D11Buffer* m_pFullScreenVB;
    ID3D11InputLayout* m_pFullScreenLayout;
    bool m_useNegative = false;

    const float m_fixedScale = 0.5f;
    ID3D11Buffer* m_pModelBufferInst;
    static const int MaxInst = 23;
    std::vector<InstanceData> m_modelInstances = {};

    XMVECTOR m_frustumPlanes[6];

    float m_CubeAngle = 0.0f;
    WCHAR* m_szTitle;
    WCHAR* m_szWindowClass;

    XMFLOAT3 m_CameraPosition; 
    float m_CameraSpeed;
    float m_LRAngle;    //turn left/right
    float m_UDAngle;    //turn up / down

    int m_visibleCubes = 0;

};
#endif

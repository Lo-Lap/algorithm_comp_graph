#include "framework.h"
#include "RenderClass.h"
#include "DDSTextureLoader11.h"
#include <filesystem>

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")

struct Vertex
{
    float x, y, z;
    float u, v;
};

struct SkyboxVertex 
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
};

HRESULT RenderClass::Init(HWND hWnd, WCHAR szTitle[], WCHAR szWindowClass[])
{
    m_szTitle = szTitle;
    m_szWindowClass = szWindowClass;

    HRESULT result;

    IDXGIFactory* pFactory = nullptr;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

    IDXGIAdapter* pSelectedAdapter = NULL;
    if (SUCCEEDED(result))
    {
        IDXGIAdapter* pAdapter = NULL;
        UINT adapterIdx = 0;
        while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter)))
        {
            DXGI_ADAPTER_DESC desc;
            pAdapter->GetDesc(&desc);

            if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
            {
                pSelectedAdapter = pAdapter;
                break;
            }

            pAdapter->Release();

            adapterIdx++;
        }
    }

    // Create DirectX 11 device
    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    if (SUCCEEDED(result))
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
            flags, levels, 1, D3D11_SDK_VERSION, &m_pDevice, &level, &m_pDeviceContext);
    }

    if (SUCCEEDED(result))
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = true;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = 0;

        result = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
    }

    if (SUCCEEDED(result))
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        UINT width = rc.right - rc.left;
        UINT height = rc.bottom - rc.top;
        result = ConfigureBackBuffer(width, height);
    }

    if (SUCCEEDED(result))
    {
        result = InitBufferShader();
    }

    if (SUCCEEDED(result))
    {
        result = InitSkybox();
    }

    pSelectedAdapter->Release();
    pFactory->Release();

    if (FAILED(result))
    {
        Terminate();
    }

    return result;
}

HRESULT RenderClass::InitBufferShader()
{
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT result = S_OK;

    ID3DBlob* pVertexCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileShader(L"ColorVertex.vs", &m_pVertexShader, nullptr, &pVertexCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileShader(L"ColorPixel.ps", nullptr, &m_pPixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(InputDesc, 2, pVertexCode->GetBufferPointer(), pVertexCode->GetBufferSize(), &m_pLayout);
    }

    if (pVertexCode)
        pVertexCode->Release();


    static const Vertex vertices[] =
    {
        {-1.0f, -1.0f,  1.0f, 0.0f, 1.0f},
        { 1.0f, -1.0f,  1.0f, 1.0f, 1.0f},
        { 1.0f, -1.0f, -1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f, -1.0f, 0.0f, 0.0f},

        {-1.0f,  1.0f, -1.0f, 0.0f, 1.0f},
        { 1.0f,  1.0f, -1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
        {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f},

        { 1.0f, -1.0f, -1.0f, 0.0f, 1.0f},
        { 1.0f, -1.0f,  1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
        { 1.0f,  1.0f, -1.0f, 0.0f, 0.0f},

        {-1.0f, -1.0f,  1.0f, 0.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f},
        {-1.0f,  1.0f, -1.0f, 1.0f, 0.0f},
        {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f},

        { 1.0f, -1.0f,  1.0f, 0.0f, 1.0f},
        {-1.0f, -1.0f,  1.0f, 1.0f, 1.0f},
        {-1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
        { 1.0f,  1.0f,  1.0f, 0.0f, 0.0f},

        {-1.0f, -1.0f, -1.0f, 0.0f, 1.0f},
        { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, -1.0f, 1.0f, 0.0f},
        {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f}
    };

    WORD indices[] =
    {
        0, 2, 1, 
        0, 3, 2,

        4, 6, 5, 
        4, 7, 6,

        8, 10, 9,
        8, 11, 10,

        12, 14, 13,
        12, 15, 14,

        16, 18, 17,
        16, 19, 18,

        20, 22, 21,
        20, 23, 22
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * ARRAYSIZE(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    result = m_pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
    if (FAILED(result))
        return result;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    initData.pSysMem = indices;
    result = m_pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
    if (FAILED(result))
        return result;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(XMMATRIX);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    result = m_pDevice->CreateBuffer(&bd, nullptr, &m_pModelBuffer);
    if (FAILED(result))
        return result;

    D3D11_BUFFER_DESC vpBufferDesc = {};
    vpBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vpBufferDesc.ByteWidth = sizeof(XMMATRIX);
    vpBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    vpBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = m_pDevice->CreateBuffer(&vpBufferDesc, nullptr, &m_pVPBuffer);
    if (FAILED(result))
        return result;

    result = DirectX::CreateDDSTextureFromFile(m_pDevice, L"cat.dds", nullptr, &m_pTextureView);
    if (FAILED(result)) 
        return result;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    result = m_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerState);

    return result;
}

HRESULT RenderClass::InitSkybox()
{
    ID3DBlob* pVertexCode = nullptr;
    HRESULT result = CompileShader(L"SkyboxVertex.vs", &m_pSkyboxVS, nullptr, &pVertexCode);
    if (SUCCEEDED(result))
    {
        result = CompileShader(L"SkyboxPixel.ps", nullptr, &m_pSkyboxPS);
    }

    D3D11_INPUT_ELEMENT_DESC skyboxLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(skyboxLayout, 1, pVertexCode->GetBufferPointer(), pVertexCode->GetBufferSize(), &m_pSkyboxLayout);
    }

    SkyboxVertex SkyboxVertices[] =
    {
        { -1.0f, -1.0f, -1.0f },
        { -1.0f,  1.0f, -1.0f },
        {  1.0f,  1.0f, -1.0f },
        { -1.0f, -1.0f, -1.0f },
        {  1.0f,  1.0f, -1.0f },
        {  1.0f, -1.0f, -1.0f },

        {  1.0f, -1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f },
        { -1.0f,  1.0f,  1.0f },
        {  1.0f, -1.0f,  1.0f },
        { -1.0f,  1.0f,  1.0f },
        { -1.0f, -1.0f,  1.0f },

        { -1.0f, -1.0f,  1.0f },
        { -1.0f,  1.0f,  1.0f },
        { -1.0f,  1.0f, -1.0f },
        { -1.0f, -1.0f,  1.0f },
        { -1.0f,  1.0f, -1.0f },
        { -1.0f, -1.0f, -1.0f },

        { 1.0f, -1.0f, -1.0f },
        { 1.0f,  1.0f, -1.0f },
        { 1.0f,  1.0f,  1.0f },
        { 1.0f, -1.0f, -1.0f },
        { 1.0f,  1.0f,  1.0f },
        { 1.0f, -1.0f,  1.0f },

        { -1.0f, 1.0f, -1.0f },
        { -1.0f, 1.0f,  1.0f },
        { 1.0f, 1.0f,  1.0f },
        { -1.0f, 1.0f, -1.0f },
        { 1.0f, 1.0f,  1.0f },
        { 1.0f, 1.0f, -1.0f },

        { -1.0f, -1.0f,  1.0f },
        { -1.0f, -1.0f, -1.0f },
        { 1.0f, -1.0f, -1.0f },
        { -1.0f, -1.0f,  1.0f },
        { 1.0f, -1.0f, -1.0f },
        { 1.0f, -1.0f,  1.0f },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SkyboxVertex) * ARRAYSIZE(SkyboxVertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = SkyboxVertices;
    result = m_pDevice->CreateBuffer(&bd, &initData, &m_pSkyboxVB);
    if (FAILED(result))
        return result;

    D3D11_BUFFER_DESC vpBufferDesc = {};
    vpBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vpBufferDesc.ByteWidth = sizeof(XMMATRIX);
    vpBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    vpBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = m_pDevice->CreateBuffer(&vpBufferDesc, nullptr, &m_pSkyboxVPBuffer);
    if (FAILED(result))
        return result;

    result = CreateDDSTextureFromFile(m_pDevice, L"skybox.dds", nullptr, &m_pSkyboxSRV);
    if (FAILED(result))
        return result;

    return S_OK;
}

void RenderClass::Terminate()
{
    TerminateBufferShader();
    TerminateSkybox();

    if (m_pDeviceContext) {
        m_pDeviceContext->ClearState();
        m_pDeviceContext->Release();
        m_pDeviceContext = nullptr;
    }
    
    if (m_pRenderTargetView)
    {
        m_pRenderTargetView->Release();
        m_pRenderTargetView = nullptr;
    }

    if (m_pDepthView)
    {
        m_pDepthView->Release();
        m_pDepthView = nullptr;
    }

    if (m_pSwapChain) {
        m_pSwapChain->Release();
        m_pSwapChain = nullptr;
    }
    
    if (m_pDevice) {
        m_pDevice->Release();
        m_pDevice = nullptr;
    }
}

void RenderClass::TerminateBufferShader()
{
    if (m_pLayout)
        m_pLayout->Release();

    if (m_pPixelShader)
        m_pPixelShader->Release();

    if (m_pVertexShader)
        m_pVertexShader->Release();

    if (m_pIndexBuffer)
        m_pIndexBuffer->Release();

    if (m_pVertexBuffer)
        m_pVertexBuffer->Release();

    if (m_pModelBuffer)
        m_pModelBuffer->Release();

    if (m_pVPBuffer)
        m_pVPBuffer->Release();

    if (m_pTextureView) 
        m_pTextureView->Release();

    if (m_pSamplerState) 
        m_pSamplerState->Release();
}

void RenderClass::TerminateSkybox()
{
    if (m_pSkyboxVB) {
        m_pSkyboxVB->Release();
    }

    if (m_pSkyboxSRV) {
        m_pSkyboxSRV->Release();
    }

    if (m_pSkyboxVPBuffer) {
        m_pSkyboxVPBuffer->Release();
    }

    if (m_pSkyboxLayout) {
        m_pSkyboxLayout->Release();
    }

    if (m_pSkyboxVS) {
        m_pSkyboxVS->Release();
    }

    if (m_pSkyboxPS) {
        m_pSkyboxPS->Release();
    }
}

std::wstring Extension(const std::wstring& path)
{
    size_t dotPos = path.find_last_of(L".");
    if (dotPos == std::wstring::npos || dotPos == 0)
    {
        return L"";
    }
    return path.substr(dotPos + 1);
}

HRESULT RenderClass::CompileShader(const std::wstring& path, ID3D11VertexShader** ppVertexShader, ID3D11PixelShader** ppPixelShader, ID3DBlob** pCodeShader)
{
    std::wstring extension = Extension(path);

    std::string platform = "";

    if (extension == L"vs")
    {
        platform = "vs_5_0";
    }
    else if (extension == L"ps")
    {
        platform = "ps_5_0";
    }

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif 

    ID3DBlob* pCode = nullptr;
    ID3DBlob* pErr = nullptr;

    HRESULT result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", platform.c_str(), 0, 0, &pCode, &pErr);
    if (!SUCCEEDED(result) && pErr != nullptr)
    {
        OutputDebugStringA((const char*)pErr->GetBufferPointer());
    }
    if (pErr)
        pErr->Release();

    if (SUCCEEDED(result))
    {
        if (extension == L"vs" && ppVertexShader)
        {
            result = m_pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, ppVertexShader);
            if (FAILED(result))
            {
                pCode->Release();
                return result;
            }
        }
        else if (extension == L"ps" && ppPixelShader)
        {
            result = m_pDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, ppPixelShader);
            if (FAILED(result))
            {
                pCode->Release();
                return result;
            }
        }
    }

    if (pCodeShader)
    {
        *pCodeShader = pCode;
    }
    else
    {
        pCode->Release();
    }
    return result;
}

void RenderClass::MoveCamera(float dx, float dy, float dz)
{
    m_CameraPosition.x += dx * m_CameraSpeed;
    m_CameraPosition.y += dy * m_CameraSpeed;
    m_CameraPosition.z += dz * m_CameraSpeed;
}

void RenderClass::RotateCamera(float lrAngle, float udAngle)
{
    m_LRAngle += lrAngle;
    m_UDAngle -= udAngle;

    // when we turn completely around our axis, we return to the place.
    if (m_LRAngle > XM_2PI) m_LRAngle -= XM_2PI;
    if (m_LRAngle < -XM_2PI) m_LRAngle += XM_2PI;

    // to avoid tilting the camera:
    if (m_UDAngle > XM_PIDIV2) m_UDAngle = XM_PIDIV2;
    if (m_UDAngle < -XM_PIDIV2) m_UDAngle = -XM_PIDIV2;
}

void RenderClass::Render()
{
    m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthView);
    float BackColor[4] = { 0.48f, 0.57f, 0.48f, 1.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, BackColor);
    m_pDeviceContext->ClearDepthStencilView(m_pDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    SetMVPBuffer();

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->IASetInputLayout(m_pLayout);

    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    m_pDeviceContext->PSSetShaderResources(0, 1, &m_pTextureView);
    m_pDeviceContext->PSSetSamplers(0, 1, &m_pSamplerState);
    m_pDeviceContext->DrawIndexed(36, 0, 0);

    m_pSwapChain->Present(1, 0);
}

void RenderClass::SetMVPBuffer()
{
    XMMATRIX rotLR = XMMatrixRotationY(m_LRAngle);
    XMMATRIX rotUD = XMMatrixRotationX(m_UDAngle);
    XMMATRIX totalRot = rotLR * rotUD;

    XMVECTOR defaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR direction = XMVector3TransformNormal(defaultForward, totalRot);

    XMVECTOR eyePos = XMVectorSet(m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z, 0.0f);
    XMVECTOR focusPoint = XMVectorAdd(eyePos, direction);
    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(eyePos, focusPoint, upDir);

    RECT rc;
    GetClientRect(FindWindow(m_szWindowClass, m_szTitle), &rc);
    float aspect = static_cast<float>(rc.right - rc.left) / (rc.bottom - rc.top);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.0f);

    XMMATRIX rotLRSky = XMMatrixRotationY(-m_LRAngle);
    XMMATRIX rotUDSky = XMMatrixRotationX(-m_UDAngle);
    XMMATRIX viewSkybox = rotLRSky * rotUDSky;
    XMMATRIX vpSkybox = XMMatrixTranspose(viewSkybox * proj);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_pDeviceContext->Map(m_pSkyboxVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &vpSkybox, sizeof(XMMATRIX));
        m_pDeviceContext->Unmap(m_pSkyboxVPBuffer, 0);
    }

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    ID3D11DepthStencilState* pDSStateSkybox = nullptr;
    m_pDevice->CreateDepthStencilState(&dsDesc, &pDSStateSkybox);
    m_pDeviceContext->OMSetDepthStencilState(pDSStateSkybox, 0);

    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_FRONT;
    rsDesc.FrontCounterClockwise = false;
    ID3D11RasterizerState* pSkyboxRS = nullptr;
    if (SUCCEEDED(m_pDevice->CreateRasterizerState(&rsDesc, &pSkyboxRS)))
    {
        m_pDeviceContext->RSSetState(pSkyboxRS);
    }

    UINT stride = sizeof(SkyboxVertex);
    UINT offset = 0;
    m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pSkyboxVB, &stride, &offset);
    m_pDeviceContext->IASetInputLayout(m_pSkyboxLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_pDeviceContext->VSSetShader(m_pSkyboxVS, nullptr, 0);
    m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pSkyboxVPBuffer);

    m_pDeviceContext->PSSetShader(m_pSkyboxPS, nullptr, 0);
    m_pDeviceContext->PSSetShaderResources(0, 1, &m_pSkyboxSRV);
    m_pDeviceContext->PSSetSamplers(0, 1, &m_pSamplerState);

    m_pDeviceContext->Draw(36, 0);

    pDSStateSkybox->Release();
    if (pSkyboxRS)
    {
        pSkyboxRS->Release();
        m_pDeviceContext->RSSetState(nullptr);
    }

    m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthView);

    m_CubeAngle += 0.01f;
    if (m_CubeAngle > XM_2PI) m_CubeAngle -= XM_2PI;
    XMMATRIX model = XMMatrixRotationY(m_CubeAngle);

    XMMATRIX vp = view * proj;
    XMMATRIX mT = XMMatrixTranspose(model);
    XMMATRIX vpT = XMMatrixTranspose(vp);

    m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &mT, 0, 0);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = m_pDeviceContext->Map(m_pVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &vpT, sizeof(XMMATRIX));
        m_pDeviceContext->Unmap(m_pVPBuffer, 0);
    }
    m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pModelBuffer);
    m_pDeviceContext->VSSetConstantBuffers(1, 1, &m_pVPBuffer);
}

HRESULT RenderClass::ConfigureBackBuffer(UINT width, UINT height)
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* pDepthStencil = nullptr;
    hr = m_pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);
    if (FAILED(hr))
        return hr;

    hr = m_pDevice->CreateDepthStencilView(pDepthStencil, nullptr, &m_pDepthView);
    pDepthStencil->Release();
    if (FAILED(hr))
        return hr;

    return hr;
}

void RenderClass::Resize(HWND hWnd)
{
    if (m_pRenderTargetView)
    {
        m_pRenderTargetView->Release();
        m_pRenderTargetView = nullptr;
    }

    if (m_pDepthView) 
    {
        m_pDepthView->Release();
        m_pDepthView = nullptr;
    }

    if (m_pSwapChain)
    {
        HRESULT hr;

        RECT rc;
        GetClientRect(hWnd, &rc);
        UINT width = rc.right - rc.left;
        UINT height = rc.bottom - rc.top;

        hr = m_pSwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"ResizeBuffers failed.", L"Error", MB_OK);
            return;
        }

        HRESULT resultBack = ConfigureBackBuffer(width, height);
        if (FAILED(resultBack))
        {
            MessageBox(nullptr, L"Configure back buffer failed.", L"Error", MB_OK);
            return;
        }

        m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthView);

        D3D11_VIEWPORT vp;
        vp.Width = (FLOAT)width;
        vp.Height = (FLOAT)height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        m_pDeviceContext->RSSetViewports(1, &vp);
    }
}
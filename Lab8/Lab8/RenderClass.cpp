#include "framework.h"
#include "RenderClass.h"
#include "DDSTextureLoader11.h"
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")



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
        result = Init2DArray();
    }

    if (SUCCEEDED(result))
    {
        result = InitSkybox();
    }

    if (SUCCEEDED(result)) 
    {
        InitImGui(hWnd);
    }
    
    if (SUCCEEDED(result))
    {
        result = InitParallelogram();
    }

    if (SUCCEEDED(result))
    {
        result = InitFullScreenTriangle();
    }

    if (SUCCEEDED(result))
    {
        result = InitComputeShader();
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
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(CubeVertex, xyz),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(CubeVertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(CubeVertex, uv),     D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
        result = m_pDevice->CreateInputLayout(layout, 3, pVertexCode->GetBufferPointer(), pVertexCode->GetBufferSize(), &m_pLayout);
    }

    if (pVertexCode)
        pVertexCode->Release();

    if (SUCCEEDED(result))
    {
        result = CompileShader(L"LightPixel.ps", nullptr, &m_pLightPixelShader);
    }

    static const CubeVertex vertices[] =
    {
        { {-1.0f, -1.0f,  1.0f}, { 0.0f,  -1.0f,  0.0f}, {0.0f, 1.0f} }, 
        { { 1.0f, -1.0f,  1.0f}, { 0.0f,  -1.0f,  0.0f}, {1.0f, 1.0f} }, 
        { { 1.0f, -1.0f, -1.0f}, { 0.0f,  -1.0f,  0.0f}, {1.0f, 0.0f} }, 
        { {-1.0f, -1.0f, -1.0f}, { 0.0f,  -1.0f,  0.0f}, {0.0f, 0.0f} }, 

        { {-1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f, 0.0f}, {0.0f, 1.0f} },
        { { 1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f, 0.0f}, {1.0f, 1.0f} },
        { { 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f, 0.0f}, {1.0f, 0.0f} },
        { {-1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f, 0.0f}, {0.0f, 0.0f} },

        { { 1.0f, -1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
        { { 1.0f, -1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
        { { 1.0f,  1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },
        { { 1.0f,  1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },

        { {-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
        { {-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
        { {-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },
        { {-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },

        { { 1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f} },
        { {-1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f} },
        { {-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f} },
        { { 1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f} },

        { {-1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f,  -1.0f}, {0.0f, 1.0f} },
        { { 1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f,  -1.0f}, {1.0f, 1.0f} },
        { { 1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f,  -1.0f}, {1.0f, 0.0f} },
        { {-1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f,  -1.0f}, {0.0f, 0.0f} },
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

    D3D11_BUFFER_DESC lightBufferDesc = {};
    lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightBufferDesc.ByteWidth = sizeof(PointLight) * 3;
    lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = m_pDevice->CreateBuffer(&lightBufferDesc, nullptr, &m_pLightBuffer);
    if (FAILED(result))
        return result;


    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CubeVertex) * ARRAYSIZE(vertices);
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

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(InstanceData) * MaxInst;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    result = m_pDevice->CreateBuffer(&bd, nullptr, &m_pModelBufferInst);
    if (FAILED(result))
        return result;

    // Init instances cubes
    const int innerCount = 10;
    const int outerCount = 12;
    const float innerRadius = 4.0f;
    const float outerRadius = 9.5f;

    InstanceData modelBuf;
    modelBuf.countInstance = MaxInst;
    modelBuf.model = XMMatrixScaling(m_fixedScale, m_fixedScale, m_fixedScale);
    modelBuf.texInd = 0;
    m_modelInstances.push_back(modelBuf);

    for (int i = 0; i < innerCount; i++)
    {
        float angle = XM_2PI * i / innerCount;
        XMFLOAT3 position = { innerRadius * cosf(angle), 0.0f, innerRadius * sinf(angle) };

        InstanceData modelBuf;
        modelBuf.countInstance = MaxInst;
        modelBuf.model = XMMatrixScaling(m_fixedScale, m_fixedScale, m_fixedScale) *
            XMMatrixTranslation(position.x, position.y, position.z);
        modelBuf.texInd = i % 2;
        m_modelInstances.push_back(modelBuf);
    }

    for (int i = 0; i < outerCount; i++)
    {
        float angle = XM_2PI * i / outerCount;
        XMFLOAT3 position = { outerRadius * cosf(angle), 0.0f, outerRadius * sinf(angle) };

        InstanceData modelBuf;
        modelBuf.countInstance = MaxInst;
        modelBuf.model = XMMatrixScaling(m_fixedScale, m_fixedScale, m_fixedScale) *
            XMMatrixTranslation(position.x, position.y, position.z);
        modelBuf.texInd = i % 2;
        m_modelInstances.push_back(modelBuf);
    }

    m_pDeviceContext->UpdateSubresource(m_pModelBufferInst, 0, nullptr, m_modelInstances.data(), 0, 0);

    D3D11_BUFFER_DESC vpBufferDesc = {};
    vpBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vpBufferDesc.ByteWidth = sizeof(CameraBuffer);
    vpBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    vpBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = m_pDevice->CreateBuffer(&vpBufferDesc, nullptr, &m_pVPBuffer);
    if (FAILED(result))
        return result;

    result = DirectX::CreateDDSTextureFromFile(m_pDevice, L"cube_normal.dds", nullptr, &m_pNormalMapView);
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

HRESULT RenderClass::Init2DArray()
{
    ID3D11Resource* pTextureResources[2] = { nullptr, nullptr };

    HRESULT result = DirectX::CreateDDSTextureFromFile(
        m_pDevice,
        L"cat.dds",
        &pTextureResources[0],
        nullptr
    );
    if (FAILED(result)) 
        return result;

    result = DirectX::CreateDDSTextureFromFile(
        m_pDevice,
        L"textile.dds",
        &pTextureResources[1],
        nullptr
    );
    if (FAILED(result))
    {
        pTextureResources[0]->Release();
        return result;
    }

    ID3D11Texture2D* pTexture = nullptr;
    pTextureResources[0]->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);

    D3D11_TEXTURE2D_DESC texDesc;
    pTexture->GetDesc(&texDesc);
    pTexture->Release();

    D3D11_TEXTURE2D_DESC arrayDesc = texDesc;
    arrayDesc.ArraySize = 2;

    ID3D11Texture2D* pTextureArray = nullptr;
    result = m_pDevice->CreateTexture2D(&arrayDesc, nullptr, &pTextureArray);
    if (FAILED(result))
    {
        pTextureResources[0]->Release();
        pTextureResources[1]->Release();
        return result;
    }

    for (UINT i = 0; i < 2; ++i)
    {
        for (UINT mip = 0; mip < texDesc.MipLevels; ++mip)
        {
            if (pTextureResources[i] != nullptr) 
            {
                m_pDeviceContext->CopySubresourceRegion(
                    pTextureArray,
                    D3D11CalcSubresource(mip, i, texDesc.MipLevels),
                    0, 0, 0,
                    pTextureResources[i],
                    mip,
                    nullptr
                );
            }
            else 
            {
                OutputDebugString(L"Error: Null texture resource encountered\n");
            }
        }
    }
    pTextureResources[0]->Release();
    pTextureResources[1]->Release();

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.ArraySize = 2;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.MipLevels = texDesc.MipLevels;
    srvDesc.Texture2DArray.MostDetailedMip = 0;

    result = m_pDevice->CreateShaderResourceView(pTextureArray, &srvDesc, &m_pTextureView);
    pTextureArray->Release();
    return result;
}

HRESULT RenderClass::InitFullScreenTriangle()
{
    FullScreenVertex vertices[3] = {
        { -1.0f, -1.0f, 0, 1,   0.0f, 1.0f },
        { -1.0f,  3.0f, 0, 1,   0.0f,-1.0f },
        {  3.0f, -1.0f, 0, 1,   2.0f, 1.0f }
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    HRESULT hr = m_pDevice->CreateBuffer(&bd, &initData, &m_pFullScreenVB);
    if (FAILED(hr)) return hr;

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShader(L"NegativeVertex.vs", &m_pPostProcessVS, nullptr, &pVSBlob);
    if (FAILED(hr)) return hr;

    hr = m_pDevice->CreateInputLayout(layout, 2, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &m_pFullScreenLayout);
    if (pVSBlob) pVSBlob->Release();
    if (FAILED(hr)) return hr;

    hr = CompileShader(L"NegativePixel.ps", nullptr, &m_pPostProcessPS);
    return hr;
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

HRESULT RenderClass::InitParallelogram() 
{
    ID3DBlob* pVertexCode = nullptr;
    HRESULT result = CompileShader(L"ParallelogramVertex.vs", &m_pParallelogramVS, nullptr, &pVertexCode);
    if (SUCCEEDED(result))
    {
        result = CompileShader(L"ParallelogramPixel.ps", nullptr, &m_pParallelogramPS);
    }

    D3D11_INPUT_ELEMENT_DESC parallelogramLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(parallelogramLayout, 1, pVertexCode->GetBufferPointer(), pVertexCode->GetBufferSize(), &m_pParallelogramLayout);
    }

    ParallelogramVertex ParallelogramVertices[] =
    {
        {-0.75, -0.75, 0.0},
        {-0.75,  0.75, 0.0},
        { 0.75,  0.75, 0.0},
        { 0.75, -0.75, 0.0}
    };

    WORD ParallelogramIndices[] =
    {
        0, 1, 2,
        0, 2, 3
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ParallelogramVertex) * ARRAYSIZE(ParallelogramVertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = ParallelogramVertices;
    result = m_pDevice->CreateBuffer(&bd, &initData, &m_ParallelogramVertexBuffer);
    if (FAILED(result))
        return result;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * ARRAYSIZE(ParallelogramIndices);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    initData.pSysMem = ParallelogramIndices;
    result = m_pDevice->CreateBuffer(&bd, &initData, &m_pParallelogramIndexBuffer);
    if (FAILED(result))
        return result;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(ColorBuffer);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;
    result = m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pColorBuffer);
    if (FAILED(result))
        return result;

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    result = m_pDevice->CreateBlendState(&blendDesc, &m_pBlendState);
    if (FAILED(result))
        return result;

    D3D11_DEPTH_STENCIL_DESC dsDescTrans = {};
    dsDescTrans.DepthEnable = true;
    dsDescTrans.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDescTrans.DepthFunc = D3D11_COMPARISON_LESS;
    result = m_pDevice->CreateDepthStencilState(&dsDescTrans, &m_pStateParallelogram);
    return result;
}

HRESULT RenderClass::InitComputeShader()
{
    HRESULT result = CompileComputeShader(L"ComputeShader.cs", &m_pComputeShader);
    if (FAILED(result)) 
        return result;

    D3D11_BUFFER_DESC frustumDesc = {};
    frustumDesc.ByteWidth = sizeof(XMVECTOR) * 6;
    frustumDesc.Usage = D3D11_USAGE_DYNAMIC;
    frustumDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    frustumDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = m_pDevice->CreateBuffer(&frustumDesc, nullptr, &m_pFrustumPlanesBuffer);
    if (FAILED(result)) return result;

    D3D11_BUFFER_DESC argsDesc = {};
    argsDesc.ByteWidth = sizeof(UINT) * 5;
    argsDesc.Usage = D3D11_USAGE_DEFAULT;
    argsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    argsDesc.CPUAccessFlags = 0;
    argsDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    argsDesc.StructureByteStride = 0;

    result = m_pDevice->CreateBuffer(&argsDesc, nullptr, &m_pIndirectArgsBuffer);
    if (FAILED(result))
        return result;

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavArgsDesc = {};
    uavArgsDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavArgsDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavArgsDesc.Buffer.FirstElement = 0;
    uavArgsDesc.Buffer.NumElements = argsDesc.ByteWidth / sizeof(UINT);
    uavArgsDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    result = m_pDevice->CreateUnorderedAccessView(m_pIndirectArgsBuffer, &uavArgsDesc, &m_pIndirectArgsUAV);
    if (FAILED(result)) 
        return result;

    D3D11_BUFFER_DESC idsDesc = {};
    idsDesc.ByteWidth = sizeof(UINT) * MaxInst;
    idsDesc.Usage = D3D11_USAGE_DEFAULT;
    idsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    idsDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    idsDesc.StructureByteStride = sizeof(UINT);

    result = m_pDevice->CreateBuffer(&idsDesc, nullptr, &m_pObjectsIdsBuffer);
    if (FAILED(result)) 
        return result;

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = MaxInst;

    result = m_pDevice->CreateUnorderedAccessView(m_pObjectsIdsBuffer, &uavDesc, &m_pObjectsIdsUAV);
    if (FAILED(result)) 
        return result;

    D3D11_BUFFER_DESC instanceDesc = {};
    instanceDesc.ByteWidth = sizeof(InstanceData) * MaxInst;
    instanceDesc.Usage = D3D11_USAGE_DEFAULT;
    instanceDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    instanceDesc.CPUAccessFlags = 0;
    instanceDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    instanceDesc.StructureByteStride = sizeof(InstanceData);

    ID3D11Buffer* pInstanceStructuredBuffer = nullptr;
    result = m_pDevice->CreateBuffer(&instanceDesc, nullptr, &pInstanceStructuredBuffer);
    if (FAILED(result)) 
        return result;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = MaxInst;

    result = m_pDevice->CreateShaderResourceView(pInstanceStructuredBuffer, &srvDesc, &m_pInstanceDataSRV);
    if (FAILED(result))
    {
        pInstanceStructuredBuffer->Release();
        return result;
    }

    m_pDeviceContext->UpdateSubresource(pInstanceStructuredBuffer, 0, nullptr, m_modelInstances.data(), 0, 0);
    pInstanceStructuredBuffer->Release();

    return S_OK;
}

void RenderClass::TerminateComputeShader()
{
    if (m_pComputeShader) 
        m_pComputeShader->Release();

    if (m_pFrustumPlanesBuffer) 
        m_pFrustumPlanesBuffer->Release();

    if (m_pIndirectArgsBuffer) 
        m_pIndirectArgsBuffer->Release();

    if (m_pObjectsIdsBuffer) 
        m_pObjectsIdsBuffer->Release();

    if (m_pIndirectArgsUAV) 
        m_pIndirectArgsUAV->Release();

    if (m_pObjectsIdsUAV) 
        m_pObjectsIdsUAV->Release();

    if (m_pInstanceDataSRV) 
        m_pInstanceDataSRV->Release();
}

void RenderClass::TerminateParallelogram()
{
    if (m_ParallelogramVertexBuffer)
        m_ParallelogramVertexBuffer -> Release();

    if (m_pParallelogramIndexBuffer)
        m_pParallelogramIndexBuffer -> Release();

    if (m_pParallelogramPS)
        m_pParallelogramPS->Release();

    if (m_pParallelogramVS)
        m_pParallelogramVS->Release();

    if (m_pParallelogramLayout)
        m_pParallelogramLayout->Release();

    if (m_pColorBuffer)
        m_pColorBuffer->Release();

    if (m_pBlendState) 
        m_pBlendState->Release();

    if (m_pStateParallelogram) 
        m_pStateParallelogram->Release();
}

void RenderClass::Terminate()
{
    TerminateBufferShader();
    TerminateSkybox();
    TerminateParallelogram();
    TerminateComputeShader();

    if (m_pDeviceContext) 
    {
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

    if (m_pSwapChain) 
    {
        m_pSwapChain->Release();
        m_pSwapChain = nullptr;
    }
    
    if (m_pDevice) 
    {
        m_pDevice->Release();
        m_pDevice = nullptr;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void RenderClass::TerminateBufferShader()
{
    if (m_pLayout)
        m_pLayout->Release();

    if (m_pPixelShader)
        m_pPixelShader->Release();

    if (m_pVertexShader)
        m_pVertexShader->Release();

    if (m_pLightPixelShader)
        m_pLightPixelShader->Release();

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

    if (m_pNormalMapView)
        m_pNormalMapView->Release();

    if (m_pSamplerState) 
        m_pSamplerState->Release();

    if (m_pLightBuffer)
        m_pLightBuffer->Release();

    if (m_pModelBufferInst)
        m_pModelBufferInst->Release();

    if (m_pPostProcessTexture) 
        m_pPostProcessTexture->Release();

    if (m_pPostProcessRTV) 
        m_pPostProcessRTV->Release();

    if (m_pPostProcessSRV) 
        m_pPostProcessSRV->Release();

    if (m_pPostProcessVS) 
        m_pPostProcessVS->Release();

    if (m_pPostProcessPS) 
        m_pPostProcessPS->Release();

    if (m_pFullScreenVB) 
        m_pFullScreenVB->Release();

    if (m_pFullScreenLayout) 
        m_pFullScreenLayout->Release();

    m_modelInstances.clear();
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

HRESULT RenderClass::CompileComputeShader(const std::wstring& path, ID3D11ComputeShader** ppComputeShader)
{
    std::wstring extension = Extension(path);

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif 

    ID3DBlob* pCode = nullptr;
    ID3DBlob* pErr = nullptr;

    HRESULT result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "cs_5_0", flags, 0, &pCode, &pErr);
    if (!SUCCEEDED(result) && pErr != nullptr)
    {
        OutputDebugStringA((const char*)pErr->GetBufferPointer());
    }
    if (pErr)
        pErr->Release();

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateComputeShader(pCode->GetBufferPointer(), pCode->GetBufferSize(),
            nullptr, ppComputeShader);
    }

    if (pCode)
        pCode->Release();

    return result;
}

void RenderClass::MoveCamera(float dx, float dy, float dz)
{
    if (m_CameraPosition.z <= 0)
    {
        m_CameraPosition.x += dx * m_CameraSpeed;
    }
    else
    {
        m_CameraPosition.x -= dx * m_CameraSpeed;
    }

    m_CameraPosition.y += dy * m_CameraSpeed;
    m_CameraPosition.z += dz * m_CameraSpeed;
}

void RenderClass::RotateCamera(float lrAngle, float udAngle)
{
    m_LRAngle += lrAngle;
    m_UDAngle -= udAngle;

    // when we turn completely around our axis, we return to the place.
    m_LRAngle = fmodf(m_LRAngle, XM_2PI);
    if (m_LRAngle > XM_PI) m_LRAngle -= XM_2PI;
    if (m_LRAngle < -XM_PI) m_LRAngle += XM_2PI;

    // to avoid tilting the camera:
    if (m_UDAngle > XM_PIDIV2) m_UDAngle = XM_PIDIV2;
    if (m_UDAngle < -XM_PIDIV2) m_UDAngle = -XM_PIDIV2;
}

void RenderClass::UpdateFrustum(const XMMATRIX& viewProjMatrix) 
{
    XMFLOAT4X4 matrix;
    XMStoreFloat4x4(&matrix, viewProjMatrix);

    m_frustumPlanes[0] = XMVectorSet(
        matrix._14 + matrix._11,
        matrix._24 + matrix._21,
        matrix._34 + matrix._31,
        matrix._44 + matrix._41
    );

    m_frustumPlanes[1] = XMVectorSet(
        matrix._14 - matrix._11,
        matrix._24 - matrix._21,
        matrix._34 - matrix._31,
        matrix._44 - matrix._41
    );

    m_frustumPlanes[2] = XMVectorSet(
        matrix._14 + matrix._12,
        matrix._24 + matrix._22,
        matrix._34 + matrix._32,
        matrix._44 + matrix._42
    );

    m_frustumPlanes[3] = XMVectorSet(
        matrix._14 - matrix._12,
        matrix._24 - matrix._22,
        matrix._34 - matrix._32,
        matrix._44 - matrix._42
    );

    m_frustumPlanes[4] = XMVectorSet(
        matrix._13,
        matrix._23,
        matrix._33,
        matrix._43
    );

    m_frustumPlanes[5] = XMVectorSet(
        matrix._14 - matrix._13,
        matrix._24 - matrix._23,
        matrix._34 - matrix._33,
        matrix._44 - matrix._43
    );

    for (int i = 0; i < 6; i++) 
    {
        m_frustumPlanes[i] = XMPlaneNormalize(m_frustumPlanes[i]);
    }
}

bool RenderClass::IsAABBInFrustum(const XMFLOAT3& center, float size) const
{
    for (int i = 0; i < 6; i++) 
    {
        float d = XMVectorGetX(XMPlaneDotCoord(m_frustumPlanes[i], XMLoadFloat3(&center)));
        float r = size * (fabs(XMVectorGetX(m_frustumPlanes[i])) +
            fabs(XMVectorGetY(m_frustumPlanes[i])) +
            fabs(XMVectorGetZ(m_frustumPlanes[i])));

        if (d + r < 0) 
        {
            return false;
        }
    }
    return true;
}

void RenderClass::Render()
{
    ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
    m_pDeviceContext->PSSetShaderResources(0, 1, nullSRVs);
    m_pDeviceContext->VSSetShaderResources(0, 1, nullSRVs);

    float clearColor[4] = { 0.48f, 0.57f, 0.48f, 1.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pPostProcessRTV, clearColor);
    m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);
    m_pDeviceContext->ClearDepthStencilView(m_pDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    m_pDeviceContext->OMSetRenderTargets(1, &m_pPostProcessRTV, m_pDepthView);

    XMMATRIX rotLR = XMMatrixRotationY(m_LRAngle);
    XMMATRIX rotUD = XMMatrixRotationX(m_UDAngle);
    XMMATRIX totalRot;
    if (m_CameraPosition.z <= 0)
    {
        totalRot = rotLR * rotUD;
    }
    else
    {
        totalRot = rotUD * rotLR;
    }

    XMVECTOR eyePos = XMVectorSet(m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z, 0.0f);
    XMVECTOR focusPoint = XMVectorAdd(eyePos, XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), totalRot));
    XMMATRIX view = XMMatrixLookAtLH(eyePos, focusPoint, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    RECT rc;
    GetClientRect(FindWindow(m_szWindowClass, m_szTitle), &rc);
    float aspect = static_cast<float>(rc.right - rc.left) / (rc.bottom - rc.top);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.0f);

    RenderSkybox(proj);
    RenderCubes(view, proj);
    RenderParallelogram(eyePos);

    m_pDeviceContext->PSSetShaderResources(0, 1, nullSRVs);
    m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);

    if (m_useNegative)
    {
        m_pDeviceContext->VSSetShader(m_pPostProcessVS, nullptr, 0);
        m_pDeviceContext->PSSetShader(m_pPostProcessPS, nullptr, 0);
        m_pDeviceContext->IASetInputLayout(m_pFullScreenLayout);

        m_pDeviceContext->PSSetShaderResources(0, 1, &m_pPostProcessSRV);
        m_pDeviceContext->PSSetSamplers(0, 1, &m_pSamplerState);

        UINT stride = sizeof(FullScreenVertex);
        UINT offset = 0;
        m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pFullScreenVB, &stride, &offset);
        m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pDeviceContext->Draw(3, 0);
    }
    else
    {
        ID3D11Resource* srcResource = nullptr;
        m_pPostProcessSRV->GetResource(&srcResource);

        ID3D11Resource* dstResource = nullptr;
        m_pRenderTargetView->GetResource(&dstResource);

        if (srcResource && dstResource)
        {
            m_pDeviceContext->CopyResource(dstResource, srcResource);
        }

        if (srcResource) srcResource->Release();
        if (dstResource) dstResource->Release();
    }

    RenderImGui();
    m_pSwapChain->Present(1, 0);
    m_pDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_pDeviceContext->PSSetShaderResources(0, 1, nullSRVs);
}

void RenderClass::RenderSkybox(XMMATRIX proj)
{
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
}

std::vector<UINT> RenderClass::ReadUintBufferData(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, UINT count)
{
    std::vector<UINT> result(count);

    D3D11_BUFFER_DESC desc;
    pBuffer->GetDesc(&desc);

    D3D11_BUFFER_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ID3D11Buffer* pStagingBuffer = nullptr;
    if (SUCCEEDED(m_pDevice->CreateBuffer(&stagingDesc, nullptr, &pStagingBuffer)))
    {
        pContext->CopyResource(pStagingBuffer, pBuffer);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(pContext->Map(pStagingBuffer, 0, D3D11_MAP_READ, 0, &mapped)))
        {
            memcpy(result.data(), mapped.pData, sizeof(UINT) * count);
            pContext->Unmap(pStagingBuffer, 0);
        }
        pStagingBuffer->Release();
    }

    return result;
}

void RenderClass::RenderCubes(XMMATRIX view, XMMATRIX proj)
{
    m_pDeviceContext->OMSetRenderTargets(1, &m_pPostProcessRTV, m_pDepthView);
    m_pDeviceContext->OMSetDepthStencilState(nullptr, 0);

    CameraBuffer cameraBuffer;
    cameraBuffer.vp = XMMatrixTranspose(view * proj);
    cameraBuffer.cameraPos = m_CameraPosition;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = m_pDeviceContext->Map(m_pVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &cameraBuffer, sizeof(CameraBuffer));
        m_pDeviceContext->Unmap(m_pVPBuffer, 0);
    }

    UINT stride = sizeof(CubeVertex);
    UINT offset = 0;
    m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->IASetInputLayout(m_pLayout);

    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    m_pDeviceContext->VSSetConstantBuffers(1, 1, &m_pVPBuffer);

    m_pDeviceContext->PSSetShaderResources(0, 1, &m_pTextureView);
    m_pDeviceContext->PSSetShaderResources(1, 1, &m_pNormalMapView);
    m_pDeviceContext->PSSetSamplers(0, 1, &m_pSamplerState);

    UpdateFrustum(view * proj);

    m_CubeAngle += 0.01f;
    if (m_CubeAngle > XM_2PI) m_CubeAngle -= XM_2PI;

    if (m_pComputeShader)
    {
        // GPU frustum culling
        //OutputDebugString(L"Frustum Culling in GPU\n");
        for (int i = 0; i < m_modelInstances.size(); i++) 
        {
            XMFLOAT3 position;
            XMStoreFloat3(&position, m_modelInstances[i].model.r[3]);

            m_modelInstances[i].model = XMMatrixScaling(m_fixedScale, m_fixedScale, m_fixedScale) *
                XMMatrixRotationY(m_CubeAngle) *
                XMMatrixTranslation(position.x, position.y, position.z);
        }

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_pDeviceContext->Map(m_pFrustumPlanesBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, m_frustumPlanes, sizeof(XMVECTOR) * 6);
            m_pDeviceContext->Unmap(m_pFrustumPlanesBuffer, 0);
        }

        UINT initialArgs[5] = { 36, 0, 0, 0, 0 }; 
        m_pDeviceContext->UpdateSubresource(m_pIndirectArgsBuffer, 0, nullptr, initialArgs, 0, 0);

        m_pDeviceContext->CSSetShader(m_pComputeShader, nullptr, 0);
        m_pDeviceContext->CSSetConstantBuffers(0, 1, &m_pFrustumPlanesBuffer);
        m_pDeviceContext->CSSetUnorderedAccessViews(0, 1, &m_pIndirectArgsUAV, nullptr);
        m_pDeviceContext->CSSetUnorderedAccessViews(1, 1, &m_pObjectsIdsUAV, nullptr);
        m_pDeviceContext->CSSetShaderResources(0, 1, &m_pInstanceDataSRV);

        m_pDeviceContext->Dispatch((MaxInst + 63) / 64, 1, 1);

        auto args = ReadUintBufferData(m_pDeviceContext, m_pIndirectArgsBuffer, 2);
        m_visibleCubes = args[1];

        if (m_visibleCubes > 0)
        {
            auto visibleIds = ReadUintBufferData(m_pDeviceContext, m_pObjectsIdsBuffer, m_visibleCubes);

            std::vector<InstanceData> visibleInstances(m_visibleCubes);
            for (UINT i = 0; i < m_visibleCubes; i++)
            {
                UINT id = visibleIds[i];
                visibleInstances[i].model = XMMatrixTranspose(m_modelInstances[id].model);
                visibleInstances[i].texInd = m_modelInstances[id].texInd;
            }

            m_pDeviceContext->UpdateSubresource(
                m_pModelBufferInst,
                0,
                nullptr,
                visibleInstances.data(),
                0,
                sizeof(InstanceData) * m_visibleCubes
            );
        }

        m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pModelBufferInst);
        m_pDeviceContext->DrawIndexedInstancedIndirect(m_pIndirectArgsBuffer, 0);

        ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr, nullptr };
        m_pDeviceContext->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
        ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
        m_pDeviceContext->CSSetShaderResources(0, 1, nullSRVs);
        m_pDeviceContext->CSSetShader(nullptr, nullptr, 0);
    }
    else
    {
        // CPU frustum culling
        std::vector<InstanceData> visibleInstances;
        visibleInstances.reserve(m_modelInstances.size());
        m_visibleCubes = 0;

        for (int i = 0; i < m_modelInstances.size(); i++) {
            XMFLOAT3 position;
            XMStoreFloat3(&position, m_modelInstances[i].model.r[3]);

            m_modelInstances[i].model = XMMatrixScaling(m_fixedScale, m_fixedScale, m_fixedScale) *
                XMMatrixRotationY(m_CubeAngle) *
                XMMatrixTranslation(position.x, position.y, position.z);

            float size = m_fixedScale * 0.95f;
            if (IsAABBInFrustum(position, size))
            {
                InstanceData data;
                data.model = XMMatrixTranspose(m_modelInstances[i].model);
                data.texInd = m_modelInstances[i].texInd;
                visibleInstances.push_back(data);
                m_visibleCubes++;
            }
        }

        if (!visibleInstances.empty())
        {
            m_pDeviceContext->UpdateSubresource(
                m_pModelBufferInst,
                0,
                nullptr,
                visibleInstances.data(),
                0,
                sizeof(InstanceData) * m_visibleCubes
            );

            m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pModelBufferInst);
            m_pDeviceContext->DrawIndexedInstanced(36, visibleInstances.size(), 0, 0, 0);
        }
    }


    static float orbitLight = 0.0f;
    orbitLight += 0.01f;
    if (orbitLight > XM_2PI) orbitLight -= XM_2PI;
    PointLight lights[3];
    float radius = 2.0f;
    lights[0].Position = XMFLOAT3(0.0f, radius * cosf(orbitLight), radius * sinf(-orbitLight));
    lights[0].Range = 3.0f;
    lights[0].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    lights[0].Intensity = 1.0f;

    lights[1].Position = XMFLOAT3(radius * cosf(orbitLight), 0.0f, radius * sinf(orbitLight));
    lights[1].Range = 3.0f;
    lights[1].Color = XMFLOAT3(1.0f, 1.0f, 0.13f);
    lights[1].Intensity = 1.0f;

    radius = 8.0f;
    lights[2].Position = XMFLOAT3(radius * cosf(orbitLight), 0.0f, radius * sinf(-orbitLight));
    lights[2].Range = 5.0f;
    lights[2].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    lights[2].Intensity = 1.0f;

    D3D11_MAPPED_SUBRESOURCE mappedResourceLight;
    hr = m_pDeviceContext->Map(m_pLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceLight);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResourceLight.pData, lights, sizeof(PointLight) * 3);
        m_pDeviceContext->Unmap(m_pLightBuffer, 0);
    }
    m_pDeviceContext->PSSetConstantBuffers(2, 1, &m_pLightBuffer);

    m_pDeviceContext->PSSetShader(m_pLightPixelShader, nullptr, 0);
    for (int i = 0; i < 3; i++)
    {
        XMMATRIX lightModel = XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixTranslation(lights[i].Position.x, lights[i].Position.y, lights[i].Position.z);
        XMMATRIX lightModelT = XMMatrixTranspose(lightModel);

        XMFLOAT4X4 lightModelTStored;
        XMStoreFloat4x4(&lightModelTStored, lightModelT);

        m_pDeviceContext->UpdateSubresource(m_pModelBufferInst, 0, nullptr, &lightModelTStored, 0, 0);
        m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pModelBufferInst);

        XMFLOAT4 lightColor = XMFLOAT4(lights[i].Color.x, lights[i].Color.y, lights[i].Color.z, 1.0f);
        m_pDeviceContext->UpdateSubresource(m_pColorBuffer, 0, nullptr, &lightColor, 0, 0);

        m_pDeviceContext->DrawIndexed(36, 0, 0);
    }
}
void RenderClass::RenderParallelogram(XMVECTOR eyePos)
{
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = false;

    ID3D11RasterizerState* pRSState = nullptr;
    m_pDevice->CreateRasterizerState(&rsDesc, &pRSState);
    m_pDeviceContext->RSSetState(pRSState);

    m_pDeviceContext->OMSetDepthStencilState(m_pStateParallelogram, 0);
    m_pDeviceContext->OMSetBlendState(m_pBlendState, nullptr, 0xFFFFFFFF);


    UINT stride = sizeof(ParallelogramVertex);
    UINT offset = 0;
    m_pDeviceContext->IASetVertexBuffers(0, 1, &m_ParallelogramVertexBuffer, &stride, &offset);
    m_pDeviceContext->IASetIndexBuffer(m_pParallelogramIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->IASetInputLayout(m_pParallelogramLayout);

    m_pDeviceContext->VSSetShader(m_pParallelogramVS, nullptr, 0);
    m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pModelBuffer);
    m_pDeviceContext->VSSetConstantBuffers(1, 1, &m_pVPBuffer);

    m_pDeviceContext->PSSetShader(m_pParallelogramPS, nullptr, 0);
    m_pDeviceContext->PSSetConstantBuffers(0, 1, &m_pColorBuffer);
    m_pDeviceContext->PSSetConstantBuffers(2, 1, &m_pLightBuffer);

    static float angle = 0.0f;
    angle += 0.015f;

    XMMATRIX modelParallelogramRed = XMMatrixTranslation(sinf(angle) * 2.0f, -0.5f, -5.0f);
    XMMATRIX mTParallelogramRed = XMMatrixTranspose(modelParallelogramRed);
    XMFLOAT4 redColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.5f);

    XMMATRIX modelParallelogramGreen = XMMatrixTranslation(-sinf(angle) * 2.0f, -0.5f, -6.0f);
    XMMATRIX mTParallelogramGreen = XMMatrixTranspose(modelParallelogramGreen);
    XMFLOAT4 greenColor = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.5f);

    XMFLOAT3 redObjectPosition;
    XMStoreFloat3(&redObjectPosition, modelParallelogramRed.r[3]);

    XMFLOAT3 greenObjectPosition;
    XMStoreFloat3(&greenObjectPosition, modelParallelogramGreen.r[3]);

    XMVECTOR redObjPos = XMLoadFloat3(&redObjectPosition);
    XMVECTOR greenObjPos = XMLoadFloat3(&greenObjectPosition);

    float redProjection = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(redObjPos, eyePos)));
    float greenProjection = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(greenObjPos, eyePos)));

    if (redProjection >= greenProjection)
    {
        m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &mTParallelogramRed, 0, 0);
        m_pDeviceContext->UpdateSubresource(m_pColorBuffer, 0, nullptr, &redColor, 0, 0);
        m_pDeviceContext->DrawIndexed(6, 0, 0);

        m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &mTParallelogramGreen, 0, 0);
        m_pDeviceContext->UpdateSubresource(m_pColorBuffer, 0, nullptr, &greenColor, 0, 0);
        m_pDeviceContext->DrawIndexed(6, 0, 0);
    }
    else
    {
        m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &mTParallelogramGreen, 0, 0);
        m_pDeviceContext->UpdateSubresource(m_pColorBuffer, 0, nullptr, &greenColor, 0, 0);
        m_pDeviceContext->DrawIndexed(6, 0, 0);

        m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &mTParallelogramRed, 0, 0);
        m_pDeviceContext->UpdateSubresource(m_pColorBuffer, 0, nullptr, &redColor, 0, 0);
        m_pDeviceContext->DrawIndexed(6, 0, 0);
    }

    pRSState->Release();
}

void RenderClass::InitImGui(HWND hWnd) 
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(m_pDevice, m_pDeviceContext);
}

void RenderClass::RenderImGui() 
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Options");
    ImGui::Checkbox("Negative Effect", &m_useNegative);
    ImGui::End();

    ImGui::Begin("Frustum Culling Info");
    ImGui::Text("Total Cubes: %d", MaxInst);
    ImGui::Text("Visible Cubes: %d", m_visibleCubes);
    ImGui::Text("Culled Cubes: %d", MaxInst - m_visibleCubes);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

HRESULT RenderClass::ConfigureBackBuffer(UINT width, UINT height)
{
    if (m_pPostProcessTexture) m_pPostProcessTexture->Release();
    if (m_pPostProcessRTV) m_pPostProcessRTV->Release();
    if (m_pPostProcessSRV) m_pPostProcessSRV->Release();
    if (m_pRenderTargetView) m_pRenderTargetView->Release();
    if (m_pDepthView) m_pDepthView->Release();

    m_pPostProcessTexture = nullptr;
    m_pPostProcessRTV = nullptr;
    m_pPostProcessSRV = nullptr;
    m_pRenderTargetView = nullptr;
    m_pDepthView = nullptr;

    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) return hr;

    hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D32_FLOAT;
    descDepth.SampleDesc.Count = 1;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* pDepthStencil = nullptr;
    hr = m_pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);
    if (FAILED(hr)) return hr;

    hr = m_pDevice->CreateDepthStencilView(pDepthStencil, nullptr, &m_pDepthView);
    pDepthStencil->Release();
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = m_pDevice->CreateTexture2D(&texDesc, nullptr, &m_pPostProcessTexture);
    if (FAILED(hr)) return hr;

    hr = m_pDevice->CreateRenderTargetView(m_pPostProcessTexture, nullptr, &m_pPostProcessRTV);
    if (FAILED(hr)) return hr;

    hr = m_pDevice->CreateShaderResourceView(m_pPostProcessTexture, nullptr, &m_pPostProcessSRV);
    if (FAILED(hr)) return hr;

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pDeviceContext->RSSetViewports(1, &vp);

    return S_OK;
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
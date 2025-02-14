#include "framework.h"
#include "RenderClass.h"

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
    COLORREF color;
};

HRESULT RenderClass::Init(HWND hWnd)
{
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
        result = ConfigureBackBuffer();
    }

    if (SUCCEEDED(result))
    {
        result = InitBufferShader();
    }

    pSelectedAdapter->Release();
    pFactory->Release();

    if (FAILED(result))
    {
        Terminate();
    }

    return result;
}

void RenderClass::Terminate()
{
    TerminateBufferShader();

    if (m_pDeviceContext)
    {
        m_pDeviceContext->ClearState();
        m_pDeviceContext->Release();
    }

    if (m_pRenderTargetView)
        m_pRenderTargetView->Release();

    if (m_pSwapChain)
        m_pSwapChain->Release();

    if (m_pDevice)
        m_pDevice->Release();
}

HRESULT RenderClass::InitBufferShader()
{
    static const Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, RGB(255,0,0)},
        { 0.5f, -0.5f, 0.0f, RGB(0,255,0)},
        {-0.5f, -0.5f, 0.0f, RGB(0,0,255)}
    };
    static const USHORT indices[] = {2, 0, 1};

    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT result = S_OK;

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &vertices;
        data.SysMemPitch = sizeof(vertices);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
        
        if (FAILED(result))
        {
            return result;
        }
        
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &indices;
        data.SysMemPitch = sizeof(indices);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
        if (FAILED(result))
        {
            return result;
        }
    }

    ID3DBlob* pVertexCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileShader(L"ColorVertex.vs", &pVertexCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileShader(L"ColorPixel.ps");
    }

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(InputDesc, 2, pVertexCode->GetBufferPointer(), pVertexCode->GetBufferSize(), &m_pLayout);
    }

    if (pVertexCode)
        pVertexCode->Release();

    return result;
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

HRESULT RenderClass::CompileShader(const std::wstring& path, ID3DBlob** pCodeShader)
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
        if (extension == L"vs")
        {
            result = m_pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &m_pVertexShader);
            if (FAILED(result))
            {
                pCode->Release();
                return result;
            }
        }
        else if (extension == L"ps")
        {
            result = m_pDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &m_pPixelShader);
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

void RenderClass::Render()
{
    float BackColor[4] = { 0.48f, 0.57f, 0.48f, 1.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, BackColor);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->IASetInputLayout(m_pLayout);

    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    m_pDeviceContext->Draw(3, 0);
    m_pSwapChain->Present(0, 0);
}

HRESULT RenderClass::ConfigureBackBuffer()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();
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

    if (m_pSwapChain)
    {
        HRESULT hr;

        RECT rc;
        GetClientRect(hWnd, &rc);
        UINT width = rc.right - rc.left;
        UINT height = rc.bottom - rc.top;

        // Resize the swap chain
        hr = m_pSwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"ResizeBuffers failed.", L"Error", MB_OK);
            return;
        }


        HRESULT resultBack = ConfigureBackBuffer();
        if (FAILED(resultBack))
        {
            MessageBox(nullptr, L"Configure back buffer failed.", L"Error", MB_OK);
            return;
        }

        m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);

        // Setup the viewport
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



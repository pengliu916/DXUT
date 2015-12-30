#pragma once

#include "stdafx.h"

#include "Utility.h"

using namespace DirectX;

#ifndef SUB_TEXTUREWIDTH
#define SUB_TEXTUREWIDTH 640
#endif

#ifndef SUB_TEXTUREHEIGHT
#define SUB_TEXTUREHEIGHT 480
#endif

using namespace std;
using namespace Microsoft::WRL;

typedef XMFLOAT2 float2;
typedef XMFLOAT3 float3;
typedef XMFLOAT4 float4;

class DrosteEffect
{
public:
    struct constBuffer
    {
        float2      inputTexReso;
        float2      outputTexReso;
        float2      logmapReso;
        float2      center;
        float       separate;
        float       phase;
        float       scale;
        float       direction;
    };

    D3D11_VIEWPORT					    m_RTviewport;
    ComPtr<ID3D11SamplerState>			m_pDrosteEffectSS;
    ComPtr<ID3D11VertexShader>			m_pScreenQuadVS;
    ComPtr<ID3D11PixelShader>			m_pDrosteEffectPS;
    ComPtr<ID3D11PixelShader>			m_pDrosteEffectIntermediatePS;
    ComPtr<ID3D11InputLayout>			m_pScreenQuadIL;
    ComPtr<ID3D11Buffer>				m_pScreenQuadVB;

    // Flag Texture
    ComPtr<ID3D11ShaderResourceView>	m_pInputTexSRV;
    ComPtr<ID3D11Resource>              m_pInputTex;

    // For Texture output
    ComPtr<ID3D11Texture2D>				m_pOutputTexture2D;
    ComPtr<ID3D11RenderTargetView>		m_pOutputTextureRTV;
    ComPtr<ID3D11ShaderResourceView>	m_pOutputTextureSRV;

    // For Intermediate output
    ComPtr<ID3D11Texture2D>				m_pIntermediateTexture2D;
    ComPtr<ID3D11RenderTargetView>		m_pIntermediateTextureRTV;
    ComPtr<ID3D11ShaderResourceView>	m_pIntermediateTextureSRV;
    D3D11_VIEWPORT                      m_IntermediateRTviewport;

    ComPtr<ID3D11RasterizerState>		m_pSolidRS;

    constBuffer                         m_ConstBuffer;
    ComPtr<ID3D11Buffer>                m_pConstBuffer;

    DrosteEffect( UINT width = SUB_TEXTUREWIDTH, UINT height = SUB_TEXTUREHEIGHT )
    {
        m_ConstBuffer.center = float2( 0.f, 0.f );
        m_ConstBuffer.scale = -3.f;
        m_ConstBuffer.phase = 1.f;
        m_ConstBuffer.separate = 40.f;
        m_ConstBuffer.logmapReso = float2( 8.f, 8.f );
        m_ConstBuffer.direction = 1.f;
    }

    HRESULT RecompilePS()
    {
        HRESULT hr;
        auto pd3dDevice = DXUTGetD3D11Device();

        ID3DBlob* pPSBlob = NULL;
        V_RETURN( CompileShaderFromFile( L"DrosteEffect.fx", "PS", "ps_5_0", &pPSBlob ) );
        V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, m_pDrosteEffectPS.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pDrosteEffectPS, "m_pDrosteEffectPS" );
        V_RETURN( CompileShaderFromFile( L"DrosteEffect.fx", "intermPS", "ps_5_0", &pPSBlob ) );
        V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, m_pDrosteEffectIntermediatePS.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pDrosteEffectIntermediatePS, "m_pDrosteEffectIntermediatePS" );
        pPSBlob->Release();
        return S_OK;
    }

    HRESULT CreateResource( ID3D11Device* pd3dDevice )
    {
        HRESULT hr = S_OK;

        ID3DBlob* pVSBlob = NULL;

        D3D11_INPUT_ELEMENT_DESC layout[] = { { "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, };

        V_RETURN( CompileShaderFromFile( L"DrosteEffect.fx", "VS", "vs_5_0", &pVSBlob ) );
        V_RETURN( pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, m_pScreenQuadVS.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pScreenQuadVS, "m_pScreenQuadVS" );
        V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), m_pScreenQuadIL.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pScreenQuadIL, "m_pScreenQuadIL" );

        pVSBlob->Release();

        ID3DBlob* pPSBlob = NULL;
        V_RETURN( CompileShaderFromFile( L"DrosteEffect.fx", "PS", "ps_5_0", &pPSBlob ) );
        V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, m_pDrosteEffectPS.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pDrosteEffectPS, "m_pDrosteEffectPS" );
        V_RETURN( CompileShaderFromFile( L"DrosteEffect.fx", "intermPS", "ps_5_0", &pPSBlob ) );
        V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, m_pDrosteEffectIntermediatePS.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pDrosteEffectIntermediatePS, "m_pDrosteEffectIntermediatePS" );
        pPSBlob->Release();

        // Load input texture
        V_RETURN( CreateWICTextureFromFile( pd3dDevice, nullptr, L"IMG_5287.png", m_pInputTex.ReleaseAndGetAddressOf(), m_pInputTexSRV.ReleaseAndGetAddressOf() ) );
        ComPtr<ID3D11Texture2D> pTextureInterface;
        m_pInputTex.Get()->QueryInterface<ID3D11Texture2D>( &pTextureInterface );

        D3D11_TEXTURE2D_DESC desc;
        pTextureInterface->GetDesc( &desc );

        m_ConstBuffer.inputTexReso = float2( ( float ) desc.Width, ( float ) desc.Height );
        m_ConstBuffer.outputTexReso = float2( ( float ) desc.Width, ( float ) desc.Height );

        D3D11_BUFFER_DESC bd =
        {
            sizeof( constBuffer ),
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_CONSTANT_BUFFER,
            0,
            0
        };

        V_RETURN( pd3dDevice->CreateBuffer( &bd, NULL, m_pConstBuffer.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pConstBuffer, "m_pConstBuffer" );

        D3D11_RASTERIZER_DESC RasterDesc;
        ZeroMemory( &RasterDesc, sizeof( D3D11_RASTERIZER_DESC ) );
        RasterDesc.FillMode = D3D11_FILL_SOLID;
        RasterDesc.CullMode = D3D11_CULL_NONE;
        RasterDesc.DepthClipEnable = TRUE;
        V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, m_pSolidRS.ReleaseAndGetAddressOf() ) );
        DXUT_SetDebugName( m_pSolidRS, "Solid" );


        //Create the sample state
        D3D11_SAMPLER_DESC sampDesc;
        ZeroMemory( &sampDesc, sizeof( sampDesc ) );
        sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        V_RETURN( pd3dDevice->CreateSamplerState( &sampDesc, m_pDrosteEffectSS.ReleaseAndGetAddressOf() ) );

        m_RTviewport.Width = ( float ) m_ConstBuffer.inputTexReso.x;
        m_RTviewport.Height = ( float ) m_ConstBuffer.inputTexReso.y;
        m_RTviewport.MinDepth = 0.0f;
        m_RTviewport.MaxDepth = 1.0f;
        m_RTviewport.TopLeftX = 0;
        m_RTviewport.TopLeftY = 0;

        ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
        pd3dImmediateContext->UpdateSubresource( m_pConstBuffer.Get(), 0, NULL, &m_ConstBuffer, 0, 0 );
        pd3dImmediateContext->RSSetState( m_pSolidRS.Get() );
        return hr;
    }

    HRESULT Resize( ID3D11Device* pd3dDevice, int Width, int Height )
    {
        HRESULT hr;

        //Create rendertaget resource
        D3D11_TEXTURE2D_DESC	RTtextureDesc = { 0 };
        RTtextureDesc.Width = Width;
        RTtextureDesc.Height = Height;
        RTtextureDesc.MipLevels = 1;
        RTtextureDesc.ArraySize = 1;
        RTtextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        RTtextureDesc.SampleDesc.Count = 1;
        RTtextureDesc.Usage = D3D11_USAGE_DEFAULT;
        RTtextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        RTtextureDesc.CPUAccessFlags = 0;
        RTtextureDesc.MiscFlags = 0;

        V_RETURN( pd3dDevice->CreateTexture2D( &RTtextureDesc, NULL, m_pOutputTexture2D.ReleaseAndGetAddressOf() ) );
        RTtextureDesc.Width = ( UINT ) ( Height );
        V_RETURN( pd3dDevice->CreateTexture2D( &RTtextureDesc, NULL, m_pIntermediateTexture2D.ReleaseAndGetAddressOf() ) );

        D3D11_SHADER_RESOURCE_VIEW_DESC RTshaderResourceDesc;
        RTshaderResourceDesc.Format = RTtextureDesc.Format;
        RTshaderResourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        RTshaderResourceDesc.Texture2D.MostDetailedMip = 0;
        RTshaderResourceDesc.Texture2D.MipLevels = 1;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pOutputTexture2D.Get(), &RTshaderResourceDesc, m_pOutputTextureSRV.ReleaseAndGetAddressOf() ) );
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pIntermediateTexture2D.Get(), &RTshaderResourceDesc, m_pIntermediateTextureSRV.ReleaseAndGetAddressOf() ) );

        D3D11_RENDER_TARGET_VIEW_DESC	RTviewDesc;
        RTviewDesc.Format = RTtextureDesc.Format;
        RTviewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        RTviewDesc.Texture2D.MipSlice = 0;
        V_RETURN( pd3dDevice->CreateRenderTargetView( m_pOutputTexture2D.Get(), &RTviewDesc, m_pOutputTextureRTV.ReleaseAndGetAddressOf() ) );
        V_RETURN( pd3dDevice->CreateRenderTargetView( m_pIntermediateTexture2D.Get(), &RTviewDesc, m_pIntermediateTextureRTV.ReleaseAndGetAddressOf() ) );

        m_RTviewport.Width = m_ConstBuffer.inputTexReso.x = ( float ) Width;
        m_RTviewport.Height = m_ConstBuffer.inputTexReso.y = ( float ) Height;

        m_IntermediateRTviewport.Width = ( float ) Height;
        m_IntermediateRTviewport.Height = ( float ) Height;

        return S_OK;
    }

    void Render( ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime )
    {
        float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        pd3dImmediateContext->OMSetRenderTargets( 1, m_pOutputTextureRTV.GetAddressOf(), nullptr );
        pd3dImmediateContext->ClearRenderTargetView( m_pOutputTextureRTV.Get(), ClearColor );
        pd3dImmediateContext->UpdateSubresource( m_pConstBuffer.Get(), 0, NULL, &m_ConstBuffer, 0, 0 );

        UINT Stride = 0;
        UINT Offset = 0;
        pd3dImmediateContext->IASetVertexBuffers( 0, 1, m_pScreenQuadVB.GetAddressOf(), &Stride, &Offset );
        pd3dImmediateContext->IASetInputLayout( m_pScreenQuadIL.Get() );
        pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        pd3dImmediateContext->VSSetShader( m_pScreenQuadVS.Get(), NULL, 0 );
        pd3dImmediateContext->GSSetShader( nullptr, NULL, 0 );
        pd3dImmediateContext->PSSetShader( m_pDrosteEffectPS.Get(), NULL, 0 );
        pd3dImmediateContext->VSSetConstantBuffers( 0, 1, m_pConstBuffer.GetAddressOf() );
        pd3dImmediateContext->PSSetSamplers( 0, 1, m_pDrosteEffectSS.GetAddressOf() );
        pd3dImmediateContext->PSSetShaderResources( 0, 1, m_pInputTexSRV.GetAddressOf() );
        pd3dImmediateContext->PSSetConstantBuffers( 0, 1, m_pConstBuffer.GetAddressOf() );
        pd3dImmediateContext->RSSetViewports( 1, &m_RTviewport );
        pd3dImmediateContext->RSSetState( m_pSolidRS.Get() );
        pd3dImmediateContext->Draw( 4, 0 );

        pd3dImmediateContext->OMSetRenderTargets( 1, m_pIntermediateTextureRTV.GetAddressOf(), nullptr );
        pd3dImmediateContext->ClearRenderTargetView( m_pIntermediateTextureRTV.Get(), ClearColor );
        pd3dImmediateContext->PSSetShader( m_pDrosteEffectIntermediatePS.Get(), NULL, 0 );
        pd3dImmediateContext->RSSetViewports( 1, &m_IntermediateRTviewport );
        pd3dImmediateContext->Draw( 4, 0 );

    }

    LRESULT HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        switch ( uMsg )
        {
        case WM_KEYDOWN:
            {
                int nKey = static_cast< int >( wParam );
                if ( nKey == 'R' )
                {
                    RecompilePS();
                }
                break;
            }
        }
        return 0;
    }
};
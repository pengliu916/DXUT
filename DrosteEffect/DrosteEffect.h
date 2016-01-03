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
    ComPtr<ID3D11ShaderResourceView>	m_pInputTexArraySRV;
    ComPtr<ID3D11ShaderResourceView>	m_pInputTexSRV;
    ComPtr<ID3D11Resource>              m_pInputTex;

    // For Texture output
    ComPtr<ID3D11Texture2D>				m_pOutputTexture2D;
    ComPtr<ID3D11RenderTargetView>		m_pOutputTextureRTV;
    ComPtr<ID3D11ShaderResourceView>	m_pOutputTextureSRV;

    // For high reso output
    ComPtr<ID3D11Texture2D>				m_pOutputHighResoTexture2D;
    ComPtr<ID3D11RenderTargetView>		m_pOutputHighResoTextureRTV;
    D3D11_VIEWPORT                      m_HighResoRTviewport;

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
    HRESULT CreateTexture2DArraySRV( std::vector<std::wstring>& filenames )
    {
        HRESULT hr;
        //
        // Load the texture elements individually from file.  These textures
        // won't be used by the GPU (0 bind flags), they are just used to 
        // load the image data from file.  We use the STAGING usage so the
        // CPU can read the resource.
        //
        UINT size = filenames.size();
        std::vector<ComPtr<ID3D11Texture2D>> srcTex( size );
        auto device = DXUTGetD3D11Device();
        for ( UINT i = 0; i < size; ++i )
        {
            /*V_RETURN( CreateDDSTextureFromFileEx( device, filenames[i].c_str(), 0u, D3D11_USAGE_STAGING, 0, 
                                                  D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 0, false, 
                                                  ( ID3D11Resource** ) srcTex[i].ReleaseAndGetAddressOf(), nullptr, nullptr ) );*/
            V_RETURN( CreateWICTextureFromFileEx( device, nullptr, filenames[i].c_str(), 0u, D3D11_USAGE_STAGING, 0,
                                                D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 0, false,
                                                ( ID3D11Resource** ) srcTex[i].ReleaseAndGetAddressOf(), nullptr ) );

        }

        //
        // Create the texture array.  Each element in the texture 
        // array has the same format/dimensions.
        //
        D3D11_TEXTURE2D_DESC texElementDesc;
        srcTex[0]->GetDesc( &texElementDesc );
        D3D11_TEXTURE2D_DESC texArrayDesc;
        texArrayDesc.Width = texElementDesc.Width;
        texArrayDesc.Height = texElementDesc.Height;
        texArrayDesc.MipLevels = texElementDesc.MipLevels;
        texArrayDesc.ArraySize = size;
        texArrayDesc.Format = texElementDesc.Format;
        texArrayDesc.SampleDesc.Count = 1;
        texArrayDesc.SampleDesc.Quality = 0;
        texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
        texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texArrayDesc.CPUAccessFlags = 0;
        texArrayDesc.MiscFlags = 0;

        ComPtr<ID3D11Texture2D> texArray = 0;
        V_RETURN( device->CreateTexture2D( &texArrayDesc, 0, &texArray ) );

        //
        // Copy individual texture elements into texture array.
        //
        auto context = DXUTGetD3D11DeviceContext();

        // for each texture element...
        for ( UINT texElement = 0; texElement < size; ++texElement )
        {
            // for each mipmap level...
            for ( UINT mipLevel = 0; mipLevel < texElementDesc.MipLevels; ++mipLevel )
            {
                D3D11_MAPPED_SUBRESOURCE mappedTex2D;
                V_RETURN( context->Map( srcTex[texElement].Get(), mipLevel, D3D11_MAP_READ, 0, &mappedTex2D ) );
                context->UpdateSubresource( texArray.Get(),
                                            D3D11CalcSubresource( mipLevel, texElement, texElementDesc.MipLevels ),
                                            0, mappedTex2D.pData, mappedTex2D.RowPitch, mappedTex2D.DepthPitch );
                context->Unmap( srcTex[texElement].Get(), mipLevel );
            }
        }
        //
        // Create a resource view to the texture array.
        //
        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.Format = texArrayDesc.Format;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        viewDesc.Texture2DArray.MostDetailedMip = 0;
        viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
        viewDesc.Texture2DArray.FirstArraySlice = 0;
        viewDesc.Texture2DArray.ArraySize = size;
        V_RETURN( device->CreateShaderResourceView( texArray.Get(), &viewDesc, m_pInputTexArraySRV.ReleaseAndGetAddressOf() ) );

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
        vector<wstring> filenames;
        filenames.push_back( L"img0.png" );
        filenames.push_back( L"img1.png" );
        filenames.push_back( L"img2.png" );
        filenames.push_back( L"img3.png" );
        filenames.push_back( L"img4.png" );

        V_RETURN( CreateWICTextureFromFile( pd3dDevice, nullptr, filenames[0].c_str(), m_pInputTex.ReleaseAndGetAddressOf(), m_pInputTexSRV.ReleaseAndGetAddressOf() ) );
        V_RETURN( CreateTexture2DArraySRV( filenames ) );

        ComPtr<ID3D11Texture2D> pTextureInterface;
        m_pInputTex.Get()->QueryInterface<ID3D11Texture2D>( &pTextureInterface );

        D3D11_TEXTURE2D_DESC desc;
        pTextureInterface->GetDesc( &desc );

        m_ConstBuffer.inputTexReso = float2( ( float ) desc.Width, ( float ) desc.Height );
        m_ConstBuffer.outputTexReso = float2( ( float ) desc.Width, ( float ) desc.Height );

        //Create rendertaget resource
        D3D11_TEXTURE2D_DESC	RTtextureDesc = { 0 };
        RTtextureDesc.Width = desc.Width;
        RTtextureDesc.Height = desc.Height;
        RTtextureDesc.MipLevels = 1;
        RTtextureDesc.ArraySize = 1;
        RTtextureDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
        RTtextureDesc.SampleDesc.Count = 1;
        RTtextureDesc.Usage = D3D11_USAGE_DEFAULT;
        RTtextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        RTtextureDesc.CPUAccessFlags = 0;
        RTtextureDesc.MiscFlags = 0;

        V_RETURN( pd3dDevice->CreateTexture2D( &RTtextureDesc, NULL, m_pOutputHighResoTexture2D.ReleaseAndGetAddressOf() ) );

        D3D11_RENDER_TARGET_VIEW_DESC	RTviewDesc;
        RTviewDesc.Format = RTtextureDesc.Format;
        RTviewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        RTviewDesc.Texture2D.MipSlice = 0;
        V_RETURN( pd3dDevice->CreateRenderTargetView( m_pOutputHighResoTexture2D.Get(), &RTviewDesc, m_pOutputHighResoTextureRTV.ReleaseAndGetAddressOf() ) );

        m_HighResoRTviewport.Width = ( float ) desc.Width;
        m_HighResoRTviewport.Height = ( float ) desc.Height;
        m_HighResoRTviewport.MinDepth = 0.0f;
        m_HighResoRTviewport.MaxDepth = 1.0f;
        m_HighResoRTviewport.TopLeftX = 0;
        m_HighResoRTviewport.TopLeftY = 0;

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

    void SaveHighResoImgToFile( wstring filename )
    {
        auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
        // Draw to high reso texture
        float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        pd3dImmediateContext->OMSetRenderTargets( 1, m_pOutputHighResoTextureRTV.GetAddressOf(), nullptr );
        pd3dImmediateContext->ClearRenderTargetView( m_pOutputHighResoTextureRTV.Get(), ClearColor );
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
        pd3dImmediateContext->PSSetShaderResources( 0, 1, m_pInputTexArraySRV.GetAddressOf() );
        pd3dImmediateContext->PSSetConstantBuffers( 0, 1, m_pConstBuffer.GetAddressOf() );
        pd3dImmediateContext->RSSetViewports( 1, &m_HighResoRTviewport );
        pd3dImmediateContext->RSSetState( m_pSolidRS.Get() );
        pd3dImmediateContext->Draw( 4, 0 );

        // Save to file
        SaveWICTextureToFile( DXUTGetD3D11DeviceContext(), ( ID3D11Resource* ) m_pOutputHighResoTexture2D.Get(),
                              GUID_ContainerFormatJpeg, filename.c_str() );
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
        RTtextureDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
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
        pd3dImmediateContext->PSSetShaderResources( 0, 1, m_pInputTexArraySRV.GetAddressOf() );
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
#pragma once

#include "stdafx.h"

#include "Utility.h"
#include "NormalGenerator.h"

using namespace DirectX;

#ifndef SUB_TEXTUREWIDTH
#define SUB_TEXTUREWIDTH 640
#endif

#ifndef SUB_TEXTUREHEIGHT
#define SUB_TEXTUREHEIGHT 480
#endif

#define ANCHOR_PARTICLE		1
#define FREE_PARTICLE		2
using namespace std;

typedef XMFLOAT2 float2;
typedef XMFLOAT3 float3;
typedef XMFLOAT4 float4;




struct Particle {
	unsigned int State;
	float3 Position;
};

struct CS_cb_perFrame{
	XMMATRIX	mObject;
	XMMATRIX	mWorld;
	XMMATRIX	mViewProj;
};

struct CS_cb_perCall{
	int			iWidth;
	int			iHeight;
	float		fK_length;
	float		fK_angle;

	float		fB;
	float3		vG;

	float		fL0;
	float		fKair;
	float		fInvM;
	float		fDt;

	float3		fWindVel;
	float		fGlobalTime;

	float3		vSpherePos;
	float		fSphereRadius;
};

class Cloth
{
public:
	NormalGenerator*				m_pNormalGenerator;
	D3D11_VIEWPORT					m_RTviewport;
	CModelViewerCamera				m_Camera;
	ID3D11SamplerState*				m_pGeneralTexSS;


	ID3D11VertexShader*				m_pClothRenderVS;
	ID3D11PixelShader*				m_pClothRenderPS;
	ID3D11GeometryShader*			m_pClothRenderGS;
	ID3D11InputLayout*				m_pClothRenderIL;
	ID3D11Buffer*					m_pClothVB; 
	ID3D11Buffer*					m_pClothRenderIB; 

	// Flag Texture
	ID3D11ShaderResourceView*		m_pFlagTexSRV;

	// For wind effect
	ID3D11ShaderResourceView*		m_pRandomTexSRV;
	ID3D11Texture1D*				m_pRandomTex1D;

	// For physical simulation
	D3D11_VIEWPORT					m_SimViewport;
	ID3D11VertexShader*				m_pSimulationVS;
	ID3D11PixelShader*				m_pSimulationPS;
	ID3D11PixelShader*				m_pUpdateAnchorPS;
	ID3D11PixelShader*				m_pGetAccPS;
	ID3D11PixelShader*				m_pGetPosVelPS;
	ID3D11GeometryShader*			m_pSimulationGS;
	ID3D11InputLayout*				m_pSimulationIL;
	ID3D11Buffer*					m_pSimulationVB;

	ID3D11ShaderResourceView*		m_pClothTexSRV;

	// For sphere rendering
	ID3D11VertexShader*				m_pSphereVS;
	ID3D11PixelShader*				m_pSpherePS;
	ID3D11InputLayout*				m_pSphereIL;
	CDXUTSDKMesh					m_SphereMesh;

	// Pos Texture related obj
	ID3D11Texture2D*				m_pClothInitPosTex;
	ID3D11ShaderResourceView*		m_pClothInitPosSRV;
	ID3D11Texture2D*				m_pClothPosTex[5];
	ID3D11ShaderResourceView*		m_pClothPosSRV[5];
	ID3D11RenderTargetView*			m_pClothPosRTV[5];
	ID3D11Texture2D*				m_pClothVelTex[5];
	ID3D11ShaderResourceView*		m_pClothVelSRV[5];
	ID3D11RenderTargetView*			m_pClothVelRTV[5];
	ID3D11Texture2D*				m_pClothAccTex[5];
	ID3D11ShaderResourceView*		m_pClothAccSRV[5];
	ID3D11RenderTargetView*			m_pClothAccRTV[5];

	//For Texture output
	ID3D11Texture2D*				m_pOutputTexture2D;
	ID3D11Texture2D*				m_pOutputStencilTexture2D;
	ID3D11RenderTargetView*			m_pOutputTextureRTV;
	ID3D11ShaderResourceView*		m_pOutputTextureSRV;
	ID3D11DepthStencilView*			m_pOutputStencilView;

	ID3D11RasterizerState*			m_pRasterizerStateSolid;
	ID3D11RasterizerState*          m_pRasterizerStateWireframe;
	bool							m_bWireframe;

	CS_cb_perFrame					m_CBperFrame;
	ID3D11Buffer*					m_pCBperFrame;

	CS_cb_perCall					m_CBperCall;
	ID3D11Buffer*					m_pCBperCall;

	// Wind 
	float							m_fWindHeading;
	float							m_fWindStrength;

	// Cloth mesh setting
	int								m_iGridWidth;
	int								m_iGridHeight;
	int								m_iNumVertices;
	int								m_iNumAnchor;
	int								m_iNumRenderIndices;


	Particle*						m_pInitParticlePos; 
	int2*							m_pAnchorPoint;
	UINT							m_rendertargetWidth;
	UINT							m_rendertargetHeight;

	float							m_fClothScale;
	float							m_fDt;
	float							m_fAccumulator;
	float							m_fK_angleFactor;
	float							m_fThresholdTime;

	Cloth(UINT width = SUB_TEXTUREWIDTH, UINT height = SUB_TEXTUREHEIGHT, 
		UINT gridwidth = 161, UINT gridheight = 121, float scale=10)
	{
		m_pNormalGenerator = new NormalGenerator(gridwidth, gridheight);
		m_rendertargetWidth=width;
		m_rendertargetHeight=height;
		m_pOutputTexture2D=NULL;
		m_pOutputTextureRTV=NULL;
		m_pOutputTextureSRV=NULL;

		m_bWireframe = false;

		m_iGridHeight = gridheight;
		m_iGridWidth = gridwidth;
		m_iNumVertices = gridheight * gridwidth;
		m_iNumAnchor = 5;

		m_fClothScale = scale;

		m_fDt = 0.001;
		m_fAccumulator = 0;
		m_fThresholdTime = 0.01;
		m_fK_angleFactor = 0.1;
		m_fWindHeading = 6.2;
		m_fWindStrength = 15;

		m_pInitParticlePos = new Particle[m_iNumVertices];
		m_pAnchorPoint = new int2[m_iNumAnchor];
		/*m_pAnchorPoint[0] = int2( 0, 0 );
		m_pAnchorPoint[1] = int2( m_iGridWidth / 2 , 0 );
		m_pAnchorPoint[2] = int2((m_iGridWidth - 1), 0);
		*/		
		int intival = m_iGridHeight / (m_iNumAnchor - 1);
		for (int i = 0; i < m_iNumAnchor; ++i){
			int y = i*intival;
			if (y >= m_iGridHeight) y = m_iGridHeight - 1;
			m_pAnchorPoint[i] = int2(0, y);
		}

		m_CBperCall.iWidth = gridwidth;
		m_CBperCall.iHeight = gridheight;
		m_CBperCall.fK_length = 300000;
		m_CBperCall.fK_angle = m_CBperCall.fK_length * m_fK_angleFactor;
		m_CBperCall.fB = 1;
		m_CBperCall.vG = float3( 0, -9.8, 0 );
		m_CBperCall.fL0 = 2.0*scale / static_cast<float>(m_iGridWidth - 1);
		m_CBperCall.fKair = 90;
		m_CBperCall.fInvM = 1.0f / 1.5;

		m_CBperCall.fWindVel = float3(cos(m_fWindHeading)*m_fWindStrength, 0, sin(m_fWindHeading)*m_fWindStrength);

		m_CBperCall.vSpherePos = float3(-0.6*scale, 0.3*scale, 0);
		m_CBperCall.fSphereRadius = 0.3*scale;
	}

	HRESULT Initial()
	{
		HRESULT hr=S_OK;
		
		return hr;
	}

	HRESULT CreateClothPosTex(ID3D11Device* pd3dDeivce)
	{
		HRESULT hr;
		float* texArray = new float[4*m_iGridHeight*m_iGridWidth];
		for( int y = 0; y < m_iGridHeight; ++y ){
			for( int x = 0; x < m_iGridWidth; ++x){
				float3	pos;
				pos.x = 2.0f * x / static_cast<float>( m_iGridWidth - 1 );
				pos.y = 2.0f * y / static_cast<float>( m_iGridWidth - 1 )-0.3;
				pos.z = 0;

				texArray[ m_iGridWidth * 4 * y + x * 4 + 0 ] = pos.x*m_fClothScale;
				texArray[m_iGridWidth * 4 * y + x * 4 + 1] = pos.y*m_fClothScale;
				texArray[m_iGridWidth * 4 * y + x * 4 + 2] = pos.z*m_fClothScale;
				texArray[ m_iGridWidth * 4 * y + x * 4 + 3 ] = FREE_PARTICLE;
				for( int i = 0; i < m_iNumAnchor; ++i ){
					if( m_pAnchorPoint[i].x == x && m_pAnchorPoint[i].y == y){
						texArray[ m_iGridWidth * 4 * y + x * 4 + 3 ] = ANCHOR_PARTICLE;
						//texArray[m_iGridWidth * 4 * y + x * 4 + 1] *= 0.8;
					}
				}
			}
		}
		
		D3D11_SUBRESOURCE_DATA initialData;
		ZeroMemory(&initialData, sizeof(D3D11_SUBRESOURCE_DATA));
		initialData.pSysMem = texArray;
		initialData.SysMemPitch = m_iGridWidth * 4 * sizeof(float);
		initialData.SysMemSlicePitch = m_iGridHeight * m_iGridWidth * 4 * sizeof(float);


		D3D11_TEXTURE2D_DESC dstex;
		ZeroMemory( &dstex, sizeof(D3D11_TEXTURE2D_DESC) );
		dstex.Width = m_iGridWidth;
		dstex.Height = m_iGridHeight;
		dstex.MipLevels = 1;
		dstex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		dstex.SampleDesc.Count=1;
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		dstex.ArraySize = 1;
		V_RETURN( pd3dDeivce->CreateTexture2D( &dstex, &initialData, &m_pClothInitPosTex ) );
		DXUT_SetDebugName( m_pClothInitPosTex, "m_pClothInitPosTex");
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		V_RETURN( pd3dDeivce->CreateTexture2D( &dstex, &initialData, &m_pClothPosTex[0] ) );
		DXUT_SetDebugName( m_pClothPosTex[0], "m_pClothPosTex[0]");
		char temp[100];
		for( int i = 1; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateTexture2D( &dstex, NULL, &m_pClothPosTex[i] ) );
			sprintf_s(temp,"m_pClothPosTex[%d]",i);
			DXUT_SetDebugName( m_pClothPosTex[i], temp );
		}
		delete texArray;

		// Create the resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory( &SRVDesc, sizeof( SRVDesc ) );
		SRVDesc.Format = dstex.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = dstex.MipLevels;
		V_RETURN( pd3dDeivce->CreateShaderResourceView( m_pClothInitPosTex, &SRVDesc, &m_pClothInitPosSRV ) );
		DXUT_SetDebugName( m_pClothInitPosSRV, "m_pClothInitPosSRV");
		for( int i = 0; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateShaderResourceView( m_pClothPosTex[i], &SRVDesc, &m_pClothPosSRV[i] ) );
			sprintf_s(temp,"m_pClothPosSRV[%d]",i);
			DXUT_SetDebugName( m_pClothPosSRV[i], temp );
		}

		//Create rendertarget resource
		D3D11_RENDER_TARGET_VIEW_DESC	RTviewDesc;
		RTviewDesc.Format=dstex.Format;
		RTviewDesc.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
		RTviewDesc.Texture2D.MipSlice=0;
		for( int i = 0; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateRenderTargetView( m_pClothPosTex[i], &RTviewDesc, &m_pClothPosRTV[i] ) );
			sprintf_s(temp,"m_pClothPosRTV[%d]",i);
			DXUT_SetDebugName( m_pClothPosRTV[i], temp );
		}
	}

	HRESULT CreateClothVelTex(ID3D11Device* pd3dDeivce)
	{
		HRESULT hr;
		float* texArray = new float[4*m_iGridHeight*m_iGridWidth];
		for( int y = 0; y < m_iGridHeight; ++y ){
			for( int x = 0; x < m_iGridWidth; ++x){
				texArray[ m_iGridWidth * 4 * y + x * 4 + 0 ] = 0;
				texArray[ m_iGridWidth * 4 * y + x * 4 + 1 ] = 0;
				texArray[ m_iGridWidth * 4 * y + x * 4 + 2 ] = 0;
				texArray[ m_iGridWidth * 4 * y + x * 4 + 3 ] = 1;
			}
		}
		
		D3D11_SUBRESOURCE_DATA initialData;
		ZeroMemory(&initialData, sizeof(D3D11_SUBRESOURCE_DATA));
		initialData.pSysMem = texArray;
		initialData.SysMemPitch = m_iGridWidth * 4 * sizeof(float);
		initialData.SysMemSlicePitch = m_iGridHeight * m_iGridWidth * 4 * sizeof(float);


		D3D11_TEXTURE2D_DESC dstex;
		ZeroMemory( &dstex, sizeof(D3D11_TEXTURE2D_DESC) );
		dstex.Width = m_iGridWidth;
		dstex.Height = m_iGridHeight;
		dstex.MipLevels = 1;
		dstex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		dstex.SampleDesc.Count=1;
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		dstex.ArraySize = 1;
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		V_RETURN( pd3dDeivce->CreateTexture2D( &dstex, &initialData, &m_pClothVelTex[0] ) );
		DXUT_SetDebugName( m_pClothVelTex[0], "m_pClothVelTex[0]");
		char temp[100];
		for( int i = 1; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateTexture2D( &dstex, NULL, &m_pClothVelTex[i] ) );
			sprintf_s(temp,"m_pClothVelTex[%d]",i);
			DXUT_SetDebugName( m_pClothVelTex[i], temp );
		}
		delete texArray;

		// Create the resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory( &SRVDesc, sizeof( SRVDesc ) );
		SRVDesc.Format = dstex.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = dstex.MipLevels;
		for( int i = 0; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateShaderResourceView( m_pClothVelTex[i], &SRVDesc, &m_pClothVelSRV[i] ) );
			sprintf_s(temp,"m_pClothVelSRV[%d]",i);
			DXUT_SetDebugName( m_pClothVelSRV[i], temp );
		}

		//Create rendertaget resource
		D3D11_RENDER_TARGET_VIEW_DESC	RTviewDesc;
		RTviewDesc.Format=dstex.Format;
		RTviewDesc.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
		RTviewDesc.Texture2D.MipSlice=0;
		for( int i = 0; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateRenderTargetView( m_pClothVelTex[i], &RTviewDesc, &m_pClothVelRTV[i] ) );
			sprintf_s(temp,"m_pClothVelRTV[%d]",i);
			DXUT_SetDebugName( m_pClothVelRTV[i], temp );
		}
	}

	HRESULT CreateClothAccTex(ID3D11Device* pd3dDeivce)
	{
		HRESULT hr;
		float* texArray = new float[4*m_iGridHeight*m_iGridWidth];
		for( int y = 0; y < m_iGridHeight; ++y ){
			for( int x = 0; x < m_iGridWidth; ++x){
				texArray[ m_iGridWidth * 4 * y + x * 4 + 0 ] = 0;
				texArray[ m_iGridWidth * 4 * y + x * 4 + 1 ] = 0;
				texArray[ m_iGridWidth * 4 * y + x * 4 + 2 ] = 0;
				texArray[ m_iGridWidth * 4 * y + x * 4 + 3 ] = 1;
			}
		}
		
		D3D11_SUBRESOURCE_DATA initialData;
		ZeroMemory(&initialData, sizeof(D3D11_SUBRESOURCE_DATA));
		initialData.pSysMem = texArray;
		initialData.SysMemPitch = m_iGridWidth * 4 * sizeof(float);
		initialData.SysMemSlicePitch = m_iGridHeight * m_iGridWidth * 4 * sizeof(float);


		D3D11_TEXTURE2D_DESC dstex;
		ZeroMemory( &dstex, sizeof(D3D11_TEXTURE2D_DESC) );
		dstex.Width = m_iGridWidth;
		dstex.Height = m_iGridHeight;
		dstex.MipLevels = 1;
		dstex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		dstex.SampleDesc.Count=1;
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		dstex.ArraySize = 1;
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		V_RETURN( pd3dDeivce->CreateTexture2D( &dstex, &initialData, &m_pClothAccTex[0] ) );
		DXUT_SetDebugName( m_pClothAccTex[0], "m_pClothAccTex[0]");
		char temp[100];
		for( int i = 1; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateTexture2D( &dstex, NULL, &m_pClothAccTex[i] ) );
			sprintf_s(temp,"m_pClothAccTex[%d]",i);
			DXUT_SetDebugName( m_pClothAccTex[i], temp );
		}
		delete texArray;

		// Create the resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory( &SRVDesc, sizeof( SRVDesc ) );
		SRVDesc.Format = dstex.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = dstex.MipLevels;
		for( int i = 0; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateShaderResourceView( m_pClothAccTex[i], &SRVDesc, &m_pClothAccSRV[i] ) );
			sprintf_s(temp,"m_pClothAccSRV[%d]",i);
			DXUT_SetDebugName( m_pClothAccSRV[i], temp );
		}

		//Create rendertaget resource
		D3D11_RENDER_TARGET_VIEW_DESC	RTviewDesc;
		RTviewDesc.Format=dstex.Format;
		RTviewDesc.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
		RTviewDesc.Texture2D.MipSlice=0;
		for( int i = 0; i < 5; ++i ){
			V_RETURN( pd3dDeivce->CreateRenderTargetView( m_pClothAccTex[i], &RTviewDesc, &m_pClothAccRTV[i] ) );
			sprintf_s(temp,"m_pClothAccRTV[%d]",i);
			DXUT_SetDebugName( m_pClothAccRTV[i], temp );
		}
	}

	HRESULT CreateClothPosVB(ID3D11Device* pd3dDevice)
	{
		HRESULT hr;
		int v = 0;
		for( int y = 0; y < m_iGridHeight; ++y ){
			for( int x = 0; x < m_iGridWidth; ++x){
				float3	pos;
				pos.x = 2 * x / static_cast<float>( m_iGridWidth - 1 ) - 1;
				pos.y = 2 * y / static_cast<float>( m_iGridHeight - 1 ) - 1;
				pos.z = 0;
				m_pInitParticlePos[v].Position = pos;
				m_pInitParticlePos[v].State = FREE_PARTICLE;
				for( int i = 0; i < m_iNumAnchor; ++i ){
					if( m_pAnchorPoint[i].x == x && m_pAnchorPoint[i].y == y){
						m_pInitParticlePos[v].State = ANCHOR_PARTICLE;
					}
				}
				++v;
			}
		}
		D3D11_BUFFER_DESC bd = 
		{
			m_iNumVertices * sizeof( Particle ),
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_VERTEX_BUFFER,
			0,
			0
		};
		D3D11_SUBRESOURCE_DATA initialData;
		initialData.pSysMem = m_pInitParticlePos;
		initialData.SysMemPitch = sizeof( Particle );
		V_RETURN( pd3dDevice->CreateBuffer( &bd, &initialData, &m_pClothVB ));
		DXUT_SetDebugName( m_pClothRenderVS, "m_pClothRenderVS" );
	}

	HRESULT CreateClothRenderIB(ID3D11Device* pd3dDevice)
	{
		HRESULT hr;
		
		m_iNumRenderIndices = 2 * ( m_iGridHeight -1 ) * ( m_iGridWidth - 1 ) * 3;
		int size = m_iNumRenderIndices;
		WORD (*indices)[3] = new WORD[size / 3][3];
		// Break grid into several part can make a index buffer more friendly to vertex cache
		const int subWidth = 10;
		int n = 0;
		for( UINT x0 = 0; x0 < m_iGridWidth - 1; x0 += subWidth){
			UINT x1 = x0 + subWidth;
			for( UINT y = 0; y < m_iGridHeight - 1; ++y){
				for( UINT x = x0; ( x < m_iGridWidth - 1 ) && ( x < x1 ); ++x){
					indices[n][0] = ( y + 1 ) * m_iGridWidth + ( x + 0 );
					indices[n][1] = ( y + 0 ) * m_iGridWidth + ( x + 0 );
					indices[n][2] = ( y + 1 ) * m_iGridWidth + ( x + 1 );
					++n;
					indices[n][0] = ( y + 0 ) * m_iGridWidth + ( x + 0 );
					indices[n][1] = ( y + 0 ) * m_iGridWidth + ( x + 1 );
					indices[n][2] = ( y + 1 ) * m_iGridWidth + ( x + 1 );
					++n;
				}
			}
		}
		D3D11_BUFFER_DESC bd = 
		{
			size * sizeof( WORD ),
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_INDEX_BUFFER,
			0,
			0
		};
		D3D11_SUBRESOURCE_DATA initialData;
		initialData.pSysMem = indices;
		initialData.SysMemPitch = sizeof( WORD );
		V_RETURN( pd3dDevice->CreateBuffer( &bd, &initialData, &m_pClothRenderIB ));
		DXUT_SetDebugName( m_pClothRenderIB, "m_pClothRenderIB" );
	}

	HRESULT CreateResource( ID3D11Device* pd3dDevice )
	{
		HRESULT hr = S_OK;

		ID3DBlob* pVSBlob = NULL;
		wstring filename = L"Simulate.fx";

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TYPE", 0, DXGI_FORMAT_R32_UINT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		V_RETURN( CompileShaderFromFile( L"Render.fx", "RenderVS", "vs_5_0", &pVSBlob ) );
		V_RETURN( pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pClothRenderVS ) );
		DXUT_SetDebugName( m_pClothRenderVS, "m_pClothRenderVS" );
		V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pClothRenderIL ) );
		DXUT_SetDebugName( m_pClothRenderIL, "m_pClothRenderIL" );

		D3D11_INPUT_ELEMENT_DESC layout1[] = { { "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 } };

		V_RETURN( CompileShaderFromFile( L"Simulate.fx", "SimulateVS", "vs_5_0", &pVSBlob ) );
		V_RETURN( pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pSimulationVS ) );
		DXUT_SetDebugName( m_pSimulationVS, "m_pSimulationVS" );
		V_RETURN( pd3dDevice->CreateInputLayout( layout1, ARRAYSIZE( layout1 ), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pSimulationIL ) );
		DXUT_SetDebugName( m_pSimulationIL, "m_pSimulationIL" );

		D3D11_INPUT_ELEMENT_DESC layoutMesh[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		V_RETURN( CompileShaderFromFile( L"Render.fx", "SphereVS", "vs_5_0", &pVSBlob ) );
		V_RETURN( pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pSphereVS ) );
		V_RETURN( pd3dDevice->CreateInputLayout( layoutMesh, ARRAYSIZE( layoutMesh ), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pSphereIL ) );

		pVSBlob->Release();

		ID3DBlob* pGSBlob = NULL;
		V_RETURN( CompileShaderFromFile( L"Render.fx", "RenderGS", "gs_5_0", &pGSBlob ) );
		V_RETURN( pd3dDevice->CreateGeometryShader( pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &m_pClothRenderGS ) );
		DXUT_SetDebugName( m_pClothRenderGS, "m_pClothRenderGS" );

		V_RETURN( CompileShaderFromFile( L"Simulate.fx", "SimulateGS", "gs_5_0", &pGSBlob ) );
		V_RETURN( pd3dDevice->CreateGeometryShader( pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &m_pSimulationGS ) );
		DXUT_SetDebugName( m_pSimulationGS, "m_pSimulationGS" );
		pGSBlob->Release();

		ID3DBlob* pPSBlob = NULL;
		V_RETURN( CompileShaderFromFile( L"Render.fx", "RenderPS", "ps_5_0", &pPSBlob ) );
		V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pClothRenderPS ) );
		DXUT_SetDebugName( m_pClothRenderPS, "m_pClothRenderPS" );

		V_RETURN( CompileShaderFromFile( L"Simulate.fx", "SimulatePS", "ps_5_0", &pPSBlob ) );
		V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pSimulationPS ) );
		DXUT_SetDebugName( m_pSimulationPS, "m_pSimulationPS" );

		V_RETURN( CompileShaderFromFile( L"Simulate.fx", "UpdateAnchorPS", "ps_5_0", &pPSBlob ) );
		V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pUpdateAnchorPS ) );
		DXUT_SetDebugName( m_pUpdateAnchorPS, "m_pUpdateAnchorPS" );

		V_RETURN( CompileShaderFromFile( L"Simulate.fx", "GetAccPS", "ps_5_0", &pPSBlob ) );
		V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pGetAccPS ) );
		DXUT_SetDebugName( m_pGetAccPS, "m_pGetAccPS" );

		V_RETURN( CompileShaderFromFile( L"Simulate.fx", "GetPosVelPS", "ps_5_0", &pPSBlob ) );
		V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pGetPosVelPS ) );
		DXUT_SetDebugName( m_pGetPosVelPS, "m_pGetPosVelPS" );

		V_RETURN( CompileShaderFromFile( L"Render.fx", "SpherePS", "ps_5_0", &pPSBlob ) );
		V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pSpherePS ) );
		pPSBlob->Release();

		// Load sphere mesh
		V_RETURN( m_SphereMesh.Create( pd3dDevice, L"Ball.sdkmesh" ) );

		// Load flag texture
		V_RETURN( DXUTCreateShaderResourceViewFromFile( pd3dDevice, L"cloth.dds", &m_pFlagTexSRV ) );

		// Create cloth grid mesh
		V_RETURN( CreateClothPosTex( pd3dDevice ) );
		V_RETURN( CreateClothVelTex( pd3dDevice ) );
		V_RETURN( CreateClothAccTex( pd3dDevice ) );
		V_RETURN( CreateClothPosVB( pd3dDevice ) );
		V_RETURN( CreateClothRenderIB( pd3dDevice ) );

		V_RETURN( m_pNormalGenerator->CreateResource( pd3dDevice, m_pClothPosSRV[0] ) );

		D3D11_BUFFER_DESC bd =
		{
			sizeof( CS_cb_perFrame ),
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_CONSTANT_BUFFER,
			0,
			0
		};
		V_RETURN( pd3dDevice->CreateBuffer( &bd, NULL, &m_pCBperFrame ) );
		DXUT_SetDebugName( m_pCBperFrame, "m_pCBperFrame" );
		bd.ByteWidth = sizeof( CS_cb_perCall );
		V_RETURN( pd3dDevice->CreateBuffer( &bd, NULL, &m_pCBperCall ) );
		DXUT_SetDebugName( m_pCBperCall, "m_pCBperCall" );

		//Create rendertaget resource
		D3D11_TEXTURE2D_DESC	RTtextureDesc = { 0 };
		RTtextureDesc.Width = m_rendertargetWidth;
		RTtextureDesc.Height = m_rendertargetHeight;
		RTtextureDesc.MipLevels = 1;
		RTtextureDesc.ArraySize = 1;
		RTtextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		RTtextureDesc.SampleDesc.Count = 1;
		RTtextureDesc.Usage = D3D11_USAGE_DEFAULT;
		RTtextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		RTtextureDesc.CPUAccessFlags = 0;
		RTtextureDesc.MiscFlags = 0;

		V_RETURN( pd3dDevice->CreateTexture2D( &RTtextureDesc, NULL, &m_pOutputTexture2D ) );

		D3D11_SHADER_RESOURCE_VIEW_DESC RTshaderResourceDesc;
		RTshaderResourceDesc.Format = RTtextureDesc.Format;
		RTshaderResourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		RTshaderResourceDesc.Texture2D.MostDetailedMip = 0;
		RTshaderResourceDesc.Texture2D.MipLevels = 1;
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_pOutputTexture2D, &RTshaderResourceDesc, &m_pOutputTextureSRV ) );

		D3D11_RENDER_TARGET_VIEW_DESC	RTviewDesc;
		RTviewDesc.Format = RTtextureDesc.Format;
		RTviewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTviewDesc.Texture2D.MipSlice = 0;
		V_RETURN( pd3dDevice->CreateRenderTargetView( m_pOutputTexture2D, &RTviewDesc, &m_pOutputTextureRTV ) );

		//Create random texture resource
		int iNumRandValues = 1024;
		srand( timeGetTime() );
		//create the data
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = new float[iNumRandValues * 4];
		if ( !InitData.pSysMem )
			return E_OUTOFMEMORY;
		InitData.SysMemPitch = iNumRandValues * 4 * sizeof( float );
		InitData.SysMemSlicePitch = iNumRandValues * 4 * sizeof( float );
		for ( int i = 0; i < iNumRandValues * 4; i++ )
		{
			( ( float* ) InitData.pSysMem )[i] = float( ( rand() % 10000 ) - 5000 );
		}

		// Create the texture
		D3D11_TEXTURE1D_DESC dstex;
		dstex.Width = iNumRandValues;
		dstex.MipLevels = 1;
		dstex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		dstex.ArraySize = 1;
		V_RETURN( pd3dDevice->CreateTexture1D( &dstex, &InitData, &m_pRandomTex1D ) );
		DXUT_SetDebugName( m_pRandomTex1D, "m_pRandomTex1D" );
		SAFE_DELETE_ARRAY( InitData.pSysMem );

		// Create the resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory( &SRVDesc, sizeof( SRVDesc ) );
		SRVDesc.Format = dstex.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
		SRVDesc.Texture2D.MipLevels = dstex.MipLevels;
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_pRandomTex1D, &SRVDesc, &m_pRandomTexSRV ) );
		DXUT_SetDebugName( m_pRandomTexSRV, "m_pRandomTexSRV" );


		//Create DepthStencil buffer and view
		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory( &descDepth, sizeof( descDepth ) );
		descDepth.Width = m_rendertargetWidth;
		descDepth.Height = m_rendertargetHeight;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXUTGetDeviceSettings().d3d11.AutoDepthStencilFormat;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;
		hr = pd3dDevice->CreateTexture2D( &descDepth, NULL, &m_pOutputStencilTexture2D );


		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory( &descDSV, sizeof( descDSV ) );
		descDSV.Format = descDepth.Format;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		// Create the depth stencil view
		V_RETURN( pd3dDevice->CreateDepthStencilView( m_pOutputStencilTexture2D, // Depth stencil texture
													  &descDSV, // Depth stencil desc
													  &m_pOutputStencilView ) );  // [out] Depth stencil view

												  // Create solid and wireframe rasterizer state objects
		D3D11_RASTERIZER_DESC RasterDesc;
		ZeroMemory( &RasterDesc, sizeof( D3D11_RASTERIZER_DESC ) );
		RasterDesc.FillMode = D3D11_FILL_SOLID;
		RasterDesc.CullMode = D3D11_CULL_NONE;
		RasterDesc.DepthClipEnable = TRUE;
		V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &m_pRasterizerStateSolid ) );
		DXUT_SetDebugName( m_pRasterizerStateSolid, "Solid" );

		RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
		V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &m_pRasterizerStateWireframe ) );
		DXUT_SetDebugName( m_pRasterizerStateWireframe, "Wireframe" );

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
		V_RETURN( pd3dDevice->CreateSamplerState( &sampDesc, &m_pGeneralTexSS ) );

		m_RTviewport.Width = ( float ) m_rendertargetWidth;
		m_RTviewport.Height = ( float ) m_rendertargetHeight;
		m_RTviewport.MinDepth = 0.0f;
		m_RTviewport.MaxDepth = 1.0f;
		m_RTviewport.TopLeftX = 0;
		m_RTviewport.TopLeftY = 0;

		m_SimViewport.Width = m_iGridWidth;
		m_SimViewport.Height = m_iGridHeight;
		m_SimViewport.MinDepth = 0.0f;
		m_SimViewport.MaxDepth = 1.0f;
		m_SimViewport.TopLeftX = 0;
		m_SimViewport.TopLeftY = 0;

		XMVECTORF32 vecEye = { 0.0f, 0.0f, -10.0f };
		XMVECTORF32 vecAt={ 0.0f, 0.0f, 0.0f	};
		m_Camera.SetViewParams( vecEye, vecAt );
		m_Camera.SetRadius(5.0f*m_fClothScale, 1.0f*m_fClothScale, 20.0f*m_fClothScale);

		m_CBperFrame.mObject = XMMatrixIdentity();

		ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
		pd3dImmediateContext->UpdateSubresource( m_pCBperCall, 0, NULL, &m_CBperCall, 0, 0 );
		pd3dImmediateContext->RSSetState( m_pRasterizerStateSolid );
		//SAFE_RELEASE(pd3dImmediateContext);
		return hr;
	}
	
	void Resize()
	{
		// Setup the camera's projection parameters
			float fAspectRatio = m_rendertargetWidth / ( FLOAT )m_rendertargetHeight;
			m_Camera.SetProjParams( XM_PI / 4, fAspectRatio, 0.1f, 5000.0f );
			m_Camera.SetWindow(m_rendertargetWidth,m_rendertargetHeight );
			m_Camera.SetButtonMasks( MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
	}

	void Update( float fElapsedTime )
	{
		m_Camera.FrameMove( fElapsedTime );
	}

	void RenderScene(ID3D11DeviceContext* pd3dImmediateContext)
	{
		SDKMESH_SUBSET* pSubset = NULL;
		D3D11_PRIMITIVE_TOPOLOGY PrimType;
		UINT Strides[1];
		UINT Offsets[1];
		ID3D11Buffer* pVB[1];
		pd3dImmediateContext->IASetInputLayout(m_pSphereIL);
		pd3dImmediateContext->RSSetViewports(1, &m_RTviewport);
		pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pCBperCall);
		pd3dImmediateContext->VSSetConstantBuffers(1, 1, &m_pCBperFrame);
		pd3dImmediateContext->GSSetShader(NULL, NULL, 0);

		pVB[0] = m_SphereMesh.GetVB11(0, 0);
		Strides[0] = (UINT)m_SphereMesh.GetVertexStride(0, 0);
		Offsets[0] = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
		pd3dImmediateContext->IASetIndexBuffer(m_SphereMesh.GetIB11(0), m_SphereMesh.GetIBFormat11(0), 0);
		pd3dImmediateContext->VSSetShader(m_pSphereVS, NULL, 0);
		pd3dImmediateContext->PSSetShader(m_pSpherePS, NULL, 0);
		for (UINT subset = 0; subset < m_SphereMesh.GetNumSubsets(0); ++subset)
		{
			pSubset = m_SphereMesh.GetSubset(0, subset);
			PrimType = CDXUTSDKMesh::GetPrimitiveType11((SDKMESH_PRIMITIVE_TYPE)pSubset->PrimitiveType);
			pd3dImmediateContext->IASetPrimitiveTopology(PrimType);
			pd3dImmediateContext->DrawIndexed((UINT)pSubset->IndexCount, 0, (UINT)pSubset->VertexStart);
		}
	}

	void Render(ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime )
	{
		XMMATRIX m_Proj = m_Camera.GetProjMatrix();
		XMMATRIX m_View = m_Camera.GetViewMatrix();
		XMMATRIX m_World =m_Camera.GetWorldMatrix();

		m_CBperFrame.mWorld = XMMatrixIdentity(); 
		m_CBperFrame.mObject = XMMatrixTranspose( m_World ); 
		m_CBperFrame.mViewProj = XMMatrixTranspose( m_View*m_Proj );
		pd3dImmediateContext->UpdateSubresource( m_pCBperFrame, 0, NULL, &m_CBperFrame, 0, 0 );


		Simulate( pd3dImmediateContext, fTime, fElapsedTime );

		m_pNormalGenerator->ProcessImage(pd3dImmediateContext);


		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		pd3dImmediateContext->OMSetRenderTargets(1,&m_pOutputTextureRTV,m_pOutputStencilView);
		pd3dImmediateContext->ClearRenderTargetView( m_pOutputTextureRTV, ClearColor );
		pd3dImmediateContext->ClearDepthStencilView( m_pOutputStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		UINT Stride = sizeof( Particle );
		UINT Offset = 0;
		pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pClothVB, &Stride, &Offset );
		pd3dImmediateContext->IASetInputLayout( m_pClothRenderIL );
		pd3dImmediateContext->IASetIndexBuffer( m_pClothRenderIB, DXGI_FORMAT_R16_UINT, 0 );
		pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		pd3dImmediateContext->VSSetShader( m_pClothRenderVS, NULL, 0 );
		pd3dImmediateContext->GSSetShader( m_pClothRenderGS, NULL, 0 );
		pd3dImmediateContext->PSSetShader( m_pClothRenderPS, NULL, 0 );
		pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &m_pCBperCall );
		pd3dImmediateContext->VSSetShaderResources( 0, 1, &m_pClothPosSRV[0]);
		pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &m_pCBperFrame );
		pd3dImmediateContext->GSSetConstantBuffers(1, 1, &m_pCBperFrame);
		pd3dImmediateContext->GSSetSamplers(0, 1, &m_pGeneralTexSS);
		pd3dImmediateContext->PSSetSamplers(0, 1, &m_pGeneralTexSS);
		pd3dImmediateContext->GSSetShaderResources(1, 1, &m_pNormalGenerator->m_pOutputTextureRV);
		pd3dImmediateContext->PSSetShaderResources(2, 1, &m_pFlagTexSRV);
		pd3dImmediateContext->RSSetViewports(1, &m_RTviewport);
		if( m_bWireframe ) pd3dImmediateContext->RSSetState( m_pRasterizerStateWireframe );
		pd3dImmediateContext->DrawIndexed( m_iNumRenderIndices, 0, 0 );
		if( m_bWireframe ) pd3dImmediateContext->RSSetState( m_pRasterizerStateSolid );
		ID3D11ShaderResourceView* pSRVNULLs = NULL;
		pd3dImmediateContext->VSSetShaderResources( 0, 1, &pSRVNULLs );

		RenderScene(pd3dImmediateContext);
		
	}

	void Simulate(ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime )
	{
		m_CBperCall.fGlobalTime = 0;// *fTime;
		pd3dImmediateContext->IASetInputLayout(m_pSimulationIL);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		UINT stride = 0;
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSimulationVB, &stride, &offset);
		pd3dImmediateContext->RSSetViewports(1, &m_RTviewport);
		pd3dImmediateContext->VSSetShader( m_pSimulationVS, NULL, 0 );
		pd3dImmediateContext->GSSetShader(m_pSimulationGS,NULL,0);	
		pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCBperCall );
		pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_pCBperFrame );
		pd3dImmediateContext->PSSetShaderResources(11, 1, &m_pRandomTexSRV);
		pd3dImmediateContext->PSSetSamplers(0, 1, &m_pGeneralTexSS);
		pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &m_pCBperCall );
		pd3dImmediateContext->RSSetViewports(1, &m_SimViewport);

		if (fElapsedTime > m_fThresholdTime) fElapsedTime = m_fThresholdTime;
		m_fAccumulator += fElapsedTime;
		while (m_fAccumulator >= m_fDt){
			Integration(pd3dImmediateContext, m_fDt);
			m_fAccumulator -= m_fDt;
		}
	}

	void Integration(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
	{
		ID3D11ShaderResourceView* pSRVNULLs[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		// Get S0'(t0):V0(t0) A0(t0)
		GetAcceleration(pd3dImmediateContext, m_pClothPosSRV[0], m_pClothVelSRV[0], m_pClothAccRTV[0]);
		// Get S1(t0+.5dt):P1(t0+.5dt) V1(t0+.5dt)
		GetNewPosVel(pd3dImmediateContext, m_pClothPosSRV[0], m_pClothVelSRV[0], m_pClothAccSRV[0],
			m_pClothPosRTV[1], m_pClothVelRTV[1], fElapsedTime / 2.0f);
		// Get S1'(t0+.5dt):V1(t0+.5dt) A1(t0+.5dt)
		GetAcceleration(pd3dImmediateContext, m_pClothPosSRV[1], m_pClothVelSRV[1], m_pClothAccRTV[1]);
		// Get S2(t0+.5dt): P2(t0+.5dt) V2(t0+.5dt)
		GetNewPosVel(pd3dImmediateContext, m_pClothPosSRV[1], m_pClothVelSRV[1], m_pClothAccSRV[1],
			m_pClothPosRTV[2], m_pClothVelRTV[2], fElapsedTime / 2.0f);
		// Get S2'(t0+.5dt):V2(t0+.5dt) A2(t0+.5dt)
		GetAcceleration(pd3dImmediateContext, m_pClothPosSRV[2], m_pClothVelSRV[2], m_pClothAccRTV[2]);
		// Get S3(t0+dt): P3(t0+dt) V3(t0+dt)
		GetNewPosVel(pd3dImmediateContext, m_pClothPosSRV[2], m_pClothVelSRV[2], m_pClothAccSRV[2],
			m_pClothPosRTV[3], m_pClothVelRTV[3], fElapsedTime);
		// Get S3'(t0+dt):V3(t0+dt) A3(t0+dt)
		GetAcceleration(pd3dImmediateContext, m_pClothPosSRV[3], m_pClothVelSRV[3], m_pClothAccRTV[3]);

		// Get the final result
		ID3D11RenderTargetView* RTVs[] = { m_pClothPosRTV[4], m_pClothVelRTV[4] };
		pd3dImmediateContext->OMSetRenderTargets(2, RTVs, NULL);
		pd3dImmediateContext->PSSetShader(m_pSimulationPS, NULL, 0);
		pd3dImmediateContext->PSSetShaderResources(0, 1, &m_pClothPosSRV[0]);
		pd3dImmediateContext->PSSetShaderResources(1, 1, &m_pClothVelSRV[0]);
		pd3dImmediateContext->PSSetShaderResources(3, 4, m_pClothVelSRV);
		pd3dImmediateContext->PSSetShaderResources(7, 4, m_pClothAccSRV);
		pd3dImmediateContext->Draw(1, 0);

		pd3dImmediateContext->PSSetShaderResources(0, 11, pSRVNULLs);

		// Update Anchor Points
		ID3D11RenderTargetView* nRTVs[] = { m_pClothPosRTV[0], m_pClothVelRTV[0] };
		ID3D11ShaderResourceView* nSRVs[] = { m_pClothPosSRV[4], m_pClothVelSRV[4] };
		pd3dImmediateContext->OMSetRenderTargets(2, nRTVs, NULL);
		pd3dImmediateContext->PSSetShader(m_pUpdateAnchorPS, NULL, 0);
		pd3dImmediateContext->PSSetShaderResources(0, 2, nSRVs);
		pd3dImmediateContext->Draw(1, 0);

		pd3dImmediateContext->PSSetShaderResources(0, 2, pSRVNULLs);
	}

	void GetAcceleration(ID3D11DeviceContext* pd3dImmediateContext, 
			ID3D11ShaderResourceView* posSRV,
			ID3D11ShaderResourceView* velSRV,
			ID3D11RenderTargetView* accRTV)
	{
		ID3D11ShaderResourceView* SRVs[] = { posSRV, velSRV };
		pd3dImmediateContext->OMSetRenderTargets(1,&accRTV,NULL);
		pd3dImmediateContext->PSSetShader( m_pGetAccPS, NULL, 0 );
		pd3dImmediateContext->PSSetShaderResources(0, 2, SRVs);
		pd3dImmediateContext->Draw(1,0);	
		ID3D11ShaderResourceView* pSRVNULL[] = { NULL, NULL };
		pd3dImmediateContext->PSSetShaderResources( 0, 2, pSRVNULL );
	}

	void GetNewPosVel(ID3D11DeviceContext* pd3dImmediateContext, 
			ID3D11ShaderResourceView* posSRV,
			ID3D11ShaderResourceView* velSRV,
			ID3D11ShaderResourceView* accSRV,
			ID3D11RenderTargetView* posRTV,
			ID3D11RenderTargetView* velRTV, float deltaTime)
	{
		m_CBperCall.fDt = deltaTime;
		pd3dImmediateContext->UpdateSubresource( m_pCBperCall, 0, NULL, &m_CBperCall, 0, 0 );
		ID3D11RenderTargetView* RTVs[] = { posRTV, velRTV };
		ID3D11ShaderResourceView* SRVs[] = { posSRV, velSRV, accSRV };
		pd3dImmediateContext->OMSetRenderTargets(2,RTVs,NULL);
		pd3dImmediateContext->PSSetShader( m_pGetPosVelPS, NULL, 0 );
		pd3dImmediateContext->PSSetShaderResources(0, 3, SRVs);
		pd3dImmediateContext->Draw(1,0);	
		ID3D11ShaderResourceView* pSRVNULL[] = { NULL, NULL, NULL };
		pd3dImmediateContext->PSSetShaderResources( 0, 3, pSRVNULL );
	}

	~Cloth()
	{
		//delete m_pNormalGenerator;
	}

	void Release()
	{
		SAFE_RELEASE(m_pClothRenderVS);
		SAFE_RELEASE(m_pClothRenderGS);
		SAFE_RELEASE(m_pClothRenderPS);
		SAFE_RELEASE(m_pClothRenderIL);
		SAFE_RELEASE(m_pClothVB);
		SAFE_RELEASE(m_pClothRenderIB);

		SAFE_RELEASE(m_pSphereVS);
		SAFE_RELEASE(m_pSpherePS);
		SAFE_RELEASE(m_pSphereIL);

		m_SphereMesh.Destroy();

		SAFE_RELEASE(m_pSimulationVS);
		SAFE_RELEASE(m_pSimulationGS);
		SAFE_RELEASE(m_pSimulationPS);
		SAFE_RELEASE(m_pUpdateAnchorPS);
		SAFE_RELEASE(m_pGetPosVelPS);
		SAFE_RELEASE(m_pGetAccPS);
		SAFE_RELEASE(m_pSimulationIL);
		SAFE_RELEASE(m_pSimulationVB);

		SAFE_RELEASE(m_pOutputTexture2D);
		SAFE_RELEASE(m_pOutputTextureRTV);
		SAFE_RELEASE(m_pOutputTextureSRV);
		SAFE_RELEASE(m_pOutputStencilTexture2D);
		SAFE_RELEASE(m_pOutputStencilView);

		SAFE_RELEASE(m_pClothTexSRV);

		SAFE_RELEASE(m_pClothInitPosTex);
		SAFE_RELEASE(m_pClothInitPosSRV);

		SAFE_RELEASE(m_pRandomTex1D);
		SAFE_RELEASE(m_pRandomTexSRV);

		SAFE_RELEASE(m_pFlagTexSRV);

		SAFE_RELEASE(m_pCBperCall);
		m_pNormalGenerator->Release();
		SAFE_RELEASE(m_pNormalGenerator);
		SAFE_RELEASE(m_pGeneralTexSS);
		
		for( int i = 0; i < 5; ++i ){
			SAFE_RELEASE(m_pClothPosTex[i]);
			SAFE_RELEASE(m_pClothPosSRV[i]);
			SAFE_RELEASE(m_pClothPosRTV[i]);
			
			SAFE_RELEASE(m_pClothVelTex[i]);
			SAFE_RELEASE(m_pClothVelSRV[i]);
			SAFE_RELEASE(m_pClothVelRTV[i]);
			
			SAFE_RELEASE(m_pClothAccTex[i]);
			SAFE_RELEASE(m_pClothAccSRV[i]);
			SAFE_RELEASE(m_pClothAccRTV[i]);
		}
		SAFE_RELEASE(m_pRasterizerStateSolid);
		SAFE_RELEASE(m_pRasterizerStateWireframe);

		SAFE_RELEASE(m_pCBperFrame);
	}

	LRESULT HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );
		switch(uMsg)
		{
		case WM_KEYDOWN:
			{
				int nKey = static_cast<int>(wParam);
				float step = 0.3;
				if (nKey == 'F')
				{
					m_bWireframe = !m_bWireframe;
				}

				if (nKey == 'W')
				{
					m_CBperCall.vSpherePos.z += step;
				}
				if (nKey == 'S')
				{
					m_CBperCall.vSpherePos.z -= step;
				}
				if (nKey == 'A')
				{
					m_CBperCall.vSpherePos.x -= step;
				}
				if (nKey == 'D')
				{
					m_CBperCall.vSpherePos.x += step;
				}
				break;
			}
		}
		return 0;
	}
};
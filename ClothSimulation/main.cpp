#include "stdafx.h"
#include "Utility.h"
#include "MultiTexturePresenter.h"
#include "Cloth.h"

#define SUB_TEXTUREWIDTH 1024
#define SUB_TEXTUREHEIGHT 768

//--------------------------------------------------------------------------------------
//Global Variables
//--------------------------------------------------------------------------------------
MultiTexturePresenter						MultiTexture = MultiTexturePresenter(1,true,SUB_TEXTUREWIDTH,SUB_TEXTUREHEIGHT);
Cloth										SimCloth = Cloth(SUB_TEXTUREWIDTH,SUB_TEXTUREHEIGHT);

CDXUTDialogResourceManager					DialogResourceManager;
CDXUTDialog									UI;
//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------

#define IDC_K_LENGTH_STATIC			1
#define IDC_K_LENGTH_SLIDER			2
#define IDC_K_ANGLE_STATIC			3
#define IDC_K_ANGLE_SLIDER			4
#define IDC_DAMPING_STATIC			5
#define IDC_DAMPING_SLIDER			6
#define IDC_AIRRESIST_STATIC		7
#define IDC_AIRRESIST_SLIDER		8
#define IDC_MASS_STATIC				9
#define IDC_MASS_SLIDER				10
#define IDC_WINDHEADING_STATIC		11
#define IDC_WINDHEADING_SLIDER		12
#define IDC_WINDSTRENGTH_STATIC		13
#define IDC_WINDSTRENGTH_SLIDER		14

void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);

//--------------------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------------------
HRESULT Initial()
{ 
	HRESULT hr = S_OK;
	UI.Init(&DialogResourceManager);
	//UI.SetFont
	UI.SetCallback(OnGUIEvent); int iY = 10;

	UI.SetFont(1, L"Comic Sans MS", 400, 400);
	UI.SetFont(2, L"Courier New", 16, FW_NORMAL);

	WCHAR sz[100];
	iY += 24;
	swprintf_s(sz, 100, L"k_length: %0.2f", SimCloth.m_CBperCall.fK_length);
	UI.AddStatic(IDC_K_LENGTH_STATIC, sz, 0, iY, 170, 23);
	UI.AddSlider(IDC_K_LENGTH_SLIDER, 0, iY += 26, 170, 23, 200000, 400000, SimCloth.m_CBperCall.fK_length);

	swprintf_s(sz, 100, L"k_angle factor: %0.2f", SimCloth.m_fK_angleFactor);
	UI.AddStatic(IDC_K_ANGLE_STATIC, sz, 0, iY += 26, 170, 23);
	UI.AddSlider(IDC_K_ANGLE_SLIDER, 0, iY += 26, 170, 23, 4, 3000, 3.0f/SimCloth.m_fK_angleFactor);

	swprintf_s(sz, 100, L"Damping factor: %0.2f", SimCloth.m_CBperCall.fB);
	UI.AddStatic(IDC_DAMPING_STATIC, sz, 0, iY += 26, 170, 23);
	UI.AddSlider(IDC_DAMPING_SLIDER, 0, iY += 26, 170, 23, 0, 2000, SimCloth.m_CBperCall.fB*100);


	swprintf_s(sz, 100, L"Air Resistance: %0.2f", SimCloth.m_CBperCall.fKair);
	UI.AddStatic(IDC_AIRRESIST_STATIC, sz, 0, iY += 26, 170, 23);
	UI.AddSlider(IDC_AIRRESIST_SLIDER, 0, iY += 26, 170, 23, 50, 200, SimCloth.m_CBperCall.fKair);

	swprintf_s(sz, 100, L"Particle Mass: %0.2f", 1.0f/SimCloth.m_CBperCall.fInvM);
	UI.AddStatic(IDC_MASS_STATIC, sz, 0, iY += 26, 170, 23);
	UI.AddSlider(IDC_MASS_SLIDER, 0, iY += 26, 170, 23, 100, 600, 100.0f*1.0f / SimCloth.m_CBperCall.fInvM);

	swprintf_s(sz, 100, L"Wind Heading: %0.2f", SimCloth.m_fWindHeading/6.2832);
	UI.AddStatic(IDC_WINDHEADING_STATIC, sz, 0, iY += 26, 170, 23);
	UI.AddSlider(IDC_WINDHEADING_SLIDER, 0, iY += 26, 170, 23, 0, 360, SimCloth.m_fWindHeading*360 / 6.2832);

	swprintf_s(sz, 100, L"Wind Strength: %0.2f", SimCloth.m_fWindStrength);
	UI.AddStatic(IDC_WINDSTRENGTH_STATIC, sz, 0, iY += 26, 170, 23);
	UI.AddSlider(IDC_WINDSTRENGTH_SLIDER, 0, iY += 26, 170, 23, 0, 50, SimCloth.m_fWindStrength*2.0f);

	V_RETURN( MultiTexture.Initial() );
	return hr;
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
									   DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	MultiTexture.ModifyDeviceSettings( pDeviceSettings );
	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
									  void* pUserContext )
{
	HRESULT hr = S_OK;
	ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN( SimCloth.CreateResource( pd3dDevice ));
	V_RETURN( MultiTexture.CreateResource( pd3dDevice, SimCloth.m_pOutputTextureSRV ));
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
										  const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr;
	V_RETURN(DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	UI.SetLocation(pBackBufferSurfaceDesc->Width - 180, 0);
	UI.SetSize(180, 600);

	MultiTexture.Resize();
	SimCloth.Resize();
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	SimCloth.Update( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
								  double fTime, float fElapsedTime, void* pUserContext )
{
	// Clear render target and the depth stencil 
  /*  float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };

	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );*/
	SimCloth.Render( pd3dImmediateContext, fTime, fElapsedTime );
	MultiTexture.Render( pd3dImmediateContext );
	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR2, L"UI");
	UI.OnRender(fElapsedTime);
	DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	DialogResourceManager.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();
	SimCloth.Release();
	MultiTexture.Release();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
						  bool* pbNoFurtherProcessing, void* pUserContext )
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = UI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	SimCloth.HandleMessages( hWnd, uMsg, wParam, lParam );
	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
					   bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
					   int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
	return true;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	WCHAR sz[100];
	switch (nControlID)
	{
	case IDC_K_LENGTH_SLIDER:
		{
			float k = (float)(UI.GetSlider(IDC_K_LENGTH_SLIDER)->GetValue());
			SimCloth.m_CBperCall.fK_length = k;
			swprintf_s(sz, 100, L"k_length: %0.2f", SimCloth.m_CBperCall.fK_length);
			UI.GetStatic(IDC_K_LENGTH_STATIC)->SetText(sz);
			break;
		}

	case IDC_K_ANGLE_SLIDER:
		{
			float k = (float)(UI.GetSlider(IDC_K_ANGLE_SLIDER)->GetValue());
			SimCloth.m_fK_angleFactor = 3.0f / k;
			SimCloth.m_CBperCall.fK_angle = SimCloth.m_fK_angleFactor * SimCloth.m_CBperCall.fK_length;
			swprintf_s(sz, 100, L"k_angle factor: %0.4f", SimCloth.m_fK_angleFactor);
			UI.GetStatic(IDC_K_ANGLE_STATIC)->SetText(sz);
			break;
		}

	case IDC_DAMPING_SLIDER:
		{
			float k = (float)(UI.GetSlider(IDC_DAMPING_SLIDER)->GetValue());
			SimCloth.m_CBperCall.fB = k/100.0f;
			swprintf_s(sz, 100, L"Damping factor: %0.2f", SimCloth.m_CBperCall.fB);
			UI.GetStatic(IDC_DAMPING_STATIC)->SetText(sz);
			break;
		}

	case IDC_AIRRESIST_SLIDER:
		{
			float k = (float)(UI.GetSlider(IDC_AIRRESIST_SLIDER)->GetValue());
			SimCloth.m_CBperCall.fKair = k;
			swprintf_s(sz, 100, L"Air Resistance: %0.2f", SimCloth.m_CBperCall.fKair);
			UI.GetStatic(IDC_AIRRESIST_STATIC)->SetText(sz);
			break;
	   }

	case IDC_MASS_SLIDER:
		{
			float k = (float)(UI.GetSlider(IDC_MASS_SLIDER)->GetValue())/100.0f;
			SimCloth.m_CBperCall.fInvM = 1.0f/k;
			swprintf_s(sz, 100, L"Particle Mass: %0.2f", k);
			UI.GetStatic(IDC_MASS_STATIC)->SetText(sz);
			break;
		 }

	case IDC_WINDHEADING_SLIDER:
		{
			float k = (float)(UI.GetSlider(IDC_WINDHEADING_SLIDER)->GetValue());
			SimCloth.m_fWindHeading = k*3.14159/180.0f;
			swprintf_s(sz, 100, L"Wind Heading: %0.2f", k);
			SimCloth.m_CBperCall.fWindVel = float3(cos(SimCloth.m_fWindHeading)*SimCloth.m_fWindStrength, 0, 
				sin(SimCloth.m_fWindHeading)*SimCloth.m_fWindStrength);

			UI.GetStatic(IDC_WINDHEADING_STATIC)->SetText(sz);
			break;
		}

	case IDC_WINDSTRENGTH_SLIDER:
		{
			float windStrength = (float)(UI.GetSlider(IDC_WINDSTRENGTH_SLIDER)->GetValue())/2.0f;
			swprintf_s(sz, 100, L"Wind Strength: %0.2f", windStrength);
			SimCloth.m_fWindStrength = windStrength;
			SimCloth.m_CBperCall.fWindVel = float3(cos(SimCloth.m_fWindHeading)*SimCloth.m_fWindStrength, 0,
				sin(SimCloth.m_fWindHeading)*SimCloth.m_fWindStrength);
			UI.GetStatic(IDC_WINDSTRENGTH_STATIC)->SetText(sz);
			break;
		 }

	}

}

//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// DXUT will create and use the best device (either D3D9 or D3D11) 
	// that is available on the system depending on which D3D callbacks are set below

	// Set general DXUT callbacks
	DXUTSetCallbackFrameMove( OnFrameMove );
	DXUTSetCallbackKeyboard( OnKeyboard );
	DXUTSetCallbackMouse( OnMouse );
	DXUTSetCallbackMsgProc( MsgProc );
	DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
	DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

	// Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
	DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
	DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
	DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
	DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
	DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
	DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

	// Perform any application-level initialization here

	DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen

	Initial();

	DXUTCreateWindow( L"ClothSimulation" );

	DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1024, 768 );
	DXUTMainLoop(); // Enter into the DXUT ren  der loop

	// Perform any application-level cleanup here

	return DXUTGetExitCode();
}



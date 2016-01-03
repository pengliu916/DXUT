#include "stdafx.h"
#include "Utility.h"
#include "TiledTextures.h"
#include "DrosteEffect.h"

#ifndef SUB_TEXTUREWIDTH
#define SUB_TEXTUREWIDTH 1280
#endif

#ifndef SUB_TEXTUREHEIGHT
#define SUB_TEXTUREHEIGHT 800
#endif

using namespace std::placeholders;
//--------------------------------------------------------------------------------------
//Global Variables
//--------------------------------------------------------------------------------------
TiledTextures       						MultiTexture = TiledTextures();
DrosteEffect                                drosteEffect = DrosteEffect( SUB_TEXTUREWIDTH, SUB_TEXTUREHEIGHT );

CDXUTDialogResourceManager					DialogResourceManager;
CDXUTDialog									UI;

#define IDC_SEPARATE_SATIC          1
#define IDC_SEPARATE_SLIDER         2
#define IDC_PHASE_SATIC             3
#define IDC_PHASE_SLIDER            4
#define IDC_SCALE_SATIC             5
#define IDC_SCALE_SLIDER            6
#define IDC_SAVE                    7

void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

//--------------------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------------------
HRESULT Initial()
{
    HRESULT hr = S_OK;
    UI.Init( &DialogResourceManager );
    //UI.SetFont
    UI.SetCallback( OnGUIEvent ); int iY = 10;

    UI.SetFont( 1, L"Comic Sans MS", 400, 400 );
    UI.SetFont( 2, L"Courier New", 16, FW_NORMAL );

    WCHAR sz[100];
    iY += 24;
    swprintf_s( sz, 100, L"Separate: %0.2f", drosteEffect.m_ConstBuffer.separate );
    UI.AddStatic( IDC_SEPARATE_SATIC, sz, 0, iY, 170, 23 );
    UI.AddSlider( IDC_SEPARATE_SLIDER, 0, iY += 26, 170, 23, 250, 2000, ( int ) ( drosteEffect.m_ConstBuffer.separate*10 ) );

    swprintf_s( sz, 100, L"Phase: %d", ( int ) drosteEffect.m_ConstBuffer.phase );
    UI.AddStatic( IDC_PHASE_SATIC, sz, 0, iY += 26, 170, 23 );
    UI.AddSlider( IDC_PHASE_SLIDER, 0, iY += 26, 170, 23, -5, 5, ( int ) ( drosteEffect.m_ConstBuffer.phase ) );

    swprintf_s( sz, 100, L"Scale: %0.2f", drosteEffect.m_ConstBuffer.scale );
    UI.AddStatic( IDC_SCALE_SATIC, sz, 0, iY += 26, 170, 23 );
    UI.AddSlider( IDC_SCALE_SLIDER, 0, iY += 26, 170, 23, -600, -100, ( int ) ( drosteEffect.m_ConstBuffer.scale * 100.f ) );

    UI.AddButton( IDC_SAVE, L"Save Image", 0, iY += 26, 170, 23 );

    V_RETURN( MultiTexture.Initial() );
    return hr;
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    ( void ) AdapterInfo;
    ( void ) Output;
    ( void ) DeviceInfo;
    ( void ) BackBufferFormat;
    ( void ) bWindowed;
    ( void ) pUserContext;

    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    ( void ) pUserContext;
    MultiTexture.ModifyDeviceSettings( pDeviceSettings );
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependent on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    ( void ) pUserContext;
    ( void ) pBackBufferSurfaceDesc;
    HRESULT hr = S_OK;
    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( drosteEffect.CreateResource( pd3dDevice ) );
    MultiTexture.AddTexture( drosteEffect.m_pInputTexSRV.GetAddressOf(),
                             ( int ) drosteEffect.m_ConstBuffer.inputTexReso.x,
                             ( int ) drosteEffect.m_ConstBuffer.inputTexReso.y, true );
    MultiTexture.AddTexture( drosteEffect.m_pIntermediateTextureSRV.GetAddressOf(),
                             ( int ) drosteEffect.m_ConstBuffer.inputTexReso.x,
                             ( int ) drosteEffect.m_ConstBuffer.inputTexReso.x,
                             false, "", "<float4>",
                             nullptr,
                             std::bind( &DrosteEffect::HandleMessages, &drosteEffect, _1, _2, _3, _4 ) );
    MultiTexture.AddTexture( drosteEffect.m_pOutputTextureSRV.GetAddressOf(),
                             ( int ) drosteEffect.m_ConstBuffer.inputTexReso.x,
                             ( int ) drosteEffect.m_ConstBuffer.inputTexReso.y,
                             false, "", "<float4>",
                             std::bind( &DrosteEffect::Resize, &drosteEffect, _1, _2, _3 ),
                             std::bind( &DrosteEffect::HandleMessages, &drosteEffect, _1, _2, _3, _4 ) );
    V_RETURN( MultiTexture.CreateResource( pd3dDevice ) );
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    ( void ) pSwapChain;
    ( void ) pUserContext;
    HRESULT hr;
    V_RETURN( DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    UI.SetLocation( pBackBufferSurfaceDesc->Width - 180, 0 );
    UI.SetSize( 180, 600 );
    MultiTexture.Resize( pd3dDevice, pBackBufferSurfaceDesc );
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    ( void ) fTime;
    ( void ) fElapsedTime;
    ( void ) pUserContext;
    drosteEffect.m_ConstBuffer.center.x = ( -0.5f + MultiTexture.m_pTileConstBuffer[0].mouseUV_clicked.x )*MultiTexture.m_pTileConstBuffer[0].tileReso.x;
    drosteEffect.m_ConstBuffer.center.y = ( 0.5f - MultiTexture.m_pTileConstBuffer[0].mouseUV_clicked.y )*MultiTexture.m_pTileConstBuffer[0].tileReso.y;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
    ( void ) pd3dDevice;
    ( void ) pUserContext;
    drosteEffect.Render( pd3dImmediateContext, fTime, fElapsedTime );
    MultiTexture.Render( pd3dImmediateContext );
    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR2, L"UI" );
    UI.OnRender( fElapsedTime );
    DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    ( void ) pUserContext;
    DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    ( void ) pUserContext;
    DialogResourceManager.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    MultiTexture.Destory();
    drosteEffect.~DrosteEffect();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    ( void ) pUserContext;
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if ( *pbNoFurtherProcessing )
        return 0;

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = UI.MsgProc( hWnd, uMsg, wParam, lParam );
    if ( *pbNoFurtherProcessing )
        return 0;
    MultiTexture.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    ( void ) nChar;
    ( void ) bKeyDown;
    ( void ) bAltDown;
    ( void ) pUserContext;
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                       int xPos, int yPos, void* pUserContext )
{
    ( void ) bLeftButtonDown;
    ( void ) bRightButtonDown;
    ( void ) bMiddleButtonDown;
    ( void ) bSideButton1Down;
    ( void ) bSideButton2Down;
    ( void ) nMouseWheelDelta;
    ( void ) xPos;
    ( void ) yPos;
    ( void ) pUserContext;
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    ( void ) pUserContext;
    return true;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    ( void ) pControl;
    ( void ) pUserContext;
    ( void ) nEvent;
    WCHAR sz[100];
    switch ( nControlID )
    {
    case IDC_SAVE:
        {
            drosteEffect.SaveHighResoImgToFile( L"OutputImg.jpg" );
            break;
        }
    case IDC_SEPARATE_SLIDER:
        {
            float k = ( float ) ( UI.GetSlider( IDC_SEPARATE_SLIDER )->GetValue() );
            drosteEffect.m_ConstBuffer.separate = k*0.1f;
            swprintf_s( sz, 100, L"Separate: %0.2f", drosteEffect.m_ConstBuffer.separate );
            UI.GetStatic( IDC_SEPARATE_SATIC )->SetText( sz );
            break;
        }

    case IDC_PHASE_SLIDER:
        {
            float k = ( float ) ( UI.GetSlider( IDC_PHASE_SLIDER )->GetValue() );
            drosteEffect.m_ConstBuffer.phase = k;
            swprintf_s( sz, 100, L"Phase: %d", ( int ) drosteEffect.m_ConstBuffer.phase );
            UI.GetStatic( IDC_PHASE_SATIC )->SetText( sz );
            break;
        }

    case IDC_SCALE_SLIDER:
        {
            float k = ( float ) ( UI.GetSlider( IDC_SCALE_SLIDER )->GetValue() );
            drosteEffect.m_ConstBuffer.scale = k*0.01f;
            swprintf_s( sz, 100, L"Scale: %0.2f", drosteEffect.m_ConstBuffer.scale );
            UI.GetStatic( IDC_SCALE_SATIC )->SetText( sz );
            break;
        }
    }
}

//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    ( void ) nCmdShow;
    ( void ) lpCmdLine;
    ( void ) hPrevInstance;
    ( void ) hInstance;
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

    DXUTCreateWindow( L"DrosteEffect" );

    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1600, 800 );
    DXUTMainLoop(); // Enter into the DXUT render loop

                    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}



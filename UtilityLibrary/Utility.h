#pragma once

struct int2{
	int x;
	int y;
	int2(){
		x = 0; y = 0;
	}
	int2(int a, int b){
		x = a;
		y = b;
	}
};

HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG |D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob;
	hr = DXUTCompileFromFile( szFileName, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut );
	
	return hr;
};


void PrintInfo(std::string information)
{
#if PRINT_INFO
	std::ofstream myfile;
	myfile.open ("outlog.txt",std::ios::app);
	myfile << information;
	myfile.close();
#endif
};
Texture2D txPos : register(t0);
Texture2D txNor : register(t1);
Texture2D txImg : register(t2);

SamplerState samGeneral : register(s0);

cbuffer cbPerCall : register(b0){
	int2 widthHeight;
	float k_length;
	float k_angle;

	float b;
	float3 g;

	float l0;
	float kair;
	float invM;
	float dt;

	float3 fWindVel;
	float fGlobalTime;

	float3 vSpherePos;
	float fSphereRadius;
};

cbuffer cbPerFrame : register(b1){
	matrix mObject;
	matrix mWorld;
	matrix mViewProj;
};

struct VS_Render_INPUT{
	float3 pos : POSITION;
	uint Type : TYPE;
};

struct GS_Render_INPUT{
	float4 pos : POSITION;
	float2 fInd : NORMAL;
};

struct PS_Render_INPUT{
	float4 pos :SV_POSITION;
	float3 nor :NORMAL;
	float2 tex :TEXCOORD;
	float3 lPos:TEXCOORD1;
};


struct VS_INPUT
{
	float4 vPosition	: POSITION;
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos    : SV_POSITION;
	float3 Norm   : NORMAL;
	float3 View   : TEXCOORD0;
	float3 lPos  : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
GS_Render_INPUT RenderVS(VS_Render_INPUT input, uint vertexID : SV_VertexID)
{
	GS_Render_INPUT output = (GS_Render_INPUT)0;
	int3 location = int3(vertexID % widthHeight.x, vertexID / widthHeight.x, 0);
		float4 pos = txPos.Load(location);
		output.fInd = float2((float)location.x / (float)widthHeight.x, (float)location.y / (float)widthHeight.y);
		//float4 pos = float4(input.pos,1);
		pos.w = 1;
	output.pos = mul(pos, mWorld);
	return output;
}


VS_OUTPUT SphereVS(VS_INPUT Input)
{
	VS_OUTPUT Output;

	float4 movedPos = Input.vPosition * float4(2.5*fSphereRadius, 2.5*fSphereRadius, 2.5*fSphereRadius, 1) + float4(vSpherePos,0);
		Output.Pos = mul(movedPos, mWorld);
	Output.Pos = mul(Output.Pos, mViewProj);
	Output.lPos = mul(movedPos, mWorld).xyz;
	Output.View = -normalize(Output.lPos);
	float3 movedNormal = Input.vNormal;
		Output.Norm = movedNormal;

	return Output;
}


//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------
[maxvertexcount(3)]
void RenderGS(triangle GS_Render_INPUT input[3], inout TriangleStream<PS_Render_INPUT> outputStream)
{
	PS_Render_INPUT output;
	for (uint i = 0; i < 3; ++i){
		output.lPos = input[i].pos;
		output.pos = mul(input[i].pos, mViewProj);
		output.nor = txNor.SampleLevel(samGeneral,input[i].fInd,0).xyz*2-1.0;
		output.tex = input[i].fInd;
		outputStream.Append(output);
	}
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float3 Normal	: NORMAL;
	float3 View		: TEXCOORD0;
	float3 lPos		: TEXCOORD1;
};


static const float4 ambient = float4(0.9255, 0.9380, 0.9380, 1);
static const float4 diffuse = float4(0.8392, 0.8623, 0.8904, 1);
static const float4 specular = float4(1.0000, 0.9567, 0.6704, 1);
static const float4 ks = float4(0.08, 0.44, 0.82, 16);
static const float3 lightPos = float3(0, 10, -2000);
static const float3 lightPos1 = float3(0, 10, 2000);

float4 Lighting(float3 lPos, float3 nor, float3 pos)
{
	float3 light = normalize(lPos - pos);
	float rdot = dot(nor, light);
	return diffuse * ks.y * max(0, rdot);

}
float4 RenderPS(PS_Render_INPUT input) : SV_Target
{
	//float light = abs(dot(normalize(float3(0, 0, -2)), input.nor));
	////float light = clamp(dot(normalize(float3(0, 0, -2)), input.nor), 0, 1);
	//float4 tex = txImg.SampleLevel(samGeneral, input.tex,0);
	//return tex*(float4(0.1, 0.1, 0.1, 1)*0.1 + light*0.8);

	float4 tex = txImg.SampleLevel(samGeneral, input.tex,0);
	float3 light = normalize(lightPos - input.lPos);
	float rdot = dot(input.nor, light);
	float4 AmbientColor = ambient * ks.x;
		float4 DiffuseColor = diffuse * ks.y * abs(rdot);
		float4 color = AmbientColor + DiffuseColor;// +SpecularColor;
		color.a = 1;
	return color*tex;

}


float4 SpherePS(VS_OUTPUT Input) : SV_TARGET
{
	float3 light = normalize(lightPos - Input.lPos);
	float rdot = dot(Input.Norm, light);
	float4 AmbientColor = ambient * ks.x;
	float4 DiffuseColor = diffuse * ks.y * abs(rdot);
	float4 color = AmbientColor + DiffuseColor;// +SpecularColor;
	color.a = 1;
	return color;
}




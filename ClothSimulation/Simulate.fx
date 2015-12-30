Texture2D txPos : register( t0 );
Texture2D txVel : register( t1 );
Texture2D txAcc : register( t2 ); 

Texture2D txV0 : register( t3 );
Texture2D txV1 : register( t4 );
Texture2D txV2 : register( t5 );
Texture2D txV3 : register( t6 );

Texture2D txA0 : register( t7 );
Texture2D txA1 : register( t8 );
Texture2D txA2 : register( t9 );
Texture2D txA3 : register( t10 );

Texture1D txRandom : register(t11);

SamplerState samGeneral : register(s0);

cbuffer cbPerCall : register( b0 ){
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

cbuffer cbPerFrame : register( b1 ){
	matrix mObject;
	matrix mWorld;
	matrix mViewProj;
};

float3 RandomDir(float offset)
{
	float tCoord = (fGlobalTime + offset) / 100.0;
	return txRandom.SampleLevel(samGeneral, tCoord, 0).xyz;
}

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct GS_Simulate_INPUT
{
};

struct PS_Simulate_INPUT
{
    float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};
//--------------------------------------------------------------------------------------
// Vertex Shader for every filter
//--------------------------------------------------------------------------------------
GS_Simulate_INPUT SimulateVS( )
{
    GS_Simulate_INPUT output = (GS_Simulate_INPUT)0;
 
    return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader for every filter
//--------------------------------------------------------------------------------------
[maxvertexcount(4)]
void SimulateGS(point GS_Simulate_INPUT particles[1], inout TriangleStream<PS_Simulate_INPUT> triStream)
{
   PS_Simulate_INPUT output;
	output.Pos=float4(-1.0f,1.0f,0.0f,1.0f);
	output.Tex=float2(0.0f,0.0f);
	triStream.Append(output);

	output.Pos=float4(1.0f,1.0f,0.0f,1.0f);
	output.Tex=float2(widthHeight.x,0.0f);
	triStream.Append(output);

	output.Pos=float4(-1.0f,-1.0f,0.0f,1.0f);
	output.Tex=float2(0.0f,widthHeight.y);
	triStream.Append(output);

	output.Pos=float4(1.0f,-1.0f,0.0f,1.0f);
	output.Tex=float2(widthHeight.x,widthHeight.y);
	triStream.Append(output);
}

//--------------------------------------------------------------------------------------
// Pixel Shader 
//--------------------------------------------------------------------------------------
struct Sim_2Tex_OUT{
	float4 out1;
	float4 out2;
};

float3 DampedHookeForce( float3 pA, float3 vA, int2 iB, float l, float k )
{
	float3 force = float3( 0, 0, 0 );

	float4 pB = txPos.Load( int3( iB, 0 ) );
	float4 vB;
	if( pB.w > 0.5 ){

		float3 delta = pA - pB.xyz;
		float dist = max(length(delta), 1e-7);
		
		force = (l - dist)*delta*k / dist;
		
		vB = txVel.Load( int3( iB, 0 ) );
		float3 deltaV = vA - vB.xyz;
		float vdist = max(length(deltaV), 1e-7);
		force += deltaV * ( -b ) / vdist;
	}
	return force;
}

float3 ARinTri(int2 clocation, int2 offset1, int2 offset2)
{
	float3 p0 = txPos.Load(int3(clocation, 0)).xyz;
	float3 v0 = txVel.Load(int3(clocation, 0)).xyz;

	float4 testP = txPos.Load(int3(clocation, 0), offset1);
	float3 p1, v1;
	if (testP.w < 0.5)	return float3(0, 0, 0);
	else{
		p1 = testP.xyz;
		v1 = txVel.Load(int3(clocation, 0), offset1).xyz;
	}

	float3 p2, v2;
	testP = txPos.Load(int3(clocation, 0), offset2);
	if (testP.w < 0.5) return float3(0, 0, 0);
	else{
		p2 = testP.xyz;
		v2 = txVel.Load(int3(clocation, 0), offset2).xyz;
	}
	// Relative wind vel
	float3 noise = normalize(RandomDir((clocation.x + clocation.y + fGlobalTime)*0.01));
	float3 v = (v0 + v1 + v2) / 3.0f - fWindVel*(1.0+0.3*noise);
	float	l_v = length(v);

		float3 r1 = p1 - p0;
		float3 r2 = p2 - p0;

		float3 r2xr1 = cross(r2, r1);
		float l_r2xr1 = length(r2xr1);
	// Triangle area
	float area = 0.5*l_r2xr1;
	// Triangle Normal
	float3 n = r2xr1 / l_r2xr1;
		// Projected area
		float a0 = area*dot(v, n) / l_v;
	float	vs = dot(v, v);
	float3 force = -0.5*kair*vs*a0*n;
		//return -0.5*kair*length(v)*dot(v, r2xr1) / (2.0f*l_r2xr1)*r2xr1;
	return force;

}
float3 AirResistance(int2 location)
{
	float3 force = float3(0,0,0);
	force += ARinTri(location, int2(-1, 0), int2(0, -1));
	force += ARinTri(location, int2(0, -1), int2(1, 0));
	force += ARinTri(location, int2(1, 0), int2(0, 1));
	force += ARinTri(location, int2(0, 1), int2(-1, 0));
	return force;
}
float3 acceleration( int2 location )
{
	float dist = 0;
	float3 acc = g;
	float3 force = float3(0, 0, 0);
	float3 Pos = txPos.Load(int3(location, 0)).xyz;
	float3 Vel = txVel.Load(int3(location, 0)).xyz;
	int factor = 2;
	force += DampedHookeForce(Pos, Vel, location + int2(0, 1)*factor, factor * l0, k_angle);
	force += DampedHookeForce(Pos, Vel, location + int2(0, -1)*factor, factor * l0, k_angle);
	force += DampedHookeForce(Pos, Vel, location + int2(1, 0)*factor, factor * l0, k_angle);
	force += DampedHookeForce(Pos, Vel, location + int2(-1, 0)*factor, factor * l0, k_angle);
	 
	force += DampedHookeForce(Pos, Vel, location + int2(1, 1) * factor, 1.4121356 * l0 * factor, k_angle);
	force += DampedHookeForce(Pos, Vel, location + int2(1, -1) * factor, 1.4121356 * l0 * factor, k_angle);
	force += DampedHookeForce(Pos, Vel, location + int2(-1, -1) * factor, 1.4121356 * l0 * factor, k_angle);
	force += DampedHookeForce(Pos, Vel, location + int2(-1, 1) * factor, 1.4121356 * l0 * factor, k_angle);

	force += DampedHookeForce( Pos, Vel, location + int2( 0, 1 ), l0, k_length );
	force += DampedHookeForce( Pos, Vel, location + int2( 0, -1 ), l0, k_length );
	force += DampedHookeForce( Pos, Vel, location + int2( 1, 0 ), l0, k_length );
	force += DampedHookeForce( Pos, Vel, location + int2( -1, 0 ), l0, k_length );

	force += DampedHookeForce(Pos, Vel, location + int2(1, 1), 1.4121356 * l0, k_length);
	force += DampedHookeForce(Pos, Vel, location + int2(1, -1), 1.4121356 * l0, k_length);
	force += DampedHookeForce(Pos, Vel, location + int2(-1, -1), 1.4121356 * l0, k_length);
	force += DampedHookeForce(Pos, Vel, location + int2(-1, 1), 1.4121356 * l0, k_length);


	// Wind
	float3 wind = AirResistance(location);
	force += wind;
	
	acc += force * invM;

	return acc;
}

float4 GetAccPS(PS_Simulate_INPUT input) : SV_Target
{
	float3 acc = acceleration( int2( input.Tex ) );
	return float4( acc, 1 );
}

Sim_2Tex_OUT UpdateAnchorPS(PS_Simulate_INPUT input) : SV_Target
{
	Sim_2Tex_OUT output;
	float4 Pos = txPos.Load(int3(input.Tex, 0));
	float4 Vel = txVel.Load(int3(input.Tex, 0));
	/*if( Pos.w < 1.5 ){
		float4 aPos = float4( input.Tex / float2( widthHeight - 1.0f ) - 1.0f, 0, 1 );
		Pos = mul( Pos, mObject );
		Pos.w = 1;
	}*/
	output.out1 = Pos;
	output.out2 = Vel;
	return output;
}

Sim_2Tex_OUT GetPosVelPS(PS_Simulate_INPUT input) : SV_Target
{
	Sim_2Tex_OUT output;
	float4 Pos = txPos.Load( int3( input.Tex, 0 ));
	if( Pos.w > 1.5 ){ // Not an anchor point
		output.out1 = float4( Pos.xyz + dt * txVel.Load( int3( input.Tex, 0 )).xyz, Pos.w );
		output.out2 = float4( txVel.Load( int3( input.Tex, 0 )).xyz + dt * txAcc.Load( int3( input.Tex, 0 )).xyz, 1 );
	}else{
		output.out1 = Pos;
		output.out2 = float4( 0, 0, 0, 1 );
	}
	return output;
}
float3 SphereConstraint(float3 pos, float3 center, float radius)
{
	float3 delta = pos - center;
	float distance = length(delta);
	if (distance < radius) return (radius - distance)*delta / distance;
	else return 0;
}
Sim_2Tex_OUT SimulatePS(PS_Simulate_INPUT input) : SV_Target
{
	Sim_2Tex_OUT  output;

	int3 cLocation = int3( input.Tex, 0 );
	float4 currentPos = txPos.Load( cLocation );
	float4 currentVel = txVel.Load( cLocation );

	float4 v0 = txV0.Load( cLocation );
	float4 v1 = txV1.Load( cLocation );
	float4 v2 = txV2.Load( cLocation );
	float4 v3 = txV3.Load( cLocation );

	float4 a0 = txA0.Load( cLocation );
	float4 a1 = txA1.Load( cLocation );
	float4 a2 = txA2.Load( cLocation );
	float4 a3 = txA3.Load( cLocation );

	if( currentPos.w > 1.5 ){
		output.out1 = currentPos + 1.0f / 6.0f * dt * ( v0 + 2 * v1 + 2 * v2 + v3 ); 
		output.out1.w = currentPos.w;
		output.out2 = currentVel + 1.0f / 6.0f * dt * ( a0 + 2 * a1 + 2 * a2 + a3 );
		output.out2.w = 1;
		output.out1.xyz += SphereConstraint(output.out1.xyz, vSpherePos, fSphereRadius);

	}else{
		output.out1 = currentPos;
		output.out2 = currentVel;
	}
	return output;
}


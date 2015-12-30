#define E 2.71828182845904523536
#define TAU 6.28318530718
#define R1 20
Texture2D<float4>   tex : register( t0 );
SamplerState        samGeneral: register( s0 );

cbuffer constBuffer : register( b0 )
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

void VS( in uint VertID : SV_VertexID, out float4 Pos : SV_Position, out float2 Tex : TexCoord0 )
{
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2( uint2( VertID, VertID << 1 ) & 2 );
    Pos = float4( lerp( float2( -1, 1 ), float2( 1, -1 ), Tex ), 0, 1 );
}

float2 RotateScale( float2 uv )
{
    float alpha = atan( phase * log( separate / R1 ) / TAU );
    float2 rot = float2( cos( alpha ), sin( alpha ) );
    // Rotate and scale
    return float2( uv.x*rot.x - uv.y*rot.y, uv.x*rot.y + uv.y*rot.x ) / rot.x;
}

float2 ConvertToCplxPlane( float2 uv, float2 imgReso, float2 offset )
{
    return ( uv - 0.5f ) * imgReso * float2( 1.f, -1.f ) - offset;
}

float2 ConvertToUV( float2 xy, float2 imgReso, float2 offset )
{
    return ( xy + offset ) * float2( 1.f, -1.f ) / imgReso + 0.5f;
}

float2 LogMap( float2 uv )
{
    return float2( log( length( uv ) ), atan2( uv.y, uv.x ) );
}

float2 ExpMap( float2 xy )
{
    return R1*exp( xy.x )*float2( cos( xy.y ), sin( xy.y ) );
}

float2 RepetitionSolver( float2 xy, inout float intRepetition )
{
    float2 r = xy;
    for ( uint i = 0; i < 30; i++ )
    {
        float2 resultXY = ExpMap( r );
        float alpha = tex.SampleLevel( samGeneral, ConvertToUV( resultXY, inputTexReso, center ), 0 ).a;
        if ( alpha >= 0.5f ) return r;
        else
        {
            r.x += log( separate / R1 );
            continue;
        }
    }
    return r;
}

float2 Droste( float2 uv )
{
    float2 _uv = LogMap( uv );
    _uv = RotateScale( _uv );
    float rep = 0;
    _uv = RepetitionSolver( _uv, rep );
    return ExpMap( _uv );
}

float4 PS( float4 Pos : SV_Position, float2 TexUV : TexCoord0 ) : SV_Target
{
    float t = exp( E*scale );
    float2 xy = ConvertToCplxPlane( TexUV, inputTexReso, center );
    xy = Droste( t*xy );
    float4 col = tex.SampleLevel( samGeneral, ConvertToUV( xy, inputTexReso, center ), 0 );
    return col;
}

float4 intermPS( float4 Pos : SV_Position, float2 TexUV : TexCoord0 ) : SV_Target
{
    float2 xy = ConvertToCplxPlane( TexUV, logmapReso, float2( -1.57f,-3.14f ) );

    float threashold = 0.03f;
    if ( abs( xy.y ) <= threashold || abs( xy.y - TAU ) <= threashold
         || abs( xy.x ) <= threashold || abs( xy.x - log( separate / R1 ) ) <= threashold )
        return float4( 0.f, 1.f, 0.f, 1.f );

    xy = RotateScale( xy );
    float rep = 0;
    xy = RepetitionSolver( xy, rep );
    xy = ExpMap( xy );
    float4 col = tex.SampleLevel( samGeneral, ConvertToUV( xy, inputTexReso, center ), 0 );

    return col;
}

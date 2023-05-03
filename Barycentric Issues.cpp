
v2i V2I_F24_8(v2 A)
{
    v2i Result = {};
    Result.x = (i32)round(A.x * 256.0f);
    Result.y = (i32)round(A.y * 256.0f);
    return Result;
}

f32 CrossProduct2d(v2 A, v2 B)
{
    f32 Result = (A.x) * (B.y) - (A.y) * (B.x);
    return Result;
}

i64 CrossProduct2d(v2i A, v2i B)
{
    i64 Result = i64(A.x) * i64(B.y) - i64(A.y) * i64(B.x);
    return Result;
}

/*
  NOTE: This is the floating point version of the code that doesn't have these issues
 */

void DrawTriangle(clip_vertex Vertex0, clip_vertex Vertex1, clip_vertex Vertex2, 
                  texture Texture, sampler Sampler, u32 Color)
{
    Vertex0.Pos.w = 1.0f / Vertex0.Pos.w;
    Vertex1.Pos.w = 1.0f / Vertex1.Pos.w;
    Vertex2.Pos.w = 1.0f / Vertex2.Pos.w;
    
    Vertex0.Pos.xyz *= Vertex0.Pos.w;
    Vertex1.Pos.xyz *= Vertex1.Pos.w;
    Vertex2.Pos.xyz *= Vertex2.Pos.w;
    
    v2 PointA = NdcToPixels(Vertex0.Pos.xy);
    v2 PointB = NdcToPixels(Vertex1.Pos.xy);
    v2 PointC = NdcToPixels(Vertex2.Pos.xy);

    i32 MinX = min(min((i32)PointA.x, (i32)PointB.x), (i32)PointC.x);
    i32 MaxX = max(max((i32)round(PointA.x), (i32)round(PointB.x)), (i32)round(PointC.x));
    i32 MinY = min(min((i32)PointA.y, (i32)PointB.y), (i32)PointC.y);
    i32 MaxY = max(max((i32)round(PointA.y), (i32)round(PointB.y)), (i32)round(PointC.y));

    Vertex0.Uv *= Vertex0.Pos.w;
    Vertex1.Uv *= Vertex1.Pos.w;
    Vertex2.Uv *= Vertex2.Pos.w;
    
    f32 BaryCentricDiv = CrossProduct2d(PointB - PointA, PointC - PointA);
    
    v2 Edge0 = PointB - PointA;
    v2 Edge1 = PointC - PointB;
    v2 Edge2 = PointA - PointC;

    b32 IsTopLeft0 = (Edge0.y > 0.0f) || (Edge0.x > 0.0f && Edge0.y == 0.0f);
    b32 IsTopLeft1 = (Edge1.y > 0.0f) || (Edge1.x > 0.0f && Edge1.y == 0.0f);
    b32 IsTopLeft2 = (Edge2.y > 0.0f) || (Edge2.x > 0.0f && Edge2.y == 0.0f);

    f32 Edge0DiffX = Edge0.y;
    f32 Edge1DiffX = Edge1.y;
    f32 Edge2DiffX = Edge2.y;

    f32 Edge0DiffY = -Edge0.x;
    f32 Edge1DiffY = -Edge1.x;
    f32 Edge2DiffY = -Edge2.x;

    v2 StartPos = V2(MinX, MinY) + V2(0.5f, 0.5f);
    f32 Edge0RowY = CrossProduct2d(StartPos - PointA, Edge0);
    f32 Edge1RowY = CrossProduct2d(StartPos - PointB, Edge1);
    f32 Edge2RowY = CrossProduct2d(StartPos - PointC, Edge2);
    
    for (i32 Y = MinY; Y <= MaxY; ++Y)
    {
        f32 Edge0RowX = Edge0RowY;
        f32 Edge1RowX = Edge1RowY;
        f32 Edge2RowX = Edge2RowY;
        
        for (i32 X = MinX; X <= MaxX; ++X)
        {
            v2 PixelPoint = V2(X, Y) + V2(0.5f, 0.5f);

            v2 PixelEdge0 = PixelPoint - PointA;
            v2 PixelEdge1 = PixelPoint - PointB;
            v2 PixelEdge2 = PixelPoint - PointC;
            
            if ((Edge0RowX > 0.0f || (IsTopLeft0 && Edge0RowX == 0.0f)) &&
                (Edge1RowX > 0.0f || (IsTopLeft1 && Edge1RowX == 0.0f)) &&
                (Edge2RowX > 0.0f || (IsTopLeft2 && Edge2RowX == 0.0f)))
            {
                // NOTE: Ми у середині трикутника
                u32 PixelId = Y * GlobalState.FrameBufferWidth + X;

                f32 T0 = -Edge1RowX / BaryCentricDiv;
                f32 T1 = -Edge2RowX / BaryCentricDiv;
                f32 T2 = -Edge0RowX / BaryCentricDiv;

                f32 DepthZ = T0 * Vertex0.Pos.z + T1 * Vertex1.Pos.z + T2 * Vertex2.Pos.z;
                if (GlobalState.DepthBuffer[PixelId] < DepthZ)
                {
                    GlobalState.FrameBufferPixels[PixelId] = Color;
                    GlobalState.DepthBuffer[PixelId] = DepthZ;
                }
            }

            Edge0RowX += Edge0DiffX;
            Edge1RowX += Edge1DiffX;
            Edge2RowX += Edge2DiffX;
        }

        Edge0RowY += Edge0DiffY;
        Edge1RowY += Edge1DiffY;
        Edge2RowY += Edge2DiffY;
    }
}

/*
  NOTE: Fixed Point Version of the code, generates inaccurate depth values
 */

void DrawTriangle(clip_vertex Vertex0, clip_vertex Vertex1, clip_vertex Vertex2, 
                  texture Texture, sampler Sampler, u32 Color)
{
    Vertex0.Pos.w = 1.0f / Vertex0.Pos.w;
    Vertex1.Pos.w = 1.0f / Vertex1.Pos.w;
    Vertex2.Pos.w = 1.0f / Vertex2.Pos.w;
    
    Vertex0.Pos.xyz *= Vertex0.Pos.w;
    Vertex1.Pos.xyz *= Vertex1.Pos.w;
    Vertex2.Pos.xyz *= Vertex2.Pos.w;

    Vertex0.Uv *= Vertex0.Pos.w;
    Vertex1.Uv *= Vertex1.Pos.w;
    Vertex2.Uv *= Vertex2.Pos.w;

    i32 MinX = min(min((i32)PointAF.x, (i32)PointBF.x), (i32)PointCF.x);
    i32 MaxX = max(max((i32)round(PointAF.x), (i32)round(PointBF.x)), (i32)round(PointCF.x));
    i32 MinY = min(min((i32)PointAF.y, (i32)PointBF.y), (i32)PointCF.y);
    i32 MaxY = max(max((i32)round(PointAF.y), (i32)round(PointBF.y)), (i32)round(PointCF.y));
    
    v2 PointAF = NdcToPixels(Vertex0.Pos.xy);
    v2 PointBF = NdcToPixels(Vertex1.Pos.xy);
    v2 PointCF = NdcToPixels(Vertex2.Pos.xy);

    v2i PointA = V2I_F24_8(PointAF);
    v2i PointB = V2I_F24_8(PointBF);
    v2i PointC = V2I_F24_8(PointCF);
    
    v2i Edge0 = PointB - PointA;
    v2i Edge1 = PointC - PointB;
    v2i Edge2 = PointA - PointC;

    b32 IsTopLeft0 = (Edge0.y > 0) || (Edge0.x > 0 && Edge0.y == 0);
    b32 IsTopLeft1 = (Edge1.y > 0) || (Edge1.x > 0 && Edge1.y == 0);
    b32 IsTopLeft2 = (Edge2.y > 0) || (Edge2.x > 0 && Edge2.y == 0);
    
    f32 BaryCentricDiv = 1.0f / (f32(CrossProduct2d(PointBF - PointAF, PointCF - PointAF)));

    i32 Edge0DiffX = Edge0.y;
    i32 Edge1DiffX = Edge1.y;
    i32 Edge2DiffX = Edge2.y;

    i32 Edge0DiffY = -Edge0.x;
    i32 Edge1DiffY = -Edge1.x;
    i32 Edge2DiffY = -Edge2.x;

    v2i StartPos = V2I_F24_8(V2(MinX, MinY) + V2(0.5f, 0.5f));
    i64 Edge0RowY64 = CrossProduct2d(StartPos - PointA, Edge0);
    i64 Edge1RowY64 = CrossProduct2d(StartPos - PointB, Edge1);
    i64 Edge2RowY64 = CrossProduct2d(StartPos - PointC, Edge2);

    i32 Offset0 = (IsTopLeft0 ? 0 : -1);
    i32 Offset1 = (IsTopLeft1 ? 0 : -1);
    i32 Offset2 = (IsTopLeft2 ? 0 : -1);
    
    i32 Edge0RowY = i32((Edge0RowY64 + Sign(Edge0RowY64) * 128) / 256) + Offset0;
    i32 Edge1RowY = i32((Edge1RowY64 + Sign(Edge1RowY64) * 128) / 256) + Offset1;
    i32 Edge2RowY = i32((Edge2RowY64 + Sign(Edge2RowY64) * 128) / 256) + Offset2;
    
    for (i32 Y = MinY; Y <= MaxY; ++Y)
    {
        i32 Edge0RowX = Edge0RowY;
        i32 Edge1RowX = Edge1RowY;
        i32 Edge2RowX = Edge2RowY;
        
        for (i32 X = MinX; X <= MaxX; ++X)
        {
            if (Edge0RowX >= 0 && Edge1RowX >= 0 && Edge2RowX >= 0)
            {
                // NOTE: Ми у середині трикутника
                u32 PixelId = Y * GlobalState.FrameBufferWidth + X;

                f32 T0 = -f32(Edge1RowX) * BaryCentricDiv / 256.0f;
                f32 T1 = -f32(Edge2RowX) * BaryCentricDiv / 256.0f;
                f32 T2 = -f32(Edge0RowX) * BaryCentricDiv / 256.0f;
                    
                f32 DepthZ = T0 * Vertex0.Pos.z + T1 * Vertex1.Pos.z + T2 * Vertex2.Pos.z;
                if (GlobalState.DepthBuffer[PixelId] < DepthZ)
                {
                    GlobalState.FrameBufferPixels[PixelId] = Color;
                    GlobalState.DepthBuffer[PixelId] = DepthZ;
                }
            }

            Edge0RowX += Edge0DiffX;
            Edge1RowX += Edge1DiffX;
            Edge2RowX += Edge2DiffX;
        }

        Edge0RowY += Edge0DiffY;
        Edge1RowY += Edge1DiffY;
        Edge2RowY += Edge2DiffY;
    }
}


/*

  NOTE: Below function uses floats for rasterization. I calculate my edge values incrementally and via CrossProduct2d, and I compare
        their signs to see if they would ever rasterize differently. I can see the breakpoint sometimes gets hit and from time to time,
        there are cracks in my rotating quad.
  
 */

f32 CrossProduct2d(v2 A, v2 B)
{
    f32 Result = A.x * B.y - A.y * B.x;
    return Result;
}

void DrawTriangle(v3 ModelVertex0, v3 ModelVertex1, v3 ModelVertex2,
                  v3 ModelColor0, v3 ModelColor1, v3 ModelColor2,
                  m4 Transform)
{
    v4 TransformedPoint0 = (Transform * V4(ModelVertex0, 1.0f));
    v4 TransformedPoint1 = (Transform * V4(ModelVertex1, 1.0f));
    v4 TransformedPoint2 = (Transform * V4(ModelVertex2, 1.0f));
    
    TransformedPoint0.xyz /= TransformedPoint0.w;
    TransformedPoint1.xyz /= TransformedPoint1.w;
    TransformedPoint2.xyz /= TransformedPoint2.w;
    
    v2 PointA = NdcToPixels(TransformedPoint0.xy);
    v2 PointB = NdcToPixels(TransformedPoint1.xy);
    v2 PointC = NdcToPixels(TransformedPoint2.xy);

    i32 MinX = min(min((i32)PointA.x, (i32)PointB.x), (i32)PointC.x);
    i32 MaxX = max(max((i32)round(PointA.x), (i32)round(PointB.x)), (i32)round(PointC.x));
    i32 MinY = min(min((i32)PointA.y, (i32)PointB.y), (i32)PointC.y);
    i32 MaxY = max(max((i32)round(PointA.y), (i32)round(PointB.y)), (i32)round(PointC.y));

    MinX = max(0, MinX);
    MinX = min(GlobalState.FrameBufferWidth - 1, MinX);
    MaxX = max(0, MaxX);
    MaxX = min(GlobalState.FrameBufferWidth - 1, MaxX);
    MinY = max(0, MinY);
    MinY = min(GlobalState.FrameBufferHeight - 1, MinY);
    MaxY = max(0, MaxY);
    MaxY = min(GlobalState.FrameBufferHeight - 1, MaxY);
    
    v2 Edge0 = PointB - PointA;
    v2 Edge1 = PointC - PointB;
    v2 Edge2 = PointA - PointC;

    b32 IsTopLeft0 = (Edge0.x >= 0.0f && Edge0.y > 0.0f) || (Edge0.x > 0.0f && Edge0.y == 0.0f);
    b32 IsTopLeft1 = (Edge1.x >= 0.0f && Edge1.y > 0.0f) || (Edge1.x > 0.0f && Edge1.y == 0.0f);
    b32 IsTopLeft2 = (Edge2.x >= 0.0f && Edge2.y > 0.0f) || (Edge2.x > 0.0f && Edge2.y == 0.0f);
    
    f32 BaryCentricDiv = CrossProduct2d(PointB - PointA, PointC - PointA);

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

    f32 DummySave0 = Edge0RowY;
    f32 DummySave1 = Edge1RowY;
    f32 DummySave2 = Edge2RowY;
    
    for (i32 Y = MinY; Y <= MaxY; ++Y)
    {
        f32 EdgeRowX0 = Edge0RowY;
        f32 EdgeRowX1 = Edge1RowY;
        f32 EdgeRowX2 = Edge2RowY;
        
        for (i32 X = MinX; X <= MaxX; ++X)
        {
            v2 PixelPoint = V2(X, Y) + V2(0.5f, 0.5f);

            v2 PixelEdge0 = PixelPoint - PointA;
            v2 PixelEdge1 = PixelPoint - PointB;
            v2 PixelEdge2 = PixelPoint - PointC;

            f32 CrossLength0 = CrossProduct2d(PixelEdge0, Edge0);
            f32 CrossLength1 = CrossProduct2d(PixelEdge1, Edge1);
            f32 CrossLength2 = CrossProduct2d(PixelEdge2, Edge2);

            if (sign(CrossLength0) != sign(EdgeRowX0) ||
                sign(CrossLength1) != sign(EdgeRowX1) ||
                sign(CrossLength2) != sign(EdgeRowX2))
            {
                // If I set a breakpoint here, I often run into it (mostly its differences on contours of the object but can also be
                // cracks in the model
                int i = 0;
            }
            
            if ((EdgeRowX0 > 0.0f || (IsTopLeft0 && EdgeRowX0 == 0.0f)) &&
                (EdgeRowX1 > 0.0f || (IsTopLeft1 && EdgeRowX1 == 0.0f)) &&
                (EdgeRowX2 > 0.0f || (IsTopLeft2 && EdgeRowX2 == 0.0f)))
            {
                u32 PixelId = Y * GlobalState.FrameBufferWidth + X;

                f32 T0 = -EdgeRowX1 / BaryCentricDiv;
                f32 T1 = -EdgeRowX2 / BaryCentricDiv;
                f32 T2 = -EdgeRowX0 / BaryCentricDiv;

                f32 DepthZ = T0 * TransformedPoint0.z + T1 * TransformedPoint1.z + T2 * TransformedPoint2.z;
                if (DepthZ >= 0.0f && DepthZ <= 1.0f && DepthZ < GlobalState.DepthBuffer[PixelId])
                {
                    v3 FinalColor = T0 * ModelColor0 + T1 * ModelColor1 + T2 * ModelColor2;
                    FinalColor = FinalColor * 255.0f;
                    u32 FinalColorU32 = ((u32)0xFF << 24) | ((u32)FinalColor.r << 16) | ((u32)FinalColor.g << 8) | (u32)FinalColor.b;

                    GlobalState.FrameBufferPixels[PixelId] = 0xFF00FF00; //FinalColorU32;
                    GlobalState.DepthBuffer[PixelId] = DepthZ;
                }
            }
        
            EdgeRowX0 += Edge0DiffX;
            EdgeRowX1 += Edge1DiffX;
            EdgeRowX2 += Edge2DiffX;
        }

        Edge0RowY += Edge0DiffY;
        Edge1RowY += Edge1DiffY;
        Edge2RowY += Edge2DiffY;
    }
}

/*

  NOTE: This is the fixed point version of the above function. It mostly works the same but we use 28:4 format. The cracks in this
        function are far worse than the above one. I calculate the starting edge values using 64bit integers, but the incremental
        calculations in the loop are all done via 32bit integer values. Im not sure if there is a better way to organize the fixed
        point math, I want to keep everything 32bit but the result looks a lot worse than what I wanted.
  
 */

i32 F32ToFp24_8(f32 A)
{
    i32 Result = i32(roundf(A * powf(2.0f, 8.0f)));
    return Result;
}

v2i V2ToFp24_8(v2 A)
{
    v2i Result = {};
    Result.x = F32ToFp24_8(A.x);
    Result.y = F32ToFp24_8(A.y);

    return Result;
}

void DrawTriangle(clip_vertex Vertex0, clip_vertex Vertex1, clip_vertex Vertex2, 
                  texture Texture, sampler Sampler)
{
    Vertex0.Pos.w = 1.0f / Vertex0.Pos.w;
    Vertex1.Pos.w = 1.0f / Vertex1.Pos.w;
    Vertex2.Pos.w = 1.0f / Vertex2.Pos.w;
    
    Vertex0.Pos.xyz *= Vertex0.Pos.w;
    Vertex1.Pos.xyz *= Vertex1.Pos.w;
    Vertex2.Pos.xyz *= Vertex2.Pos.w;
    
    v2 PointAF = NdcToPixels(Vertex0.Pos.xy);
    v2 PointBF = NdcToPixels(Vertex1.Pos.xy);
    v2 PointCF = NdcToPixels(Vertex2.Pos.xy);

    i32 MinX = min(min((i32)PointAF.x, (i32)PointBF.x), (i32)PointCF.x);
    i32 MaxX = max(max((i32)round(PointAF.x), (i32)round(PointBF.x)), (i32)round(PointCF.x));
    i32 MinY = min(min((i32)PointAF.y, (i32)PointBF.y), (i32)PointCF.y);
    i32 MaxY = max(max((i32)round(PointAF.y), (i32)round(PointBF.y)), (i32)round(PointCF.y));

#if 1
    MinX = max(0, MinX);
    MinX = min(GlobalState.FrameBufferWidth - 1, MinX);
    MaxX = max(0, MaxX);
    MaxX = min(GlobalState.FrameBufferWidth - 1, MaxX);
    MinY = max(0, MinY);
    MinY = min(GlobalState.FrameBufferHeight - 1, MinY);
    MaxY = max(0, MaxY);
    MaxY = min(GlobalState.FrameBufferHeight - 1, MaxY);
#endif

    v2i PointA = V2ToFp24_8(PointAF);
    v2i PointB = V2ToFp24_8(PointBF);
    v2i PointC = V2ToFp24_8(PointCF);
    
    v2i Edge0 = PointB - PointA;
    v2i Edge1 = PointC - PointB;
    v2i Edge2 = PointA - PointC;

    b32 IsTopLeft0 = (Edge0.y > 0) || (Edge0.x > 0 && Edge0.y == 0);
    b32 IsTopLeft1 = (Edge1.y > 0) || (Edge1.x > 0 && Edge1.y == 0);
    b32 IsTopLeft2 = (Edge2.y > 0) || (Edge2.x > 0 && Edge2.y == 0);
    
    f32 BaryCentricDivisor = CrossProduct2d(PointBF - PointAF, PointCF - PointAF);

    Vertex0.Uv = Vertex0.Uv * Vertex0.Pos.w;
    Vertex1.Uv = Vertex1.Uv * Vertex1.Pos.w;
    Vertex2.Uv = Vertex2.Uv * Vertex2.Pos.w;
    
    i32 Edge0DiffX = Edge0.y;
    i32 Edge1DiffX = Edge1.y;
    i32 Edge2DiffX = Edge2.y;

    i32 Edge0DiffY = -Edge0.x;
    i32 Edge1DiffY = -Edge1.x;
    i32 Edge2DiffY = -Edge2.x;

    i32 Edge0RowY = 0;
    i32 Edge1RowY = 0;
    i32 Edge2RowY = 0;
    {
        v2i StartPos = V2ToFp28_4(V2(MinX, MinY) + V2(0.5f, 0.5f));

        v2i Temp0 = StartPos - PointA;
        v2i Temp1 = StartPos - PointB;
        v2i Temp2 = StartPos - PointC;
        
        i64 Temp0RowY = i64(Temp0.x) * i64(Edge0.y) - i64(Temp0.y) * i64(Edge0.x);
        i64 Temp1RowY = i64(Temp1.x) * i64(Edge1.y) - i64(Temp1.y) * i64(Edge1.x);
        i64 Temp2RowY = i64(Temp2.x) * i64(Edge2.y) - i64(Temp2.y) * i64(Edge2.x);

        Edge0RowY = i32(round(f64(Temp0RowY) / 256.0));
        Edge1RowY = i32(round(f64(Temp1RowY) / 256.0));
        Edge2RowY = i32(round(f64(Temp2RowY) / 256.0));
    }

    Edge0RowY += IsTopLeft0 ? 0 : -1;
    Edge1RowY += IsTopLeft1 ? 0 : -1;
    Edge2RowY += IsTopLeft2 ? 0 : -1;
    
#if 0
    i32 Edge0RowY = CrossProduct2d(V2(MinX, MinY) + V2(0.5f, 0.5f) - PointA, Edge0);
    i32 Edge1RowY = CrossProduct2d(V2(MinX, MinY) + V2(0.5f, 0.5f) - PointB, Edge1);
    i32 Edge2RowY = CrossProduct2d(V2(MinX, MinY) + V2(0.5f, 0.5f) - PointC, Edge2);
#endif
        
    for (i32 Y = MinY; Y <= MaxY; ++Y)
    {
        i32 Edge0 = Edge0RowY;
        i32 Edge1 = Edge1RowY;
        i32 Edge2 = Edge2RowY;
        
        for (i32 X = MinX; X <= MaxX; ++X)
        {
            //if ((Edge0 > 0 || (IsTopLeft0 && Edge0 == 0.0f)) &&
            //    (Edge1 > 0 || (IsTopLeft1 && Edge1 == 0.0f)) &&
            //    (Edge2 > 0 || (IsTopLeft2 && Edge2 == 0.0f)))
            if (Edge0 >= 0 && Edge1 >= 0 && Edge2 >= 0)
            {
                // NOTE: Ми у середині трикутника
                u32 PixelId = Y * GlobalState.FrameBufferWidth + X;

                f32 T0 = (f32(-Edge0) / 16.0f) / BaryCentricDivisor;
                f32 T1 = (f32(-Edge1) / 16.0f) / BaryCentricDivisor;
                f32 T2 = (f32(-Edge2) / 16.0f) / BaryCentricDivisor;

                f32 DepthZ = T0 * Vertex0.Pos.z + T1 * Vertex1.Pos.z + T2 * Vertex2.Pos.z;
                if (DepthZ >= 0.0f && DepthZ <= 1.0f && DepthZ < GlobalState.DepthBuffer[PixelId])
                {
                    v3 FinalColor = T0 * ModelColor0 + T1 * ModelColor1 + T2 * ModelColor2;
                    FinalColor = FinalColor * 255.0f;
                    u32 FinalColorU32 = ((u32)0xFF << 24) | ((u32)FinalColor.r << 16) | ((u32)FinalColor.g << 8) | (u32)FinalColor.b;

                    GlobalState.FrameBufferPixels[PixelId] = 0xFF00FF00; //FinalColorU32;
                    GlobalState.DepthBuffer[PixelId] = DepthZ;
                }
            }
        
            Edge0 += Edge0DiffX;
            Edge1 += Edge1DiffX;
            Edge2 += Edge2DiffX;
        }

        Edge0RowY += Edge0DiffY;
        Edge1RowY += Edge1DiffY;
        Edge2RowY += Edge2DiffY;
    }
}

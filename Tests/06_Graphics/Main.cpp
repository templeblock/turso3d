// For conditions of distribution and use, see copyright notice in License.txt

#include "Turso3D.h"
#include "Debug/DebugNew.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#include <cstdio>
#include <cstdlib>

using namespace Turso3D;

class GraphicsTest : public Object
{
    OBJECT(GraphicsTest);

public:
    void Run()
    {
        RegisterGraphicsLibrary();
        RegisterResourceLibrary();

        cache = new ResourceCache();
        cache->AddResourceDir(ExecutableDir() + "Data");

        log = new Log();
        input = new Input();

        graphics = new Graphics();
        graphics->RenderWindow()->SetTitle("Graphics test");
        graphics->SetMode(640, 480, false, true);

        SubscribeToEvent(graphics->RenderWindow()->closeRequestEvent, &GraphicsTest::HandleCloseRequest);
        
        const size_t NUM_OBJECTS = 1000;

        float vertexData[] = {
            // Position             // Texcoord
            0.0f, 0.05f, 0.0f, 0.5f, 0.0f,
            0.05f, -0.05f, 0.0f, 1.0f, 1.0f,
            -0.05f, -0.05f, 0.0f, 0.0f, 1.0f
        };

        Vector<VertexElement> vertexDeclaration;
        vertexDeclaration.Push(VertexElement(ELEM_VECTOR3, SEM_POSITION));
        vertexDeclaration.Push(VertexElement(ELEM_VECTOR2, SEM_TEXCOORD));
        AutoPtr<VertexBuffer> vb = new VertexBuffer();
        vb->Define(USAGE_IMMUTABLE, 3, vertexDeclaration, false, vertexData);
        
        Vector<VertexElement> instanceVertexDeclaration;
        instanceVertexDeclaration.Push(VertexElement(ELEM_VECTOR3, SEM_TEXCOORD, 1, true));
        AutoPtr<VertexBuffer> ivb = new VertexBuffer();
        ivb->Define(USAGE_DYNAMIC, NUM_OBJECTS, instanceVertexDeclaration, false);

        unsigned short indexData[] = {
            0,
            1,
            2
        };

        AutoPtr<IndexBuffer> ib = new IndexBuffer();
        ib->Define(USAGE_IMMUTABLE, 3, sizeof(unsigned short), false, indexData);
        
        Constant pc(ELEM_VECTOR4, "Color");
        AutoPtr<ConstantBuffer> pcb = new ConstantBuffer();
        pcb->Define(USAGE_IMMUTABLE, 1, &pc);
        pcb->SetConstant("Color", Color::WHITE);
        pcb->Apply();

        String vsCode =
            "struct VOut"
            "{"
            "    float4 position : SV_POSITION;"
            "    float2 texCoord : TEXCOORD0;"
            "};"
            ""
            "VOut main(float3 position : POSITION, float2 texCoord : TEXCOORD0, float3 objectPosition : TEXCOORD1)"
            "{"
            "    VOut output;"
            "    output.position = float4(position + objectPosition, 1);"
            "    output.texCoord = texCoord;"
            "    return output;"
            "}";

        AutoPtr<Shader> vs = new Shader();
        vs->SetName("Test.vs");
        vs->Define(SHADER_VS, vsCode);
        ShaderVariation* vsv = vs->CreateVariation();

        String psCode =
            "cbuffer ConstantBuffer : register(b0)"
            "{"
            "   float4 Color;"
            "}"
            ""
            "Texture2D Texture : register(t0);"
            "SamplerState Sampler : register(s0);"
            ""
            "float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET"
            "{"
            "    return Color * Texture.Sample(Sampler, texCoord);"
            "}";

        AutoPtr<Shader> ps = new Shader();
        ps->SetName("Test.ps");
        ps->Define(SHADER_PS, psCode);
        ShaderVariation* psv = ps->CreateVariation();

        AutoPtr<BlendState> bs = new BlendState();
        bs->Define(false);
        AutoPtr<DepthState> ds = new DepthState();
        ds->Define(true);
        AutoPtr<RasterizerState> rs = new RasterizerState();
        rs->Define();

        Texture* tex = cache->LoadResource<Texture>("Test.png");
        
        for (;;)
        {
            input->Update();
            if (input->IsKeyPressed('F'))
                graphics->SetFullscreen(!graphics->IsFullscreen());
            if (input->IsKeyPressed(27))
                graphics->Close();

            // Drawing and state setting functions will not check Graphics initialization state, check now
            if (!graphics->IsInitialized())
                break;

            Vector3 instanceData[NUM_OBJECTS];
            for (size_t i = 0; i < NUM_OBJECTS; ++i)
                instanceData[i] = Vector3(Random() * 2.0f - 1.0f, Random() * 2.0f - 1.0f, 0.0f);
            ivb->SetData(0, NUM_OBJECTS, instanceData);

            graphics->Clear(CLEAR_COLOR | CLEAR_DEPTH, Color(0.0f, 0.0f, 0.5f));
            graphics->SetVertexBuffer(0, vb);
            graphics->SetVertexBuffer(1, ivb);
            graphics->SetIndexBuffer(ib);
            graphics->SetConstantBuffer(SHADER_PS, 0, pcb);
            graphics->SetShaders(vsv, psv);
            graphics->SetBlendState(bs);
            graphics->SetDepthState(ds);
            graphics->SetRasterizerState(rs);
            graphics->SetTexture(0, tex);

            graphics->DrawInstanced(TRIANGLE_LIST, 0, 3, 0, 0, NUM_OBJECTS);

            graphics->Present();
        }
    }

    void HandleCloseRequest(Event& /* event */)
    {
        graphics->Close();
    }

    AutoPtr<ResourceCache> cache;
    AutoPtr<Graphics> graphics;
    AutoPtr<Input> input;
    AutoPtr<Log> log;
};

int main()
{
    #ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif
    
    GraphicsTest test;
    test.Run();

    return 0;
}
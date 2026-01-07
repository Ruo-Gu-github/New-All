// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "pch.h"
#include <cstdio>

// ǰ������NanoVG��������
extern "C" {
    void Visualization_CleanupNanoVG();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        printf("====================================================================\n");
        printf("[DllVisualization] DLL_PROCESS_ATTACH - BUILD: %s %s\n", __DATE__, __TIME__);
        printf("[DllVisualization] 3D Renderer Colors:\n");
        printf("  PURPLE = ImageBrowserOrthogonal3DRenderer (threeDRendererKind=0/1)\n");
        printf("  CYAN   = RoiOrthogonal3DRenderer (threeDRendererKind=2)\n");
        printf("  ORANGE = ReconstructionRaycast3DRenderer (threeDRendererKind=3)\n");
        printf("  YELLOW = APR_RenderOrthogonal3D (RenderAllViews path)\n");
        printf("====================================================================\n");
        fflush(stdout);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        // ����NanoVG������
        Visualization_CleanupNanoVG();
        break;
    }
    return TRUE;
}


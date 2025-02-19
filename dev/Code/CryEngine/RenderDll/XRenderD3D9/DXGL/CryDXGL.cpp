/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Contains the definitions of the global functions defined in
//               CryDXGL


#include "StdAfx.h"
#include "Interfaces/CCryDXGLGIAdapter.hpp"
#include "Interfaces/CCryDXGLBlob.hpp"
#include "Interfaces/CCryDXGLDevice.hpp"
#include "Interfaces/CCryDXGLShaderReflection.hpp"
#include "Interfaces/CCryDXGLDeviceContext.hpp"
#include "Interfaces/CCryDXGLGIFactory.hpp"
#include "Implementation/GLShader.hpp"
#include "Implementation/GLDevice.hpp"
#if defined(ANDROID)
#include "Implementation/GLView.hpp"
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define CRYDXGL_CPP_SECTION_1 1
#define CRYDXGL_CPP_SECTION_2 2
#define CRYDXGL_CPP_SECTION_3 3
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYDXGL_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryDXGL_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryDXGL_cpp_provo.inl"
    #elif defined(AZ_PLATFORM_SALEM)
        #include "Salem/CryDXGL_cpp_salem.inl"
    #endif
#endif

template <typename Factory>
HRESULT CreateDXGIFactoryInternal(REFIID riid, void** ppFactory)
{
    if (riid == __uuidof(Factory))
    {
        CCryDXGLGIFactory* pFactory(new CCryDXGLGIFactory());
        if (!pFactory->Initialize())
        {
            pFactory->Release();
            *ppFactory = NULL;
            return E_FAIL;
        }
        CCryDXGLGIFactory::ToInterface(reinterpret_cast<IDXGIFactory**>(ppFactory), pFactory);
        return S_OK;
    }

    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3D11.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext)
{
    if (pAdapter == NULL)
    {
        // Get the first adapter enumerated by the factory according to the specification
        void* pvFactory(NULL);
        HRESULT iResult = CreateDXGIFactoryInternal<IDXGIFactory1>(__uuidof(IDXGIFactory1), &pvFactory);
        if (FAILED(iResult))
        {
            return iResult;
        }
        CCryDXGLGIFactory* pFactory(CCryDXGLGIFactory::FromInterface(static_cast<IDXGIFactory1*>(pvFactory)));
        iResult = pFactory->EnumAdapters(0, &pAdapter);
        pFactory->Release();
        if (FAILED(iResult))
        {
            return iResult;
        }
    }

    assert(pAdapter != NULL);
    CCryDXGLGIAdapter* pDXGLAdapter(CCryDXGLGIAdapter::FromInterface(pAdapter));

    D3D_FEATURE_LEVEL eDevFeatureLevel;
    if (pDXGLAdapter != NULL)
    {
        eDevFeatureLevel = pDXGLAdapter->GetSupportedFeatureLevel();
    }
    else
    {
        DXGL_TODO("Get the supported feature level even if no adapter is specified")
        eDevFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    }

    if (pFeatureLevels != NULL && FeatureLevels > 0)
    {
        D3D_FEATURE_LEVEL uMaxAllowedFL(*pFeatureLevels);
        while (FeatureLevels > 1)
        {
            ++pFeatureLevels;
            uMaxAllowedFL = max(uMaxAllowedFL, *pFeatureLevels);
            --FeatureLevels;
        }
        eDevFeatureLevel = min(eDevFeatureLevel, uMaxAllowedFL);
    }

    if (pFeatureLevel != NULL)
    {
        *pFeatureLevel = eDevFeatureLevel;
    }

    if (ppDevice != NULL)
    {
        _smart_ptr<CCryDXGLDevice> spDevice(new CCryDXGLDevice(pDXGLAdapter, eDevFeatureLevel));
        if (!spDevice->Initialize(pSwapChainDesc, ppSwapChain))
        {
            return E_FAIL;
        }

        CCryDXGLDevice::ToInterface(ppDevice, spDevice);

        if (ppImmediateContext != NULL)
        {
            (*ppDevice)->GetImmediateContext(ppImmediateContext);
        }
    }

    return S_OK;
}

DXGL_API HRESULT WINAPI D3D10CreateBlob(SIZE_T NumBytes, LPD3D10BLOB* ppBuffer)
{
    CCryDXGLBlob::ToInterface(ppBuffer, new CCryDXGLBlob(NumBytes));
    return (*ppBuffer)->GetBufferPointer() != NULL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3DCompiler.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI D3DReflect(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    REFIID pInterface,
    void** ppReflector)
{
    if (pInterface == IID_ID3D11ShaderReflection)
    {
        CCryDXGLShaderReflection* pReflection(new CCryDXGLShaderReflection());
        if (pReflection->Initialize(pSrcData))
        {
            CCryDXGLShaderReflection::ToInterface(reinterpret_cast<ID3D11ShaderReflection**>(ppReflector), pReflection);
            return S_OK;
        }
        pReflection->Release();
    }
    return E_FAIL;
}

DXGL_API HRESULT WINAPI D3DDisassemble(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    UINT Flags,
    LPCSTR szComments,
    ID3DBlob** ppDisassembly)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3DX11.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI D3DX11CreateTextureFromMemory(
    ID3D11Device* pDevice,
    const void* pSrcData,
    size_t SrcDataSize,
    D3DX11_IMAGE_LOAD_INFO* pLoadInfo,
    ID3DX11ThreadPump* pPump,
    ID3D11Resource** ppTexture,
    long* pResult)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

DXGL_API HRESULT WINAPI D3DX11SaveTextureToFile(ID3D11DeviceContext* pDevice,
    ID3D11Resource* pSrcResource,
    D3DX11_IMAGE_FILE_FORMAT fmt,
    const char* pDestFile)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

DXGL_API HRESULT WINAPI D3DX11CompileFromMemory(
    LPCSTR pSrcData,
    SIZE_T SrcDataLen,
    LPCSTR pFileName,
    CONST D3D10_SHADER_MACRO* pDefines,
    LPD3D10INCLUDE pInclude,
    LPCSTR pFunctionName,
    LPCSTR pProfile,
    UINT Flags1,
    UINT Flags2,
    ID3DX11ThreadPump* pPump,
    ID3D10Blob** ppShader,
    ID3D10Blob** ppErrorMsgs,
    HRESULT* pHResult)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in dxgi.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory)
{
    return CreateDXGIFactoryInternal<IDXGIFactory>(riid, ppFactory);
}

DXGL_API HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory)
{
    return CreateDXGIFactoryInternal<IDXGIFactory1>(riid, ppFactory);
}


////////////////////////////////////////////////////////////////////////////
//  Frame debugging functions
////////////////////////////////////////////////////////////////////////////

#define DXGL_PROFILE_USE_GREMEDY_STRING_MARKER 0
#if defined(ENABLE_FRAME_PROFILER_LABELS)
#   define DXGL_PROFILE_USE_KHR_DEBUG 1
#else
#   define DXGL_PROFILE_USE_KHR_DEBUG 0
#endif

#if DXGL_PROFILE_USE_GREMEDY_STRING_MARKER

template <size_t uSuffixSize>
struct SDebugStringBuffer
{
    enum
    {
        MAX_TEXT_LENGTH = 1024
    };
    char s_acBuffer[MAX_TEXT_LENGTH + uSuffixSize];

    SDebugStringBuffer(const char* szSuffix)
    {
        cryMemcpy(s_acBuffer + MAX_TEXT_LENGTH, szSuffix, uSuffixSize);
    }

    const char* Write(const char* szText)
    {
        size_t uTextLength(min((size_t)MAX_TEXT_LENGTH, strlen(szText)));
        char* szDest(s_acBuffer + MAX_TEXT_LENGTH - uTextLength);
        cryMemcpy(szDest, szText, uTextLength);
        return szDest;
    }
};

#define DXGL_DEBUG_STRING_BUFFER(_Suffix, _Name) SDebugStringBuffer<DXGL_ARRAY_SIZE(_Suffix)> _Name(_Suffix)

DXGL_DEBUG_STRING_BUFFER(": enter", g_kEnterDebugBuffer);
DXGL_DEBUG_STRING_BUFFER(": leave", g_kLeaveDebugBuffer);

#endif //DXGL_PROFILE_USE_GREMEDY_STRING_MARKER



DXGL_API void DXGLProfileLabel(const char* szName)
{
#if DXGL_PROFILE_USE_GREMEDY_STRING_MARKER && DXGL_EXTENSION_LOADER
    if (DXGL_GL_EXTENSION_SUPPORTED(GREMEDY_string_marker))
    {
        glStringMarkerGREMEDY(0, szName);
    }
#endif //DXGL_PROFILE_USE_GREMEDY_STRING_MARKER && DXGL_EXTENSION_LOADER
#if DXGL_PROFILE_USE_KHR_DEBUG && DXGL_SUPPORT_DEBUG_OUTPUT
    {
        if (glDebugMessageInsert)
        {
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, strlen(szName), szName);
        }
#if defined(OPENGL_ES)
        else if(glDebugMessageInsertKHR)
        {
            glDebugMessageInsertKHR(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, strlen(szName), szName);
        }
#endif
    }
#endif //DXGL_PROFILE_USE_KHR_DEBUG && DXGL_SUPPORT_DEBUG_OUTPUT
}

DXGL_API void DXGLProfileLabelPush(const char* szName)
{
#if DXGL_PROFILE_USE_GREMEDY_STRING_MARKER && DXGL_EXTENSION_LOADER
    if (DXGL_GL_EXTENSION_SUPPORTED(GREMEDY_string_marker))
    {
        glStringMarkerGREMEDY(0, g_kEnterDebugBuffer.Write(szName));
    }
#endif //DXGL_PROFILE_USE_GREMEDY_STRING_MARKER && DXGL_EXTENSION_LOADER
#if DXGL_PROFILE_USE_KHR_DEBUG && DXGL_SUPPORT_DEBUG_OUTPUT
    {
        if (glPushDebugGroup)
        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, strlen(szName), szName);
        }
#if defined(OPENGL_ES)
        else if(glPushDebugGroupKHR)
        {
            glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION, 0, strlen(szName), szName);
        }
#endif
    }
#endif //DXGL_PROFILE_USE_KHR_DEBUG && DXGL_SUPPORT_DEBUG_OUTPUT
#if defined(AZ_PLATFORM_MAC)
    glPushGroupMarkerEXT(0, szName);
#endif
}

DXGL_API void DXGLProfileLabelPop(const char* szName)
{
#if DXGL_PROFILE_USE_GREMEDY_STRING_MARKER && DXGL_EXTENSION_LOADER
    if (DXGL_GL_EXTENSION_SUPPORTED(GREMEDY_string_marker))
    {
        glStringMarkerGREMEDY(0, g_kLeaveDebugBuffer.Write(szName));
    }
#endif //DXGL_PROFILE_USE_GREMEDY_STRING_MARKER && DXGL_EXTENSION_LOADER
#if DXGL_PROFILE_USE_KHR_DEBUG && DXGL_SUPPORT_DEBUG_OUTPUT
    {
        if (glPopDebugGroup)
        {
            glPopDebugGroup();
        }
#if defined(OPENGL_ES)
        else if(glPopDebugGroupKHR)
        {
            glPopDebugGroupKHR();
        }
#endif
    }
#endif //DXGL_PROFILE_USE_KHR_DEBUG && DXGL_SUPPORT_DEBUG_OUTPUT
#if defined(AZ_PLATFORM_MAC)
    glPopGroupMarkerEXT();
#endif
}

inline CCryDXGLDevice* GetDXGLDevice(ID3D11Device* pDevice)
{
    return CCryDXGLDevice::FromInterface(pDevice);
}

inline CCryDXGLDeviceContext* GetDXGLDeviceContext(ID3D11DeviceContext* pDeviceContext)
{
    return CCryDXGLDeviceContext::FromInterface(pDeviceContext);
}

////////////////////////////////////////////////////////////////////////////
//  DXGL Extensions
////////////////////////////////////////////////////////////////////////////

#if !DXGL_FULL_EMULATION

#if defined(OPENGL_ES)
void DXGLSetColorDontCareActions(ID3D11RenderTargetView* const rtv,
    bool const loadDontCare,
    bool const storeDontCare)
{
    CRY_ASSERT(NULL != rtv);

    NCryOpenGL::SOutputMergerView* somv = rtv->GetGLView();
    CRY_ASSERT(somv);

    NCryOpenGL::SOutputMergerTextureView* somtv = somv->AsSOutputMergerTextureView();
    CRY_ASSERT(somtv);

    NCryOpenGL::STexture* tex = somtv->m_pTexture;
    CRY_ASSERT(tex);

    tex->m_bColorLoadDontCare = loadDontCare;
    tex->m_bColorStoreDontCare = storeDontCare;
}

void DXGLSetDepthDontCareActions(ID3D11DepthStencilView* const rtv,
    bool const loadDontCare,
    bool const storeDontCare)
{
    CRY_ASSERT(NULL != rtv);

    NCryOpenGL::SOutputMergerView* somv = rtv->GetGLView();
    CRY_ASSERT(somv);

    NCryOpenGL::SOutputMergerTextureView* somtv = somv->AsSOutputMergerTextureView();
    CRY_ASSERT(somtv);

    NCryOpenGL::STexture* tex = somtv->m_pTexture;
    CRY_ASSERT(tex);

    tex->m_bDepthLoadDontCare = loadDontCare;
    tex->m_bDepthStoreDontCare = storeDontCare;
}

void DXGLSetStencilDontCareActions(ID3D11DepthStencilView* const rtv,
    bool const loadDontCare,
    bool const storeDontCare)
{
    CRY_ASSERT(NULL != rtv);

    NCryOpenGL::SOutputMergerView* somv = rtv->GetGLView();
    CRY_ASSERT(somv);

    NCryOpenGL::SOutputMergerTextureView* somtv = somv->AsSOutputMergerTextureView();
    CRY_ASSERT(somtv);

    NCryOpenGL::STexture* tex = somtv->m_pTexture;
    CRY_ASSERT(tex);

    tex->m_bStencilLoadDontCare = loadDontCare;
    tex->m_bStencilStoreDontCare = storeDontCare;
}

void DXGLTogglePLS(ID3D11DeviceContext* pDeviceContext, bool const enable)
{
    if (DXGL_GL_EXTENSION_SUPPORTED(EXT_shader_pixel_local_storage))
    {
        NCryOpenGL::CContext* pGLContext(GetDXGLDeviceContext(pDeviceContext)->GetGLContext());
        pGLContext->TogglePLS(enable);
    }
}
#endif // defined(OPENGL_ES)

void DXGLInitializeIHVSpecifix()
{
    NCryOpenGL::SGlobalConfig::SetIHVDefaults();
}

void DXGLInitialize(uint32 uNumSharedContexts)
{
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYDXGL_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryDXGL_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryDXGL_cpp_provo.inl"
    #elif defined(AZ_PLATFORM_SALEM)
        #include "Salem/CryDXGL_cpp_salem.inl"
    #endif
#endif
    NCryOpenGL::SGlobalConfig::RegisterVariables();
    NCryOpenGL::CDevice::Configure(uNumSharedContexts);
}

void DXGLFinalize()
{
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYDXGL_CPP_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryDXGL_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryDXGL_cpp_provo.inl"
    #elif defined(AZ_PLATFORM_SALEM)
        #include "Salem/CryDXGL_cpp_salem.inl"
    #endif
#endif
}

void DXGLReserveContext(ID3D11Device* pDevice)
{
    GetDXGLDevice(pDevice)->GetGLDevice()->ReserveContext();
}

void DXGLReleaseContext(ID3D11Device* pDevice)
{
    GetDXGLDevice(pDevice)->GetGLDevice()->ReleaseContext();
}

void DXGLBindDeviceContext(ID3D11DeviceContext* pDeviceContext, bool bReserved)
{
    NCryOpenGL::CContext* pGLContext(GetDXGLDeviceContext(pDeviceContext)->GetGLContext());
    pGLContext->GetDevice()->BindContext(pGLContext);
    if (bReserved)
    {
        pGLContext->SetReservedContext(pGLContext);
        pGLContext->GetDevice()->ReserveContext();
    }
}

void DXGLUnbindDeviceContext(ID3D11DeviceContext* pDeviceContext, bool bReserved)
{
    NCryOpenGL::CContext* pGLContext(GetDXGLDeviceContext(pDeviceContext)->GetGLContext());
    if (bReserved)
    {
        pGLContext->GetDevice()->ReleaseContext();
    }
    pGLContext->GetDevice()->UnbindContext(pGLContext);
}

HRESULT DXGLMapBufferRange(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    NCryOpenGL::CContext* pGLContext(static_cast<CCryDXGLDeviceContext*>(pDeviceContext)->GetGLContext());
    NCryOpenGL::SBuffer* pGLBuffer(pBuffer->GetGLBuffer());
    return (*pGLBuffer->m_pfMapBufferRange)(pGLBuffer, uOffset, uSize, MapType, MapFlags, pMappedResource, pGLContext) ? S_OK : E_FAIL;
}

void DXGLSetDepthBoundsTest(bool bEnabled, float fMin, float fMax)
{
#if defined(GL_EXT_depth_bounds_test)
    if (bEnabled)
    {
        glEnable(GL_DEPTH_BOUNDS_TEST_EXT);
    }
    else
    {
        glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
    }
    glDepthBoundsEXT(fMin, fMax);
#else
    DXGL_WARNING("Depths Bounds Test extension not available on this platform");
#endif
}

void DXGLTogglePixelTracing(ID3D11DeviceContext* pDeviceContext, bool bEnable, uint32 uShaderHash, uint32 uPixelX, uint32 uPixelY)
{
#if DXGL_ENABLE_SHADER_TRACING
    GetDXGLDeviceContext(pDeviceContext)->GetGLContext()->TogglePixelTracing(bEnable, uShaderHash, uPixelX, uPixelY);
#endif //DXGL_ENABLE_SHADER_TRACING
}

void DXGLToggleVertexTracing(ID3D11DeviceContext* pDeviceContext, bool bEnable, uint32 uShaderHash, uint32 uVertexID)
{
#if DXGL_ENABLE_SHADER_TRACING
    GetDXGLDeviceContext(pDeviceContext)->GetGLContext()->ToggleVertexTracing(bEnable, uShaderHash, uVertexID);
#endif //DXGL_ENABLE_SHADER_TRACING
}

void DXGLCSSetConstantBuffers(ID3D11DeviceContext* pDeviceContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* pConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    GetDXGLDeviceContext(pDeviceContext)->CSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants);
}

void DXGLPSSetConstantBuffers(ID3D11DeviceContext* pDeviceContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* pConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    GetDXGLDeviceContext(pDeviceContext)->PSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants);
}

void DXGLVSSetConstantBuffers(ID3D11DeviceContext* pDeviceContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* pConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    GetDXGLDeviceContext(pDeviceContext)->VSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants);
}

void DXGLGSSetConstantBuffers(ID3D11DeviceContext* pDeviceContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* pConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    GetDXGLDeviceContext(pDeviceContext)->GSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants);
}

void DXGLHSSetConstantBuffers(ID3D11DeviceContext* pDeviceContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* pConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    GetDXGLDeviceContext(pDeviceContext)->HSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants);
}

void DXGLDSSetConstantBuffers(ID3D11DeviceContext* pDeviceContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* pConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    GetDXGLDeviceContext(pDeviceContext)->DSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants);
}

void DXGLIssueFrameFences(ID3D11Device* pDevice)
{
    GetDXGLDevice(pDevice)->GetGLDevice()->IssueFrameFences();
}

#endif //!DXGL_FULL_EMULATION

#if !defined(WIN32)
DXGL_API bool DXGLCreateWindow(
    const char* szTitle,
    uint32 uWidth,
    uint32 uHeight,
    bool bFullScreen,
    HWND* pHandle)
{
    return NCryOpenGL::CDevice::CreateWindow(szTitle, uWidth, uHeight, bFullScreen, pHandle);
}

DXGL_API void DXGLDestroyWindow(HWND kHandle)
{
    NCryOpenGL::CDevice::DestroyWindow(kHandle);
}
#endif // !defined(WIN32)

////////////////////////////////////////////////////////////////////////////
//  DxErr Logging and error functions
////////////////////////////////////////////////////////////////////////////

DXGL_API const char*  WINAPI DXGetErrorStringA(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return "";
}

DXGL_API const WCHAR* WINAPI DXGetErrorStringW(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return L"";
}

DXGL_API const char*  WINAPI DXGetErrorDescriptionA(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return "";
}

DXGL_API const WCHAR* WINAPI DXGetErrorDescriptionW(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return L"";
}

DXGL_API HRESULT WINAPI DXTraceA(const char* strFile, DWORD dwLine, HRESULT hr, const char* strMsg, BOOL bPopMsgBox)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

DXGL_API HRESULT WINAPI DXTraceW(const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

#if !DXGL_FULL_EMULATION

SDXGLContextThreadLocalHandle::SDXGLContextThreadLocalHandle()
{
    m_pTLSHandle = NCryOpenGL::CreateTLS();
}

SDXGLContextThreadLocalHandle::~SDXGLContextThreadLocalHandle()
{
    NCryOpenGL::DestroyTLS(m_pTLSHandle);
}

void SDXGLContextThreadLocalHandle::Set(ID3D11Device* pDevice)
{
    ID3D11Device* pPrevDevice(static_cast<ID3D11Device*>(NCryOpenGL::GetTLSValue(m_pTLSHandle)));
    if (pPrevDevice != pDevice)
    {
        if (pPrevDevice != NULL)
        {
            DXGLReleaseContext(pPrevDevice);
        }
        NCryOpenGL::SetTLSValue(m_pTLSHandle, pDevice);
        if (pDevice != NULL)
        {
            DXGLReserveContext(pDevice);
        }
    }
}

SDXGLDeviceContextThreadLocalHandle::SDXGLDeviceContextThreadLocalHandle()
{
    m_pTLSHandle = NCryOpenGL::CreateTLS();
}

SDXGLDeviceContextThreadLocalHandle::~SDXGLDeviceContextThreadLocalHandle()
{
    NCryOpenGL::DestroyTLS(m_pTLSHandle);
}

void SDXGLDeviceContextThreadLocalHandle::Set(ID3D11DeviceContext* pDeviceContext, bool bReserved)
{
    ID3D11DeviceContext* pPrevDeviceContext(static_cast<ID3D11DeviceContext*>(NCryOpenGL::GetTLSValue(m_pTLSHandle)));
    if (pPrevDeviceContext != pDeviceContext)
    {
        if (pPrevDeviceContext != NULL)
        {
            DXGLUnbindDeviceContext(pPrevDeviceContext, bReserved);
        }
        NCryOpenGL::SetTLSValue(m_pTLSHandle, pDeviceContext);
        if (pDeviceContext != NULL)
        {
            DXGLBindDeviceContext(pDeviceContext, bReserved);
        }
    }
}

#endif //!DXGL_FULL_EMULATION

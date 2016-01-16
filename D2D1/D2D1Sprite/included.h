#pragma once

#define  _WIN32_WINNT   0x0A01

#include <Windows.h>

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cwchar>
#include <cstddef>
#include <atomic>
#include <thread>
#include <new>

#include <dxgi1_4.h>
#include <D3D11.h>
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <dwrite_2.h>
#include <wincodec.h>

// DirectComposition 
#ifdef USING_DirectComposition
#   include <dcomp.h>
#endif

// demo
namespace Demo {
    template<class Interface>
    inline void SafeRelease(Interface *&pInterfaceToRelease){
        if (pInterfaceToRelease != nullptr){
            pInterfaceToRelease->Release();
            pInterfaceToRelease = nullptr;
        }
    }
    template <typename Interface>
    inline Interface* SafeAcquire(Interface* newObject)
    {
        if (newObject != nullptr)
            ((IUnknown*)newObject)->AddRef();

        return newObject;
    }
    inline void SafeCloseHandle(HANDLE& handle){
        if (handle){
            ::CloseHandle(handle);
            handle = nullptr;
        }
    }
    // the timer - high
    class HTimer {
    public:
        // QueryPerformanceCounter
        static inline auto QueryPerformanceCounter(LARGE_INTEGER* ll) noexcept {
            auto old = ::SetThreadAffinityMask(::GetCurrentThread(), 0);
            auto r = ::QueryPerformanceCounter(ll);
            ::SetThreadAffinityMask(::GetCurrentThread(), old);
            return r;
        }
        // refresh the frequency
        auto inline RefreshFrequency() noexcept { ::QueryPerformanceFrequency(&m_cpuFrequency); }
        // start timer
        auto inline Start() noexcept { HTimer::QueryPerformanceCounter(&m_cpuCounterStart); }
        // move end var to start var
        auto inline MovStartEnd() noexcept { m_cpuCounterStart = m_cpuCounterEnd; }
        // delta time in sec.
        template<typename T> auto inline Delta_s() noexcept {
            HTimer::QueryPerformanceCounter(&m_cpuCounterEnd);
            return static_cast<T>(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart) / static_cast<T>(m_cpuFrequency.QuadPart);
        }
        // delta time in ms.
        template<typename T> auto inline Delta_ms() noexcept {
            HTimer::QueryPerformanceCounter(&m_cpuCounterEnd);
            return static_cast<T>(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart)*static_cast<T>(1e3) / static_cast<T>(m_cpuFrequency.QuadPart);
        }
        // delta time in micro sec.
        template<typename T> auto inline Delta_mcs() noexcept {
            HTimer::QueryPerformanceCounter(&m_cpuCounterEnd);
            return static_cast<T>(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart)*static_cast<T>(1e6) / static_cast<T>(m_cpuFrequency.QuadPart);
        }
    private:
        // CPU freq
        LARGE_INTEGER            m_cpuFrequency;
        // CPU start counter
        LARGE_INTEGER            m_cpuCounterStart;
        // CPU end counter
        LARGE_INTEGER            m_cpuCounterEnd;
    public:
        // ctor
        HTimer() noexcept { m_cpuCounterStart.QuadPart = 0; m_cpuCounterEnd.QuadPart = 0; RefreshFrequency(); }
    };

#include <Mmsystem.h>
    // the timer : medium
    class MTimer {
    public:
        // refresh the frequency
        auto inline RefreshFrequency() noexcept { }
        // start timer
        auto inline Start() noexcept { m_dwStart = ::timeGetTime(); }
        // move end var to start var
        auto inline MovStartEnd() noexcept { m_dwStart = m_dwNow; }
        // delta time in sec.
        template<typename T> auto inline Delta_s() noexcept {
            m_dwNow = ::timeGetTime();
            return static_cast<T>(m_dwNow - m_dwStart) * static_cast<T>(0.001);
        }
        // delta time in ms.
        template<typename T> auto inline Delta_ms() noexcept {
            m_dwNow = ::timeGetTime();
            return static_cast<T>(m_dwNow - m_dwStart);
        }
        // delta time in micro sec.
        template<typename T> auto inline Delta_mcs() noexcept {
            m_dwNow = ::timeGetTime();
            return static_cast<T>(m_dwNow - m_dwStart) * static_cast<T>(1000);
        }
    private:
        // start time
        DWORD                   m_dwStart = 0;
        // now time
        DWORD                   m_dwNow = 0;
    public:
        // ctor
        MTimer() noexcept { this->Start(); }
    };
    // renderer
    class ImageRenderer {
    public:
        // ���캯��
        ImageRenderer() noexcept;
        // ��������
        ~ImageRenderer() noexcept;
        // ��Ⱦ֡
        auto OnRender(UINT syn) noexcept->HRESULT;
        // ������Ⱦģʽ
        void SetSpriteMode() noexcept { m_bSpriteMode = !m_bSpriteMode; }
        // ���ô��ھ��
        auto SetHwnd(HWND hwnd) noexcept { m_hwnd = hwnd; return CreateDeviceIndependentResources(); }
        // �����豸�޹���Դ
        auto CreateDeviceIndependentResources() noexcept->HRESULT;
        // �����豸�й���Դ
        auto CreateDeviceResources() noexcept->HRESULT;
        // �����豸�й���Դ
        void DiscardDeviceResources() noexcept;
        // ���ļ���ȡλͼ
        static auto LoadBitmapFromFile(
            ID2D1DeviceContext* IN pRenderTarget,
            IWICImagingFactory2* IN pIWICFactory,
            PCWSTR IN uri,
            UINT OPTIONAL width,
            UINT OPTIONAL height,
            ID2D1Bitmap1** OUT ppBitmap
            ) noexcept->HRESULT;
    private:
        // �ؽ�fps�ı�����
        void recreate_fps_layout() noexcept;
        // ��Ⱦ��ͼ
        void render_map() noexcept;
    private:
        // D2D ����
        ID2D1Factory4*                      m_pd2dFactory = nullptr;
        // WIC ����
        IWICImagingFactory2*                m_pWICFactory = nullptr;
        // DWrite����
        IDWriteFactory1*                    m_pDWriteFactory = nullptr;
        // �����ı���Ⱦ��ʽ
        IDWriteTextFormat*                  m_pTextFormatMain = nullptr;
        // FPS ��ʾ
        IDWriteTextLayout*                  m_pFPSLayout = nullptr;
        // �߾��ȼ�ʱ��
        HTimer                              m_oTimerH;
        // �о��ȼ�ʱ��
        MTimer                              m_oTimerM;
        // ֡������
        uint32_t                            m_cFrameCount = 0;
        // ֡���㵥λ
        uint32_t                            m_cRefreshCount = 30;
        // ��Ⱦģʽ
        std::atomic<bool>                   m_bSpriteMode = false;
        // ����
        bool                                m_unused[7];
        // ���ʱ��
        float                               m_fDelta = 0.f;
        // ƽ�� FPS
        float                               m_fFramePerSec = 1.f;
    private:
        // D3D �豸
        ID3D11Device*                       m_pd3dDevice = nullptr;
        // D3D �豸������
        ID3D11DeviceContext*                m_pd3dDeviceContext = nullptr;
        // D2D �豸
        ID2D1Device3*                       m_pd2dDevice = nullptr;
        // D2D �豸������
        ID2D1DeviceContext3*                m_pd2dDeviceContext = nullptr;
        // DXGI ����
        IDXGIFactory2*                      m_pDxgiFactory = nullptr;
        // DXGI �豸
        IDXGIDevice1*                       m_pDxgiDevice = nullptr;
        // DXGI ������
        IDXGIAdapter*                       m_pDxgiAdapter = nullptr;
#ifdef _DEBUG
        // ���Զ���
        ID3D11Debug*                        m_pd3dDebug = nullptr;
#endif
        // DXGI ������
        IDXGISwapChain2*                    m_pSwapChain = nullptr;
        // D2D ���鼯
        ID2D1SpriteBatch*                   m_pSpriteBatch = nullptr;
        // D2D λͼ ��ͼ��Դ��
        ID2D1Bitmap1*                       m_pMapAsset = nullptr;
        // D2D λͼ ���浱ǰ��ʾ��λͼ
        ID2D1Bitmap1*                       m_pd2dTargetBimtap = nullptr;
        // ��ɫ��ˢ
        ID2D1SolidColorBrush*               m_pBaiscBrush = nullptr;
        // ����λͼ
        ID2D1Bitmap1*                       m_pTestBitmap = nullptr;
        // ����ģ����Ч
        ID2D1Effect*                        m_pRadialBlurEffect = nullptr;
        // ����ģ����Ч ����ӿ�
        ID2D1Image*                         m_pRadialBlurOutput = nullptr;
#ifdef USING_DirectComposition
        // Direct Composition Device
        IDCompositionDevice*                m_pDcompDevice = nullptr;
        // Direct Composition Target
        IDCompositionTarget*                m_pDcompTarget = nullptr;
        // Direct Composition Visual
        IDCompositionVisual*                m_pDcompVisual = nullptr;
#endif
        // ���ھ��
        HWND                                m_hwnd = nullptr;
        //  �����豸���Եȼ�
        D3D_FEATURE_LEVEL                   m_featureLevel;
        // �ֶ�������
        DXGI_PRESENT_PARAMETERS             m_parameters;
    };
    // ThisApp
    class ThisApp{
    public:
        // ���캯��
        ThisApp() noexcept {};
        // ��������
        ~ThisApp() noexcept {};
        // ��ʼ��
        auto Initialize(HINSTANCE hInstance, int nCmdShow) noexcept ->HRESULT;
        // ��Ϣѭ��
        void RunMessageLoop() noexcept;
        // ���ڹ��̺���
        static auto CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept ->LRESULT;
    public:
        // �ر�ʱ
        auto OnClose() noexcept ->LRESULT;
        // �����ʱ
        auto OnClick() noexcept { m_oImagaRenderer.SetSpriteMode(); return LRESULT(1); }
    private:
        // ���ھ��
        HWND                        m_hwnd = nullptr;
        // ��Ⱦ��
        ImageRenderer               m_oImagaRenderer;
        // ��Ⱦ�����߳�
        std::thread                 m_threadRender;
        // �˳�
        std::atomic<BOOL>           m_bExit = FALSE;
    };
}
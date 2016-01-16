#include "included.h"

// config
namespace Demo {
    constexpr LONG WIDTH = 1600;
    constexpr LONG HEIGHT = 900;
    const wchar_t* const TITLE = L"Win10 TH2 Demo";
    template<typename R, typename T> constexpr auto lengthof(T& t) { 
        UNREFERENCED_PARAMETER(t); 
        return static_cast<R>(sizeof(t) / sizeof(*t));
    }
    template<typename T> constexpr auto lengthof(T& t) { 
        UNREFERENCED_PARAMETER(t); 
        return sizeof(t) / sizeof(*t); 
    }
    struct MapBase {
        uint32_t    width;
        uint32_t    height;
        uint32_t    unit_width;
        uint32_t    unit_height;
    };
    struct Map : MapBase {
        uint16_t    data[0];
    };
    Map*    g_mapData = nullptr;
    auto CreateSprites(const Map* map, ID2D1SpriteBatch* sprites) noexcept {
        assert(map && sprites && "bad arguments");
        sprites->Clear();
        HRESULT hr = S_OK;
        // ��Ӿ���
        const auto count = map->width * map->height;
        {
            D2D1_RECT_F rect = { 0.f };
            hr = sprites->AddSprites(count, &rect, nullptr, nullptr, nullptr, 0);
        }
        // �ɹ�
        if (SUCCEEDED(hr)) {
            for (uint32_t i = 0; i < count; ++i) {
                // ����Դ����
                uint32_t x = map->data[i] % map->width;
                uint32_t y = map->data[i] / map->width;
                D2D1_RECT_U src_rect;
                src_rect.left = x * map->unit_width;
                src_rect.top = y * map->unit_height;
                src_rect.right = src_rect.left + map->unit_width;
                src_rect.bottom = src_rect.top + map->unit_height;
                // ����Ŀ�����
                x = i % map->width;
                y = i / map->width;
                D2D1_RECT_F des_rect;
                des_rect.left = static_cast<float>(x * map->unit_width);
                des_rect.top = static_cast<float>(y * map->unit_height);
                des_rect.right = des_rect.left + static_cast<float>(map->unit_width);
                des_rect.bottom = des_rect.top +static_cast<float>(map->unit_height);
                // ����
                hr = sprites->SetSprites(i, 1, &des_rect, &src_rect);
                assert(SUCCEEDED(hr));
            }
        }
        return hr;
    }
}

// ��ʼ��
auto Demo::ThisApp::Initialize(HINSTANCE hInstance, int nCmdShow) noexcept ->HRESULT {
    HRESULT hr = E_FAIL;
    //register window class
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Demo::ThisApp::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(void*);
    wcex.hInstance = hInstance;
    wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"Direct2DTemplate";
    wcex.hIcon = nullptr;
    // ע�ᴰ��
    ::RegisterClassExW(&wcex);
    // ���㴰�ڴ�С
    RECT window_rect = { 0, 0, WIDTH, HEIGHT };
    DWORD window_style = WS_OVERLAPPEDWINDOW;
    ::AdjustWindowRect(&window_rect, window_style, FALSE);
    window_rect.right -= window_rect.left;
    window_rect.bottom -= window_rect.top;
    window_rect.left = (::GetSystemMetrics(SM_CXFULLSCREEN) - window_rect.right) / 2;
    window_rect.top = (::GetSystemMetrics(SM_CYFULLSCREEN) - window_rect.bottom) / 2;
    // ��������
    m_hwnd = ::CreateWindowExW(
#ifdef USING_DirectComposition
        WS_EX_NOREDIRECTIONBITMAP, 
#else
        0,
#endif
        wcex.lpszClassName, TITLE, window_style,
        window_rect.left, window_rect.top, window_rect.right, window_rect.bottom, 
        0, 0, hInstance, this
        );
    hr = m_hwnd ? S_OK : E_FAIL;
    // ���ô��ھ��
    if (SUCCEEDED(hr)) {
        hr = m_oImagaRenderer.SetHwnd(m_hwnd);
    }
    // ��ʾ����
    if (SUCCEEDED(hr)) {
        ::ShowWindow(m_hwnd, nCmdShow);
        ::UpdateWindow(m_hwnd);
        // �쳣
        try {
            m_threadRender.std::thread::~thread();
            m_threadRender.std::thread::thread([this]() noexcept {
                ::CoInitialize(nullptr);
                while (true) {
                    m_oImagaRenderer.OnRender(1);
                    if (m_bExit) break;
                }
                ::CoUninitialize();
            });
        }
        // ʧ��
        catch (...) {
            hr = E_FAIL;
        }
    }
    return hr;
}



// ��Ϣѭ��
void Demo::ThisApp::RunMessageLoop() noexcept {
    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
}


// ���ڹ��̺���
auto CALLBACK Demo::ThisApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept -> LRESULT {
    LRESULT result = 0;
    // ����ʱ ����ָ��
    if (message == WM_CREATE) {
        auto pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        auto pOurApp = reinterpret_cast<Demo::ThisApp*>(pcs->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, PtrToUlong(pOurApp));
        result = 1;
    }
    else {
        Demo::ThisApp *pOurApp = reinterpret_cast<Demo::ThisApp *>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(hwnd, GWLP_USERDATA))
            );
        // ������
        bool wasHandled = false;
        if (pOurApp){
            switch (message)
            {
            case WM_LBUTTONUP:
                result = pOurApp->OnClick();
                wasHandled = true;
                break;
            case WM_CLOSE:
                result = pOurApp->OnClose();
                wasHandled = true;
                break;
            case WM_DESTROY:
                ::PostQuitMessage(0);
                result = 1;
                wasHandled = true;
                break;
            }
        }
        // Ĭ�ϴ�����
        if (!wasHandled) {
            result = ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
    return result;
}


// �ر�ʱ��
auto Demo::ThisApp::OnClose() noexcept ->LRESULT {
    // ����β����(�����ȫ�����߳�)��������
    m_bExit = TRUE;
    // �ȴ��߳��˳�
    try { m_threadRender.join(); }
    catch (...) {}
    // �ݻٴ���
    ::DestroyWindow(m_hwnd);
    // �ͷ�����
    if (g_mapData) {
        std::free(g_mapData);
        g_mapData = nullptr;
    }
    // ����
    return LRESULT(1);
}

// Demo::ImageRenderer�๹�캯��
Demo::ImageRenderer::ImageRenderer() noexcept {
    m_parameters.DirtyRectsCount = 0;
    m_parameters.pDirtyRects = nullptr;
    m_parameters.pScrollRect = nullptr;
    m_parameters.pScrollOffset = nullptr;
}


// �����豸�޹���Դ
HRESULT Demo::ImageRenderer::CreateDeviceIndependentResources() noexcept {
    {
        static const MapBase s_map = { 128, 128, 32, 32 };
        g_mapData = reinterpret_cast<Map*>(
            std::malloc(sizeof(MapBase) + sizeof(uint16_t) * s_map.width * s_map.height)
                );
        if (g_mapData) {
            std::memcpy(g_mapData, &s_map, sizeof(s_map));
            for (auto i = 0ui32; i < s_map.width * s_map.height; ++i) {
                g_mapData->data[i] = std::rand() % 15;
            }
        }
    }
    // ����D2D����
    D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    HRESULT hr = ::D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        IID_ID2D1Factory1,
        &options,
        reinterpret_cast<void**>(&m_pd2dFactory)
        );
    // ���� WIC ����.
    if (SUCCEEDED(hr)) {
        hr = ::CoCreateInstance(
            CLSID_WICImagingFactory2,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_pWICFactory)
            );
    }
    // ���� DirectWrite ����.
    if (SUCCEEDED(hr)) {
        hr = ::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(m_pDWriteFactory),
            reinterpret_cast<IUnknown **>(&m_pDWriteFactory)
            );
    }
    // ���������ı���ʽ.
    if (SUCCEEDED(hr)) {
        hr = m_pDWriteFactory->CreateTextFormat(
            L"Courier New",
            //L"Fixedsys",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            22.f,
            L"en-us",
            &m_pTextFormatMain
            );
    }
    return hr;
}


// �����豸��Դ
HRESULT Demo::ImageRenderer::CreateDeviceResources() noexcept {
    HRESULT hr = S_OK;
    // DXGI Surface ��̨����
    IDXGISurface*                   pDxgiBackBuffer = nullptr;
    IDXGISwapChain1*                pSwapChain = nullptr;
    // ���� D3D11�豸���豸������ 
    if (SUCCEEDED(hr)) {
        // D3D11 ����flag 
        // һ��Ҫ��D3D11_CREATE_DEVICE_BGRA_SUPPORT
        // ���򴴽�D2D�豸�����Ļ�ʧ��
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        // Debug״̬ ��D3D DebugLayer�Ϳ���ȡ��ע��
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
        // �����豸
        hr = ::D3D11CreateDevice(
            // ��Ϊ��ָ��ѡ��Ĭ���豸
            nullptr,
            // ǿ��ָ��Ӳ����Ⱦ
            D3D_DRIVER_TYPE_HARDWARE,
            // ǿ��ָ��WARP��Ⱦ
            //D3D_DRIVER_TYPE_WARP,
            // û������ӿ�
            nullptr,
            // ����flag
            creationFlags,
            // ��ʹ�õ����Եȼ��б�
            featureLevels,
            // ���Եȼ��б���
            lengthof(featureLevels),
            // SDK �汾
            D3D11_SDK_VERSION,
            // ���ص�D3D11�豸ָ��
            &m_pd3dDevice,
            // ���ص����Եȼ�
            &m_featureLevel,
            // ���ص�D3D11�豸������ָ��
            &m_pd3dDeviceContext
            );
    }
#ifdef _DEBUG
    // ���� ID3D11Debug
    if (SUCCEEDED(hr)) {
        hr = m_pd3dDevice->QueryInterface(IID_PPV_ARGS(&m_pd3dDebug));
    }
#endif
    // ���� IDXGIDevice
    if (SUCCEEDED(hr)) {
        hr = m_pd3dDevice->QueryInterface(IID_PPV_ARGS(&m_pDxgiDevice));
    }
    // ����D2D�豸
    if (SUCCEEDED(hr)) {
        hr = m_pd2dFactory->CreateDevice(m_pDxgiDevice, &m_pd2dDevice);
    }
    // ����D2D�豸������
    if (SUCCEEDED(hr)) {
        hr = m_pd2dDevice->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
            &m_pd2dDeviceContext
            );
    }
    // ��ȡDxgi������ ���Ի�ȡ����������Ϣ
    if (SUCCEEDED(hr)) {
        // ˳��ʹ��������Ϊ��λ
        m_pd2dDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
        hr = m_pDxgiDevice->GetAdapter(&m_pDxgiAdapter);
    }
    // ��ȡDxgi����
    if (SUCCEEDED(hr)) {
        hr = m_pDxgiAdapter->GetParent(IID_PPV_ARGS(&m_pDxgiFactory));
    }
    // ����������
    if (SUCCEEDED(hr)) {
        RECT rect = { 0 }; ::GetClientRect(m_hwnd, &rect);
        // ��������Ϣ
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = rect.right - rect.left;
        swapChainDesc.Height = rect.bottom - rect.top;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.Flags = 0;
#ifdef USING_DirectComposition
        // DirectComposition����Ӧ�ó���
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        // ����DirectComposition������
        hr = m_pDxgiFactory->CreateSwapChainForComposition(
            m_pDxgiDevice,
            &swapChainDesc,
            nullptr,
            &pSwapChain
            );
#else
        // һ������Ӧ�ó���
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        // ���ô��ھ������������
        hr = m_pDxgiFactory->CreateSwapChainForHwnd(
            m_pd3dDevice,
            m_hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &pSwapChain
            );
#endif
    }
    // ��ȡ�ӿ�
    if (SUCCEEDED(hr)) {
        hr = pSwapChain->QueryInterface(
            IID_IDXGISwapChain2,
            reinterpret_cast<void**>(&m_pSwapChain)
            );
    }
    // ȷ��DXGI������߲��ᳬ��һ֡
    if (SUCCEEDED(hr)) {
        hr = m_pDxgiDevice->SetMaximumFrameLatency(1);
    }
    // ���ý�������ȡDxgi����
    if (SUCCEEDED(hr)) {
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pDxgiBackBuffer));
    }
    // ����Dxgi���洴��λͼ
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f
            );
        hr = m_pd2dDeviceContext->CreateBitmapFromDxgiSurface(
            pDxgiBackBuffer,
            &bitmapProperties,
            &m_pd2dTargetBimtap
            );
    }
    // ����
    if (SUCCEEDED(hr)) {
        // ���� Direct2D ��ȾĿ��
        m_pd2dDeviceContext->SetTarget(m_pd2dTargetBimtap);
        // ʹ��������Ϊ��λ
        m_pd2dDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    }
#ifdef USING_DirectComposition
    // ����ֱ�����(Direct Composition)�豸
    if (SUCCEEDED(hr)) {
        hr = DCompositionCreateDevice(
            /*static_cast<ID2D1Device*>(UIRenderer)*/nullptr,
            IID_PPV_ARGS(&m_pDcompDevice)
            );
    }
    // ����ֱ�����(Direct Composition)Ŀ��
    if (SUCCEEDED(hr)) {
        hr = m_pDcompDevice->CreateTargetForHwnd(
            m_hwnd, true, &m_pDcompTarget
            );
    }
    // ����ֱ�����(Direct Composition)�Ӿ�
    if (SUCCEEDED(hr)) {
        hr = m_pDcompDevice->CreateVisual(&m_pDcompVisual);
    }
    // ���õ�ǰ������Ϊ�Ӿ�����
    if (SUCCEEDED(hr)) {
        hr = m_pDcompVisual->SetContent(m_pSwapChain);
    }
    // ���õ�ǰ�Ӿ�Ϊ����Ŀ��
    if (SUCCEEDED(hr)) {
        hr = m_pDcompTarget->SetRoot(m_pDcompVisual);
    }
    // ��ϵͳ�ύ
    if (SUCCEEDED(hr)) {
        hr = m_pDcompDevice->Commit();
    }
#endif
    // ������ˢ
    if (SUCCEEDED(hr)) {
        hr = m_pd2dDeviceContext->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Black),
            &m_pBaiscBrush
            );
    }
    // �������鼯
    if (SUCCEEDED(hr)) {
        hr = m_pd2dDeviceContext->CreateSpriteBatch(&m_pSpriteBatch);
    }
    // ��������
    if (SUCCEEDED(hr)) {
        hr = Demo::CreateSprites(g_mapData, m_pSpriteBatch);
    }
    // �����ͼ��Դ��
    if (SUCCEEDED(hr)) {
        hr = this->LoadBitmapFromFile(
            m_pd2dDeviceContext,
            m_pWICFactory,
            L"PathAndObjects.png", 0, 0,
            &m_pMapAsset
            );
    }
    Demo::SafeRelease(pDxgiBackBuffer);
    Demo::SafeRelease(pSwapChain);
    return hr;
}

// Demo::ImageRenderer��������
Demo::ImageRenderer::~ImageRenderer() noexcept {
    this->DiscardDeviceResources();
    Demo::SafeRelease(m_pd2dFactory);
    Demo::SafeRelease(m_pWICFactory);
    Demo::SafeRelease(m_pDWriteFactory);
    Demo::SafeRelease(m_pTextFormatMain);
    Demo::SafeRelease(m_pFPSLayout);
    // ����
#ifdef _DEBUG
    if (m_pd3dDebug) {
        m_pd3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
        auto c = m_pd3dDebug->Release();
        ::OutputDebugStringW(L"Sad\r\n");
    }
    //Demo::SafeRelease(m_pd3dDebug);
#endif
}

// �����豸�����Դ
void Demo::ImageRenderer::DiscardDeviceResources() noexcept {
#ifdef USING_DirectComposition
    Demo::SafeRelease(m_pDcompDevice);
    Demo::SafeRelease(m_pDcompTarget);
    Demo::SafeRelease(m_pDcompVisual);
#endif
    Demo::SafeRelease(m_pMapAsset);
    Demo::SafeRelease(m_pBaiscBrush);
    Demo::SafeRelease(m_pSpriteBatch);
    Demo::SafeRelease(m_pd2dTargetBimtap);
    Demo::SafeRelease(m_pSwapChain);
    Demo::SafeRelease(m_pd3dDeviceContext);
    Demo::SafeRelease(m_pd2dDeviceContext);
    Demo::SafeRelease(m_pDxgiAdapter);
    Demo::SafeRelease(m_pd3dDevice);
    Demo::SafeRelease(m_pd2dDevice);
    Demo::SafeRelease(m_pDxgiFactory);
    Demo::SafeRelease(m_pDxgiDevice);
}

// �ؽ�FPS����
void Demo::ImageRenderer::recreate_fps_layout() noexcept {
    ++m_cFrameCount;
    constexpr size_t BUFFER_COUNT = 128;
    // ����FPS
    m_fDelta = m_oTimerH.Delta_s<float>();
    m_oTimerH.MovStartEnd();
    // �ͷ�����
    Demo::SafeRelease(m_pFPSLayout);
    assert(m_pTextFormatMain && "bad initialize");
    wchar_t buffer[BUFFER_COUNT];
    // ƽ��FPS
    if (!(m_cFrameCount % m_cRefreshCount)) {
        m_fFramePerSec = static_cast<float>(m_cRefreshCount) / m_oTimerM.Delta_s<float>();
        m_oTimerH.RefreshFrequency();
        m_oTimerM.RefreshFrequency();
        m_oTimerM.MovStartEnd();
    }
    // ��ʽ��
    auto c = std::swprintf(
        buffer, BUFFER_COUNT,
        L"%6.2f fps(c) %6.2f fps(m) @%ld -- MODE: %ls -- click window clent zone to change mode",
        1.f / m_fDelta, 
        m_fFramePerSec,
        long(m_cFrameCount),
        m_bSpriteMode ? L"DRAW SPRITE" : L"DRAW BITMAP"
        );
    assert(c > 0 && "bad std::swprintf call");
    // �����ı�����
    auto size = m_pd2dTargetBimtap->GetSize();
    m_pDWriteFactory->CreateTextLayout(
        buffer, static_cast<UINT32>(c),
        m_pTextFormatMain,
        size.width, size.height,
        &m_pFPSLayout
        );
}

// ��Ⱦ��ͼ
void Demo::ImageRenderer::render_map() noexcept {
    D2D1_MATRIX_3X2_F matrix;
    m_pd2dDeviceContext->GetTransform(&matrix);
    const float zoom = float(m_cFrameCount % (m_cRefreshCount * 4)) / float(m_cRefreshCount * 2);
    m_pd2dDeviceContext->SetTransform(
        D2D1::Matrix3x2F::Scale(D2D1::SizeF(zoom, zoom)) *
        D2D1::Matrix3x2F::Translation(D2D1::SizeF(0.f, 32.f)) * matrix
        );
    m_pd2dDeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    if (m_bSpriteMode) {
        m_pd2dDeviceContext->DrawSpriteBatch(m_pSpriteBatch, m_pMapAsset);
    }
    else {
        auto draw = [this](Demo::Map* map) noexcept {
            for (uint32_t i = 0; i < map->width * map->height; ++i) {
                // ����Դ����
                uint32_t x = map->data[i] % map->width;
                uint32_t y = map->data[i] / map->width;
                D2D1_RECT_F src_rect;
                src_rect.left = static_cast<float>(x * map->unit_width);
                src_rect.top = static_cast<float>(y * map->unit_height);
                src_rect.right = src_rect.left + static_cast<float>(map->unit_width);
                src_rect.bottom = src_rect.top + static_cast<float>(map->unit_height);
                // ����Ŀ�����
                x = i % map->width;
                y = i / map->width;
                D2D1_RECT_F des_rect;
                des_rect.left = static_cast<float>(x * map->unit_width);
                des_rect.top = static_cast<float>(y * map->unit_height);
                des_rect.right = des_rect.left + static_cast<float>(map->unit_width);
                des_rect.bottom = des_rect.top + static_cast<float>(map->unit_height);
                // ����
                m_pd2dDeviceContext->DrawBitmap(
                    m_pMapAsset,
                    &des_rect, 1.f,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                    &src_rect
                    );
            }
        };
        draw(g_mapData);
    }
    m_pd2dDeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    m_pd2dDeviceContext->SetTransform(&matrix);
}

// ��Ⱦͼ��ͼ��
HRESULT Demo::ImageRenderer::OnRender(UINT syn) noexcept {
    HRESULT hr = S_OK;
    // û�оʹ���
    if (!m_pd2dDeviceContext) {
        hr = this->CreateDeviceResources();
        assert(SUCCEEDED(hr));
        m_oTimerH.Start();
        m_oTimerM.Start();
    }
    // �ɹ�����Ⱦ
    if (SUCCEEDED(hr)) {
        // �ؽ�
        this->recreate_fps_layout();
        // ��ʼ��Ⱦ
        m_pd2dDeviceContext->BeginDraw();
        // ����
        m_pd2dDeviceContext->Clear(D2D1::ColorF(0x66CCFF, 0.5f));
        // ��Ⱦ��ͼ
        this->render_map();
        // ��ȾFPS
        if (m_pFPSLayout) {
            m_pd2dDeviceContext->DrawTextLayout(
                D2D1::Point2F(),
                m_pFPSLayout,
                m_pBaiscBrush
                );
        }
        // ������Ⱦ
        m_pd2dDeviceContext->EndDraw();
        // ����Ŀ��
        hr = m_pSwapChain->Present(syn, 0);
    }
    // �豸��ʧ?
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        this->DiscardDeviceResources();
        hr = S_FALSE;
    }
    return hr;
}


// ���ļ���ȡλͼ
auto Demo::ImageRenderer::LoadBitmapFromFile(
    ID2D1DeviceContext* IN pRenderTarget,
    IWICImagingFactory2* IN pIWICFactory,
    PCWSTR IN uri,
    UINT OPTIONAL width,
    UINT OPTIONAL height,
    ID2D1Bitmap1** OUT ppBitmap
    ) noexcept -> HRESULT {
    IWICBitmapDecoder *pDecoder = nullptr;
    IWICBitmapFrameDecode *pSource = nullptr;
    IWICStream *pStream = nullptr;
    IWICFormatConverter *pConverter = nullptr;
    IWICBitmapScaler *pScaler = nullptr;

    HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
        uri,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder
        );

    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }


    if (SUCCEEDED(hr)) {
        if (width != 0 || height != 0) {
            UINT originalWidth, originalHeight;
            hr = pSource->GetSize(&originalWidth, &originalHeight);
            if (SUCCEEDED(hr)) {
                if (width == 0) {
                    FLOAT scalar = static_cast<FLOAT>(height) / static_cast<FLOAT>(originalHeight);
                    width = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
                }
                else if (height == 0) {
                    FLOAT scalar = static_cast<FLOAT>(width) / static_cast<FLOAT>(originalWidth);
                    height = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
                }

                hr = pIWICFactory->CreateBitmapScaler(&pScaler);
                if (SUCCEEDED(hr)) {
                    hr = pScaler->Initialize(
                        pSource,
                        width,
                        height,
                        WICBitmapInterpolationModeCubic
                        );
                }
                if (SUCCEEDED(hr))  {
                    hr = pConverter->Initialize(
                        pScaler,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapDitherTypeNone,
                        nullptr,
                        0.f,
                        WICBitmapPaletteTypeMedianCut
                        );
                }
            }
        }
        else {
            hr = pConverter->Initialize(
                pSource,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.f,
                WICBitmapPaletteTypeMedianCut
                );
        }
    }
    if (SUCCEEDED(hr))  {
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            pConverter,
            nullptr,
            ppBitmap
            );
    }

    Demo::SafeRelease(pDecoder);
    Demo::SafeRelease(pSource);
    Demo::SafeRelease(pStream);
    Demo::SafeRelease(pConverter);
    Demo::SafeRelease(pScaler);

    return hr;
}

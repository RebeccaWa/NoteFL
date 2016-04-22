#include "pfdUIMap.h"
#include "pfdAlgorithm.h"
// longui api
#include <Control/UIContainer.h>
#include <Platonly/luiPoFile.h>
#include <Core/luiManager.h>
// windows api
#include <Shobjidl.h>
#include <Shlwapi.h>


/// <summary>
/// Initializes a new instance of the <see cref="UIMapControl"/> class.
/// </summary>
/// <param name="cp">The cp.</param>
PathFD::UIMapControl::UIMapControl(LongUI::UIContainer * cp) noexcept : Super(cp) {
    std::memset(m_bufCharData, 0, sizeof(m_bufCharData));
    // ��ͼ����
    m_dataMap.cell_width = CELL_WIDTH_INIT;
    m_dataMap.cell_height = CELL_HEIGHT_INIT;
    // ��ɫ����
    auto& chardata = this->get_char_data();
    chardata.width = 32;
    chardata.height = 32;
    chardata.atime = 0.5f;
    chardata.speed = 4.f;
    chardata.acount = 4;
    chardata.action[0] = 1;
    chardata.action[1] = 0;
    chardata.action[2] = 1;
    chardata.action[3] = 2;
}

/// <summary>
/// Initializes the specified node.
/// </summary>
/// <param name="node">The node.</param>
/// <returns></returns>
void PathFD::UIMapControl::initialize(pugi::xml_node node) noexcept {
    // ��ʽ����
    Super::initialize(node);
    const char* str = nullptr;
    // ��ȡ����ʱ��
    if ((str = node.attribute("steptime").value())) {
        this->SetStepDeltaTime(LongUI::AtoF(str));
    }
    // ��ȡ
    if ((str = node.attribute("charbitmap").value())) {
        m_uCharBitmap = static_cast<uint16_t>(LongUI::AtoI(str));
    }
    // ��ȡ
    if ((str = node.attribute("mapbitmap").value())) {
        m_uMapBitmap = static_cast<uint16_t>(LongUI::AtoI(str));
    }
    // ��ȡ
    if ((str = node.attribute("mapicon").value())) {
        m_idMapIcon = static_cast<uint16_t>(LongUI::AtoI(str));
    }
    assert(m_uCharBitmap && m_uMapBitmap && m_idMapIcon);
    assert(!m_pFileOpenDialog);
    assert(!m_pFileSaveDialog);
    auto hr = S_OK;
    // �����ı��Ի���
    if (SUCCEEDED(hr)) {
        hr = ::CoCreateInstance(
            CLSID_FileOpenDialog,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IFileDialog,
            reinterpret_cast<void**>(&m_pFileOpenDialog)
        );
    }
    // �����ı��Ի���
    if (SUCCEEDED(hr)) {
        hr = ::CoCreateInstance(
            CLSID_FileSaveDialog,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IFileDialog,
            reinterpret_cast<void**>(&m_pFileSaveDialog)
        );
    }
    // ����ȷ����ǩ
    if (SUCCEEDED(hr)) {
        hr = m_pFileSaveDialog->SetOkButtonLabel(L"�����ͼ�ļ�");
    }
    // ���ñ���
    if (SUCCEEDED(hr)) {
        hr = m_pFileSaveDialog->SetTitle(L"PathFD - ѡ�񱣴���ļ�");
    }
    // ����ȷ����ǩ
    if (SUCCEEDED(hr)) {
        hr = m_pFileOpenDialog->SetOkButtonLabel(L"�򿪵�ͼ�ļ�");
    }
    // ���ñ���
    if (SUCCEEDED(hr)) {
        hr = m_pFileOpenDialog->SetTitle(L"PathFD - ѡ��򿪵��ļ�");
    }
    // �����ļ�����
    if (SUCCEEDED(hr)) {
        COMDLG_FILTERSPEC filter = { L"PathFD ��ͼ�ļ�", L"*.map" };
        hr = m_pFileSaveDialog->SetFileTypes(1, &filter);
    }
    // ������չ��
    if (SUCCEEDED(hr)) {
        hr = m_pFileSaveDialog->SetDefaultExtension(L"map");
    }
    // ������
    if (FAILED(hr)) {
        assert(!"HR!");
        UIManager.ShowError(hr);
    }
}

// ------------------------- ��ͼ�߼�

// ��ͼ
void PathFD::UIMapControl::ResizeCellSize(uint32_t width, uint32_t height) noexcept {
    assert(width && height && "bad arguments");
    m_dataMap.cell_width = width;
    m_dataMap.cell_height = height;
    assert(!"NOIMPL!");
}


/// <summary>
/// ���ɵ�ͼ
/// </summary>
/// <param name="width">The width.</param>
/// <param name="height">The height.</param>
/// <returns></returns>
void PathFD::UIMapControl::GenerateMap(uint32_t width, uint32_t height) noexcept {
    // ��Ч��SB
    if (!m_pMapSpriteBatch) return;
    // �ڴ治��
    if (!this->require_mapdata(width, height)) return;
    // ���
    std::memset(m_pMapCells, 0, sizeof(m_pMapCells[0]) * (width * height));
    uint32_t pos[2] = { 0 };
    // ���ɵ�ͼ
    m_fnGeneration(m_pMapCells, width, height, pos);
    m_dataMap.map_data = m_pMapCells;
    m_dataMap.char_x = pos[0] % m_dataMap.map_width;
    m_dataMap.char_y = pos[0] / m_dataMap.map_width;
    m_uGoalX = pos[1] % m_dataMap.map_width;
    m_uGoalY = pos[1] / m_dataMap.map_width;
    // ���õ�ͼ
    this->reset_map();
}


/// <summary>
/// �����ͼ��Ч��
/// </summary>
/// <param name="width">The width.</param>
/// <param name="height">The height.</param>
/// <returns></returns>
bool PathFD::UIMapControl::require_mapdata(int32_t width, uint32_t height) noexcept {
    assert(width > 1 && height > 1 && "bad arguments");
    assert(width <= MAX_WIDTH && height <= MAX_HEIGHT && "bad arguments");
    m_dataMap.map_width = width;
    m_dataMap.map_height = height;
    auto sz = width * height;
    // ��Ҫ������������
    if (m_uMapSpriteCount < sz) {
        LongUI::NormalFree(m_pMapCells);
        m_pMapCells = LongUI::NormalAllocT<uint8_t>(sz);
        // �ڴ治��
        if (!m_pMapCells) return false;
        auto added = sz - m_uMapSpriteCount ;
        D2D1_RECT_F rect = { 0.f };
        // ���뾫��
        auto hr = m_pMapSpriteBatch->AddSprites(
            added,
            &rect, nullptr, nullptr, nullptr,
            0,        0,     0,       0
        );
        // ʧ��
        if (FAILED(hr)) {
            m_pMapSpriteBatch->Release();
            m_pMapSpriteBatch = nullptr;
            UIManager.ShowError(hr);
            return false;
        }
        m_uMapSpriteCount = sz;
    }
    return true;
}

/// <summary>
/// ���õ�ͼ
/// <returns></returns>
void PathFD::UIMapControl::reset_map() noexcept {
    // ���ý�ɫ��ͼ
    m_char.ResetMap(m_dataMap);
    // ���ÿؼ���С
    this->SetWidth(float(m_dataMap.cell_width * m_dataMap.map_width));
    this->SetHeight(float(m_dataMap.cell_height * m_dataMap.map_height));
    // ���õ�ͼ����
    this->reset_sprites();
    // �ػ��ͼ
    this->parent->SetControlLayoutChanged();
    this->parent->InvalidateThis();
}

/// <summary>
/// ���þ���
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::reset_sprites() noexcept {
    // ��Ч
    if (!m_pMapSpriteBatch) return;
    float cllw = float(m_dataMap.cell_width);
    float cllh = float(m_dataMap.cell_height);
    // Դ����
    D2D1_RECT_U srcs[] = {
        { 
            0, 
            0, 
            m_dataMap.cell_width, 
            m_dataMap.cell_height 
        },
        { 
            m_dataMap.cell_width, 
            0, 
            m_dataMap.cell_width + m_dataMap.cell_width, 
            m_dataMap.cell_height
        },
    };
    // TODO: SetSpriteSSSSS
    // ��������
    D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::White, 1.f);
    for (uint32_t y = 0; y != m_dataMap.map_height; ++y) {
        for (uint32_t x = 0; x < m_dataMap.map_width; ++x) {
            uint32_t index = x + y * m_dataMap.map_width;
            D2D1_RECT_F des;
            // ����Ŀ�����
            des.left = float(x) * cllw;
            des.top = float(y) * cllh;
            des.right =  des.left +  cllw;
            des.bottom = des.top + cllh;
            // ����
            m_pMapSpriteBatch->SetSprites(
                index, 1,
                &des, srcs + m_pMapCells[index],
                &color, nullptr, 0, 0, 0, 0
            );
        }
    }
}

// ------------------------- ��ͼ����


/// <summary>
/// �����ؼ�
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::cleanup() noexcept {
    // ɾǰ����
    this->before_deleted();
    // �ͷſռ�
    delete this;
}

/// <summary>
/// �ͷ��豸��Դ
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::release_resource() noexcept {
    LongUI::SafeRelease(m_pCellBoundaryBrush);
    LongUI::SafeRelease(m_pMapSpriteBatch);
    LongUI::SafeRelease(m_pAutoTileCache);
    LongUI::SafeRelease(m_pNumnberTable);
    LongUI::SafeRelease(m_pPathDisplay);
    LongUI::SafeRelease(m_pMapIcon);
    LongUI::SafeRelease(m_pMapSkin);
    
}

/// <summary>
/// <see cref="UIMapControl"/> ��������
/// </summary>
/// <returns></returns>
PathFD::UIMapControl::~UIMapControl() noexcept {
    // �ͷ���Դ
    this->release_resource();
    // �ͷŶԻ���
    LongUI::SafeRelease(m_pFileOpenDialog);
    LongUI::SafeRelease(m_pFileSaveDialog);
    // �ͷ��㷨
    if (m_pAlgorithm) {
        m_pAlgorithm->Dispose();
        m_pAlgorithm = nullptr;
    }
    // �ͷ�����
    if (m_pMapCells) {
        LongUI::NormalFree(m_pMapCells);
        m_pMapCells = nullptr;
    }
    // �ͷ�·��
    if (m_pPath) {
        std::free(m_pPath);
        m_pPath = nullptr;
    }
}

/// <summary>
/// ��Ⱦ��-��Ⱦ����
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::render_chain_background() const noexcept {
    // ���౳��
    Super::render_chain_background();
    // ǿ��ˢ��
    UIManager_RenderTarget->Flush();
    // ���౳��
    if (!m_pMapSpriteBatch || !m_uMapSpriteCount) return;
    // ��������
#ifdef PATHFD_ALIGNED
    D2D1_MATRIX_3X2_F transform1, transform2; 
    UIManager_RenderTarget->GetTransform(&transform1);
    transform2 = transform1;
#if 1
    transform2._31 = float(int(transform1._31));
    transform2._32 = float(int(transform1._32));
#else
    transform2._31 = std::floor(transform1._31);
    transform2._32 = std::floor(transform1._32);
#endif
    UIManager_RenderTarget->SetTransform(&transform2);
#endif
    // ���鼯��Ҫȡ�������ģʽ
    UIManager_RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    // ��Ⱦ���鼯
    UIManager_RenderTarget->DrawSpriteBatch(
        m_pMapSpriteBatch, 
        0, m_dataMap.map_width * m_dataMap.map_height,
        m_pMapIcon,
        D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
    );
    // ��ͼ�ֽ���
    UIManager_RenderTarget->FillRectangle(
        D2D1::RectF(
            0.f,
            0.f,
            float(m_dataMap.cell_width * m_dataMap.map_width),
            float(m_dataMap.cell_height * m_dataMap.map_height)
        ),
        m_pCellBoundaryBrush
    );
    // ��Ⱦ���
    {
        D2D1_RECT_F des;
        // Ŀ�����
        des.left = float(m_dataMap.char_x * m_dataMap.cell_width);
        des.top = float(m_dataMap.char_y * m_dataMap.cell_height);
        des.right =  des.left + float(m_dataMap.cell_width);
        des.bottom = des.top + float(m_dataMap.cell_height);
        // Դ����
        D2D1_RECT_F src;
        src.left = float(m_dataMap.cell_height) * 2.f;
        src.top = 0.f;
        src.right = src.left + float(m_dataMap.cell_width);
        src.bottom = src.top + float(m_dataMap.cell_height);
        // �̻�
        UIManager_RenderTarget->DrawBitmap(
            m_pMapIcon,
            &des,
            1.f,
            //D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            &src
        );
    }
    // ��Ⱦ�յ�
    {
        D2D1_RECT_F des;
        // Ŀ�����
        des.left = float(m_uGoalX * m_dataMap.cell_width);
        des.top = float(m_uGoalY * m_dataMap.cell_height);
        des.right =  des.left + float(m_dataMap.cell_width);
        des.bottom = des.top + float(m_dataMap.cell_height);
        // Դ����
        D2D1_RECT_F src;
        src.left = float(m_dataMap.cell_height) * 3.f;
        src.top = 0.f;
        src.right = src.left + float(m_dataMap.cell_width);
        src.bottom = src.top + float(m_dataMap.cell_height);
        // �̻�
        UIManager_RenderTarget->DrawBitmap(
            m_pMapIcon,
            &des,
            1.f,
            //D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            &src
        );
    }
    // ��Ⱦ��ɫ
    m_char.Render();
    // ��Ⱦѡ���
    if (m_uClickX < m_dataMap.map_width && m_uClickY < m_dataMap.map_height) {
        D2D1_RECT_F rect;
        // ����
        rect.left = float(m_uClickX * m_dataMap.cell_width) + CEEL_SELECT_WIDTH * 0.5f;
        rect.top = float(m_uClickY * m_dataMap.cell_height) + CEEL_SELECT_WIDTH * 0.5f;
        rect.right = rect.left + float(m_dataMap.cell_width) - CEEL_SELECT_WIDTH;
        rect.bottom = rect.top + float(m_dataMap.cell_height) - CEEL_SELECT_WIDTH;
        // ��Ⱦ
        m_pBrush_SetBeforeUse->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        UIManager_RenderTarget->DrawRectangle(&rect, m_pBrush_SetBeforeUse, CEEL_SELECT_WIDTH);
        m_pBrush_SetBeforeUse->SetColor(D2D1::ColorF(D2D1::ColorF::White));
        UIManager_RenderTarget->DrawRectangle(&rect, m_pBrush_SetBeforeUse, CEEL_SELECT_WIDTH * 0.5f);
    }
    // ���û���
    UIManager_RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
#ifdef PATHFD_ALIGNED
    UIManager_RenderTarget->SetTransform(&transform1);
#endif
    UIManager_RenderTarget->DrawBitmap(m_pNumnberTable);
}


/// <summary>
/// ��Ⱦ��-��Ⱦ����
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::render_chain_main() const noexcept {
    return Super::render_chain_main();
}

/// <summary>
/// ��Ⱦ��-��Ⱦǰ��
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::render_chain_foreground() const noexcept {
    return Super::render_chain_foreground();
}

/// <summary>
/// ��Ⱦ���ؼ�
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::Render() const noexcept {
    // ������Ⱦ
    this->render_chain_background();
    // ������Ⱦ
    this->render_chain_main();
    // ǰ����Ⱦ
    this->render_chain_foreground();
}


/// <summary>
/// �ؽ��ؼ���Դ
/// </summary>
/// <returns></returns>
auto PathFD::UIMapControl::Recreate() noexcept -> HRESULT {
    if (!m_pFileOpenDialog || !m_pFileSaveDialog) return E_FAIL;
    // ��ʼ��
    HRESULT hr = S_OK;
    ID2D1Bitmap1* pCellBitmapBoundary = nullptr;
    ID2D1BitmapRenderTarget* pBitmapRenderTarget = nullptr;
    size_t count = 4 * m_dataMap.cell_width * m_dataMap.cell_height;
    // ���ͷ�
    this->release_resource();
    // ����λͼ������
    if (SUCCEEDED(hr)) {
        D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM, 
            D2D1_ALPHA_MODE_PREMULTIPLIED
        );
        D2D1_SIZE_F size = D2D1::SizeF(
            float(m_dataMap.cell_width * 2), 
            float(m_dataMap.cell_height * 2)
        );
        hr = UIManager_RenderTarget->CreateCompatibleRenderTarget(
            &size,
            nullptr,
            &format,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE,
            &pBitmapRenderTarget
        );
    }
    // �����ؽ�
    if (SUCCEEDED(hr)) {
        hr = Super::Recreate();
    }
    // ������ͼ��Ԫ�ֽ���λͼ
    if (SUCCEEDED(hr)) {
        // ����
        hr = UIManager_RenderTarget->CreateBitmap(
            D2D1::SizeU(m_dataMap.cell_width, m_dataMap.cell_height),
            nullptr, 0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            ),
            &pCellBitmapBoundary
        );
    }
    // д��ֽ�����ɫ
    if (SUCCEEDED(hr)) {
        const float w = float(m_dataMap.cell_width);
        const float h = float(m_dataMap.cell_height);
        const D2D1_RECT_F rect = { w * 0.f + 1.f, h * 0.f + 1.f, w * 1.f - 1.f, h * 1.f - 1.f };
        pBitmapRenderTarget->BeginDraw();
        pBitmapRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        pBitmapRenderTarget->PushAxisAlignedClip(&rect, D2D1_ANTIALIAS_MODE_ALIASED);
        pBitmapRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.f));
        pBitmapRenderTarget->PopAxisAlignedClip();
        hr = pBitmapRenderTarget->EndDraw();
    }
    // ��������
    if (SUCCEEDED(hr)) {
        hr = pCellBitmapBoundary->CopyFromRenderTarget(nullptr, pBitmapRenderTarget, nullptr);
    }
    // ������ͼ��Ԫ�ֽ��߱�ˢ
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_BRUSH_PROPERTIES bbp;
        bbp.extendModeX = bbp.extendModeY = D2D1_EXTEND_MODE_WRAP;
        bbp.interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        hr = UIManager_RenderTarget->CreateBitmapBrush(
            pCellBitmapBoundary,
            &bbp, nullptr,
            &m_pCellBoundaryBrush
        );
    }
    // ���ý�ɫ
    if (SUCCEEDED(hr)) {
        m_pMapSkin = UIManager.GetBitmap(m_uMapBitmap);
        assert(m_pMapSkin && "bad action");
        if (!m_pMapSkin) hr = E_NOT_SET;
    }
    // ����ͼ��λͼ
    if (SUCCEEDED(hr)) {
        m_pMapIcon = UIManager.GetBitmap(m_idMapIcon);
        assert(m_pMapSkin && "bad action");
        if (!m_pMapSkin) hr = E_NOT_SET;
    }
    // ���ñ�ˢ͸����
    if (SUCCEEDED(hr)) {
        m_pCellBoundaryBrush->SetOpacity(0.25f);
    }
    // �������鼯1
    if (SUCCEEDED(hr)) {
        hr = UIManager_RenderTarget->CreateSpriteBatch(&m_pMapSpriteBatch);
    }
    // �������鼯2
    if (SUCCEEDED(hr)) {
        hr = UIManager_RenderTarget->CreateSpriteBatch(&m_pPathDisplay);
    }
    // �����Զ���Ƭλͼ����
    if (SUCCEEDED(hr)) {
        hr = UIManager_RenderTarget->CreateBitmap(
            D2D1::SizeU(m_dataMap.cell_width * 8, m_dataMap.cell_height * 8),
            nullptr, 0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            ),
            &m_pAutoTileCache
        );
    }
    // ��������
    if (SUCCEEDED(hr)) {
        auto halfw = m_dataMap.cell_width / 2;
        auto halfh = m_dataMap.cell_height / 2;
        uint32_t offx = 320;
        uint32_t offy = 320;
        D2D1_RECT_U src = {
            offx, offy,
            offx + m_dataMap.cell_width * 2,
            offy + m_dataMap.cell_height * 3,
        };
        D2D1_POINT_2U des = { 0, 0};
        /*constexpr int END = 256;
        constexpr int WIDTH = 16;
        enum { NONE, YONLY, XONLY, XANDY, XYZ };
        const DT OFFSET[] = {
            { m_dataMap.cell_width, m_dataMap.cell_height * 2 },
            { m_dataMap.cell_width, m_dataMap.cell_height },
            { 0, m_dataMap.cell_height * 2  },
            { m_dataMap.cell_width, 0 },
            { 0, m_dataMap.cell_height },
        };
        for (int i = 0; i < END; ++i) {
            int x = i & (WIDTH - 1);
            int y = i / WIDTH;

        }*/
        m_pAutoTileCache->CopyFromBitmap(&des, m_pMapSkin, &src);
    }
    // �������ֱ�
    if (SUCCEEDED(hr)) {
        hr = UIManager_RenderTarget->CreateBitmap(
            D2D1::SizeU(NUMBERTABLE_WIDTH, NUMBERTABLE_HEIGHT),
            nullptr, 0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            ),
            &m_pNumnberTable
        );
    }
    // �������ֱ�
    if (SUCCEEDED(hr)) {
        constexpr int LINECOUNT = NUMBERTABLE_WIDTH / NUMCOUNT / DIGNUMNER_WIDTH;
        constexpr int END = LINECOUNT * (NUMBERTABLE_HEIGHT/DIGNUMNER_HEIGHT);
        auto bmpd = m_pNumnberTable;
        auto bmps = m_pMapIcon;
        // ����
        auto copybmp = [=](int i, int num, int no) noexcept {
            // ��������
            auto x = (i % LINECOUNT) * DIGNUMNER_WIDTH * NUMCOUNT;
            auto y = (i / LINECOUNT) * DIGNUMNER_HEIGHT;
            D2D1_POINT_2U des = D2D1::Point2U(x + num*DIGNUMNER_WIDTH, y);
            D2D1_RECT_U src;
            src.left = DIGNUMNER_SRCX_OFFSET + no * DIGNUMNER_WIDTH;
            src.top = DIGNUMNER_SRCY_OFFSET;
            src.right = src.left + DIGNUMNER_WIDTH;
            src.bottom = src.top + DIGNUMNER_HEIGHT;
            // ���Ƶ�һλ
            bmpd->CopyFromBitmap(&des, bmps, &src);
        };
#ifdef _DEBUG
        LongUI::CUITimeMeterH tm; tm.Start();
        auto tick = ::timeGetTime();
#endif
        // ����
        for (int i = 0; i < END; ++i) {
            // ��1λ
            copybmp(i, 0, i / 1000);
            // ��2λ
            copybmp(i, 1, i / 100 % 10);
            // ��3λ
            copybmp(i, 2, i / 10 % 10);
            // ��4λ
            copybmp(i, 3, i  % 10);
        }
#ifdef _DEBUG
        auto time = tm.Delta_ms<float>();
        tick = ::timeGetTime() - tick;
        UIManager << DL_Log
            << L"Took "
            << time << L'('
            << long(tick)
            << L")ms to set number table"
            << LongUI::endl;
#endif
    }
    // ���ý�ɫ
    if (SUCCEEDED(hr)) {
        auto bitmap = UIManager.GetBitmap(m_uCharBitmap);
        assert(bitmap && "bad action");
        if (bitmap) {
            m_char.ResetChar(bitmap, this->get_char_data());
            m_char.ResetRenderTarget(UIManager_RenderTarget);
            LongUI::SafeRelease(bitmap);
        }
        else {
            hr = E_NOT_SET;
        }
    }
    LongUI::SafeRelease(pCellBitmapBoundary);
    LongUI::SafeRelease(pBitmapRenderTarget);
    return hr;
}


/// <summary>
/// һ��ʱ�䴦��
/// </summary>
/// <param name="arg">The argument.</param>
/// <returns></returns>
bool PathFD::UIMapControl::DoEvent(const LongUI::EventArgument& arg) noexcept {
    // ���ദ��
    return Super::DoEvent(arg);
}



// ����¼�
bool PathFD::UIMapControl::DoMouseEvent(const LongUI::MouseEventArgument& arg) noexcept {
    // ��֧�ֽ���״̬
#if 0
    // ����״̬���������Ϣ
    if (!this->GetEnabled()) return true;
#endif
    // ����ת��
    D2D1_POINT_2F pt4self = LongUI::TransformPointInverse(
        this->world, D2D1::Point2F(arg.ptx,  arg.pty)
    );
    // ------------------------------- ����ƶ�
    auto on_mouse_lbhold = [pt4self, this]() noexcept {
        auto x = uint32_t(pt4self.x) / m_dataMap.cell_width;
        auto y = uint32_t(pt4self.y) / m_dataMap.cell_height;
        switch (m_typeClicked)
        {
        case PathFD::UIMapControl::Type_Cell:
            break;
        case PathFD::UIMapControl::Type_Charactar:
            break;
        case PathFD::UIMapControl::Type_Start:
            if (x != m_dataMap.char_x || y != m_dataMap.char_y) {
                m_dataMap.char_x = x; m_dataMap.char_y = y;
                this->InvalidateThis();
            }
            break;
        case PathFD::UIMapControl::Type_Goal:
            if (x != m_uGoalX || y != m_uGoalY) {
                m_uGoalX = x; m_uGoalY = y;
                this->InvalidateThis();
            }
        break;
        }
    };
    // ------------------------------- ������
    auto on_lbutton_down = [pt4self, this]() noexcept {
        // ˫��
        if (m_hlpDbClick.Click(long(pt4self.x), long(pt4self.y))) {
            const bool a = m_uClickX < m_dataMap.map_width;
            const bool b = m_uClickY < m_dataMap.map_height;
            if (a && b) {
                auto index = m_uClickY * m_dataMap.map_width + m_uClickX;
                auto& cell = m_pMapCells[index];
                cell = !cell;
                // TODO: ������Ż�
                this->reset_sprites();
                this->InvalidateThis();
            }
        }
        // ����
        else {
            m_typeClicked = UIMapControl::Type_Cell;
            auto x = uint32_t(pt4self.x) / m_dataMap.cell_width;
            auto y = uint32_t(pt4self.y) / m_dataMap.cell_height;
            // ������
            if (x == m_dataMap.char_x && y == m_dataMap.char_y) {
                m_typeClicked = UIMapControl::Type_Start;
            }
            // ����յ�
            else if (x == m_uGoalX && y == m_uGoalY) {
                m_typeClicked = UIMapControl::Type_Goal;
            }
            // ��ͼѡ��
            if (m_uClickX != x || m_uClickY != y) {
                m_uClickX = x; m_uClickY = y;
                this->InvalidateThis();
            }
        }
    };
    // ------------------------------- �¼�����
    switch (arg.event)
    {
    case LongUI::MouseEvent::Event_MouseWheelV:
        // ��סCtrl������
        if (UIInput.IsKbPressed(UIInput.KB_CONTROL)) {
            float z = arg.wheel.delta * 0.5f + 1.f;
            auto zx = this->parent->GetZoomX();
            this->ZoomMapTo(zx * z, 0.5f);
            return true;
        }
        break;
    case LongUI::MouseEvent::Event_MouseWheelH:
        break;
    case LongUI::MouseEvent::Event_DragEnter:
        break;
    case LongUI::MouseEvent::Event_DragOver:
        break;
    case LongUI::MouseEvent::Event_DragLeave:
        break;
    case LongUI::MouseEvent::Event_Drop:
        break;
    case LongUI::MouseEvent::Event_MouseEnter:
        break;
    case LongUI::MouseEvent::Event_MouseLeave:
        break;
    case LongUI::MouseEvent::Event_MouseHover:
        break;
    case LongUI::MouseEvent::Event_MouseMove:
        if(UIInput.IsMbPressed(UIInput.MB_L)) on_mouse_lbhold();
        break;
    case LongUI::MouseEvent::Event_LButtonDown:
        on_lbutton_down();
        break;
    case LongUI::MouseEvent::Event_LButtonUp:
        if (m_typeClicked != UIMapControl::Type_Cell) {
            m_typeClicked = UIMapControl::Type_Cell;
            this->reset_sprites();
        }
        break;
    case LongUI::MouseEvent::Event_RButtonDown:
        break;
    case LongUI::MouseEvent::Event_RButtonUp:
        break;
    case LongUI::MouseEvent::Event_MButtonDown:
        break;
    case LongUI::MouseEvent::Event_MButtonUp:
        break;
    default:
        break;
    }
    return false;
}

// ���ſռ��С
void PathFD::UIMapControl::ZoomMapTo(float zoom, float time) noexcept {
    auto con = this->parent;
    auto zs = con->GetZoomX();
    auto len = zoom - zs;
    // ��ӿ������ŵ�ʱ�佺��
    UIManager.AddTimeCapsule([con, zs, len](float x) noexcept {
        x = LongUI::EasingFunction(LongUI::AnimationType::Type_QuarticEaseOut, x);
        auto zoomx = zs + len * x;
        con->SetZoom(zoomx, zoomx);
        // ��Ҫ��ֹʱ�佺��ˢ��
        return false;
    }, this, time);
}

/// <summary>
/// ִ��Ѱ·
/// </summary>
/// <param name="algorithm">The algorithm.</param>
/// <param name="info">The info.</param>
/// <returns></returns>
void PathFD::UIMapControl::Execute(IFDAlgorithm* algorithm, LongUI::CUIString& info) noexcept {
    assert(algorithm && "bad argument");
    PathFD::Finder finder;
    finder.data = m_pMapCells;
    finder.width = m_dataMap.map_width;
    finder.height = m_dataMap.map_height;
    finder.startx = int16_t(m_dataMap.char_x);
    finder.starty = int16_t(m_dataMap.char_y);
    finder.goalx = int16_t(m_uGoalX);
    finder.goaly = int16_t(m_uGoalY);
    LongUI::CUITimeMeterH meter;
    meter.Start();
    auto* path = algorithm->Execute(finder);
    auto time = meter.Delta_ms<double>();
    info.Format(L"%ls: %.3f ����", path ? L"�ɹ�" : L"ʧ��", time);
    // �ɵ�·����Ч
    if (m_pPath) {
        //assert(!"UNFINISHED");
        std::free(m_pPath);
    }
    // �µ�·����Ч
    if ((m_pPath = path)) {
        uint32_t count = path->len;
        uint32_t index = 0;
        assert(m_pMapSpriteBatch);
        auto mapw = m_dataMap.map_width;
        // ��ӿ���·����ʾ��ʱ�佺��
        UIManager.AddTimeCapsule([this, path, mapw, index](float x) mutable noexcept {
            constexpr auto at = LongUI::AnimationType::Type_QuarticEaseOut;
            x = LongUI::EasingFunction(at, x);
            if (x != 1.f) {
                // ����
                uint32_t end = uint32_t(x * float(path->len));
                D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Orange);
                bool invalidate = false;
                // ��������
                for (auto i = index; i <= end; ++i) {
                    auto realindex = uint32_t(path->pt[i].x) + uint32_t(path->pt[i].y) * mapw;
                    m_pMapSpriteBatch->SetSprites(
                        realindex, 1,
                        nullptr,
                        nullptr,
                        &color,
                        nullptr,
                        0, 0, 0, 0
                    );
                    invalidate = true;
                }
                // �ƽ�����
                index = end + 1;
                // ��Ҫˢ��
                if (invalidate) {
                    this->InvalidateThis();
                }
            }
            // ��Ҫ�жϵ���
            return false;
        }, &m_pMapSpriteBatch, 1.f + float(count) / 300.f);
    }
}

/// <summary>
/// ��ʼ��ʾ
/// </summary>
/// <param name="algorithm">The algorithm.</param>
/// <returns></returns>
void PathFD::UIMapControl::BeginShow(IFDAlgorithm*&& algorithm) noexcept {
    assert(algorithm && "bad argument");
    // ��������
    m_fAlgorithmStepTimeNow = 0.f;
    this->reset_map();
    // �ͷžɵ��㷨
    if (m_pAlgorithm) m_pAlgorithm->Dispose();
    // �޽��µ��㷨
    m_pAlgorithm = algorithm; algorithm = nullptr;
    // ִ���㷨
    PathFD::Finder finder;
    finder.data = m_pMapCells;
    finder.width = m_dataMap.map_width;
    finder.height = m_dataMap.map_height;
    finder.startx = int16_t(m_dataMap.char_x);
    finder.starty = int16_t(m_dataMap.char_y);
    finder.goalx = int16_t(m_uGoalX);
    finder.goaly = int16_t(m_uGoalY);
    m_pAlgorithm->BeginStep(finder);
}

/// <summary>
/// Uˢ�¿ؼ�
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::Update() noexcept {
    // ����ˢ��
    Super::Update();
    // ��ͼ��Ч
    if (!m_dataMap.map_width) return;
    // �����㷨
    if (m_pAlgorithm && m_fAlgorithmStepTimeNow >= 0.f) {
        m_fAlgorithmStepTimeNow += UIManager.GetDeltaTime();
#ifdef _DEBUG
        //m_fAlgorithmStepTimeAll = 0.5f;
#endif
        // ��Ҫˢ��
        if (m_fAlgorithmStepTimeNow > m_fAlgorithmStepTimeAll) {
            m_fAlgorithmStepTimeNow = 0.f;
            this->exe_next_step();
        }
    }
    // ��������
    m_char.Input(PathFD::InputCheck());
    // ˢ�½�ɫ
    if (m_char.Update()) {
        this->InvalidateThis();
    }
}



// ��ͣ�ָ�
void PathFD::UIMapControl::PauseResume() noexcept {
    if (m_fAlgorithmStepTimeNow < 0.f) {
        m_fAlgorithmStepTimeNow = 0.f;
    }
    else {
        m_fAlgorithmStepTimeNow = -1.f;
    }
    UIManager << DL_Log 
        << "m_fAlgorithmStepTimeNow" 
        << m_fAlgorithmStepTimeNow 
        << LongUI::endl;
}

// ִ����һ��
void PathFD::UIMapControl::ExeNextStep() noexcept {
    // ������Ч
    if (m_pAlgorithm) {
        // ��ͣ������ʾ
        m_fAlgorithmStepTimeNow = -1.f;
        // ��һ��
        this->exe_next_step();
    }
;}

// ִ����һ��
void PathFD::UIMapControl::exe_next_step() noexcept {
    assert(m_pAlgorithm && "bad action");
    auto end = m_pAlgorithm->NextStep(m_pMapSpriteBatch);
    // ������Ѱ?
    if (end) {
        m_pAlgorithm->EndStep();
        m_pAlgorithm->Dispose();
        m_pAlgorithm = nullptr;
    }
    this->InvalidateThis();
}

// PathFD �����ռ�
namespace PathFD {
    // �ļ�ͷ����
    static constexpr char PathFdHd[8] = { 'P','a','t','h','F','d','H','d' };
    // ��ͼ�ļ�ͷ����
    struct MapFileDataHeader {
        // �ļ�ͷ��ʶ 'PathFdHd'
        char            header[8];
        // ��ͼ���
        uint32_t        width;
        // ��ͼ�߶�
        uint32_t        height;
        // ���λ��X
        uint32_t        startx;
        // ���λ��Y
        uint32_t        starty;
        // �յ�λ��X
        uint32_t        goalx;
        // �յ�λ��Y
        uint32_t        goaly;
        // ���������ǵ�ͼ����
        //uint8_t         data[0];
    };
}

/// <summary>
/// �����ͼ
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::SaveMap() noexcept {
    if (!m_pMapCells) return;
    assert(m_pFileSaveDialog);
    IShellItem* item = nullptr;
    wchar_t* filename = nullptr;
    // ��ʾ����
    auto hr = m_pFileSaveDialog->Show(m_pWindow->GetHwnd());
    // ��ȡ���
    if (SUCCEEDED(hr)) {
        hr = m_pFileSaveDialog->GetResult(&item);
    }
    // ��ȡ��ʾ����
    if (SUCCEEDED(hr)) {
        hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filename);
    }
    // �����ļ�
    if (SUCCEEDED(hr)) {
        this->SaveMap(filename);
    }
    LongUI::SafeRelease(item);
    if (filename) {
        ::CoTaskMemFree(filename);
        filename = nullptr;
    }
}

/// <summary>
/// �����ͼ
/// </summary>
/// <returns></returns>
void PathFD::UIMapControl::LoadMap() noexcept {
    assert(m_pFileOpenDialog);
    IShellItem* item = nullptr;
    wchar_t* filename = nullptr;
    // ��ʾ����
    auto hr = m_pFileOpenDialog->Show(m_pWindow->GetHwnd());
    // ��ȡ���
    if (SUCCEEDED(hr)) {
        hr = m_pFileOpenDialog->GetResult(&item);
    }
    // ��ȡ��ʾ����
    if (SUCCEEDED(hr)) {
        hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filename);
    }
    // �����ļ�
    if (SUCCEEDED(hr)) {
        this->LoadMap(filename);
    }
    LongUI::SafeRelease(item);
    if (filename) {
        ::CoTaskMemFree(filename);
        filename = nullptr;
    }
}

/// <summary>
/// ʹ��ָ�����ļ��������ͼ
/// </summary>
/// <param name="filename">The filename.</param>
/// <returns></returns>
void PathFD::UIMapControl::LoadMap(const wchar_t* filename) noexcept {
    assert(filename && "bad file name");
    LongUI::CUIFile* empty = nullptr;
    LongUI::CUIFile file(filename, empty->Flag_Read);
    // �ļ���ʧ��
    if (!file.IsOk()) {
        UIManager.ShowError(L"�ļ���ʧ��", filename);
        return;
    }
    // �ļ�ͷ
    MapFileDataHeader header;
    auto read = file.Read(&header, sizeof(header));
    // ������Ҫ��
    if (read != sizeof(header) ||
        std::memcmp(header.header, PathFdHd, sizeof(PathFdHd))) {
        UIManager.ShowError(L"�Ƿ��ļ�", filename);
        return;
    }
    // ��֤��ͼ��Ч
    if (!this->require_mapdata(header.width, header.height)) return;
    uint32_t len = sizeof(m_pMapCells[0]) * (header.width * header.height);
    // ���
    std::memset(m_pMapCells, 0, len);
    // д������
    m_dataMap.map_data = m_pMapCells;
    m_dataMap.char_x = header.startx;
    m_dataMap.char_y = header.starty;
    m_uGoalX = header.goalx;
    m_uGoalY = header.goaly;
    // д���ͼ
    file.Read(m_pMapCells, len);
    // ���õ�ͼ
    this->reset_map();
}


// ��������
void PathFD::UIMapControl::SaveMap(const wchar_t* filename) noexcept {
    assert(filename && "bad file name");
    LongUI::CUIFile* empty = nullptr;
    LongUI::CUIFile file(filename, empty->Flag_CreateAlways | empty->Flag_Write);
    // �ļ���ʧ��
    if (!file.IsOk()) {
        UIManager.ShowError(L"�ļ�����ʧ��", filename);
        return;
    }
    // д���ļ�ͷ
    MapFileDataHeader header;
    std::memcpy(&header.header, PathFdHd, sizeof(PathFdHd));
    header.width = m_dataMap.map_width;
    header.height = m_dataMap.map_height;
    header.startx = m_dataMap.char_x;
    header.starty = m_dataMap.char_y;
    header.goalx = m_uGoalX;
    header.goaly = m_uGoalY;
    // д���ļ�
    file.Write(&header, sizeof(header));
    // д���ͼ
    file.Write(m_pMapCells, sizeof(m_pMapCells[0]) * header.width * header.height);
}

/// <summary>
/// �������ؼ�
/// </summary>
/// <param name="type">The type.</param>
/// <param name="node">The node.</param>
/// <returns></returns>
auto PathFD::UIMapControl::CreateControl(
    LongUI::CreateEventType type, pugi::xml_node node) noexcept -> UIControl * {
    UIMapControl* pControl = nullptr;
    switch (type)
    {
    case LongUI::Type_Initialize:
        break;
    case LongUI::Type_Recreate:
        break;
    case LongUI::Type_Uninitialize:
        break;
    case_LongUI__Type_CreateControl:
        LongUI__CreateWidthCET(UIMapControl, pControl, type, node);
    }
    return pControl;
}



#ifdef LongUIDebugEvent
// UI�ı�: ������Ϣ
bool PathFD::UIMapControl::debug_do_event(const LongUI::DebugEventInformation& info) const noexcept {
    switch (info.infomation)
    {
    case LongUI::DebugInformation::Information_GetClassName:
        info.str = L"UIMapControl";
        return true;
    case LongUI::DebugInformation::Information_GetFullClassName:
        info.str = L"::PathFD::UIMapControl";
        return true;
    case LongUI::DebugInformation::Information_CanbeCasted:
        // ����ת��
        return *info.iid == LongUI::GetIID<::PathFD::UIMapControl>()
            || Super::debug_do_event(info);
    default:
        break;
    }
    return false;
}
#endif

// pathfd ·��
namespace PathFD {
    /// <summary>
    /// ������ͼ�ؼ�
    /// </summary>
    /// <returns></returns>
    auto CreateMapControl(LongUI::CreateEventType cet, pugi::xml_node node)
        noexcept -> LongUI::UIControl * {
        return UIMapControl::CreateControl(cet, node);
    }
}
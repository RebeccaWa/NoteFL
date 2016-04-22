#include "pfdWndView.h"
#include "pfdUIMap.h"
#include <Core/luiWindow.h>
#include <Core/luiManager.h>
#include  "pfdAlgorithm.h"


/// <summary>
/// ����ؼ�
/// </summary>
/// <returns></returns>
void PathFD::CFDWndView::cleanup() noexcept {
    // ɾǰ����
    this->before_deleted();
    // �ͷſռ�
    delete this;
}


/// <summary>
/// �¼�����
/// </summary>
/// <param name="arg">The argument.</param>
/// <returns></returns>
bool PathFD::CFDWndView::DoEvent(const LongUI::EventArgument& arg) noexcept {
    switch (arg.event)
    {
    case LongUI::Event::Event_TreeBuildingFinished:
        this->init_wndview();
        __fallthrough;
    default:
        return Super::DoEvent(arg);
    }
}

// ���ú���
using LongUI::longui_cast;

// ��ʼ���ؼ�
void PathFD::CFDWndView::init_wndview() noexcept {
    LongUI::UIControl* ctrl = nullptr;
#ifdef _DEBUG
    UIManager << DL_Log << L"called" << LongUI::endl;
#endif
    // ��ͼ�ؼ�
    ctrl = m_pWindow->FindControl("mapPathFD");
    m_pMapControl = longui_cast<UIMapControl*>(ctrl);
    // ��ͼ���
    auto wc = m_pWindow->FindControl("edtMapWidth");
    // ��ͼ�߶�
    auto wh = m_pWindow->FindControl("edtMapHeight");
    // ���ɵ�ͼ
    ctrl = m_pWindow->FindControl("btnMapGene");
    {
        auto map = m_pMapControl;
        ctrl->AddEventCall([wc, map, wh](LongUI::UIControl*) noexcept {
            assert(wc && wh && "bad action");
            auto w = LongUI::AtoI(wc->GetText());
            auto h = LongUI::AtoI(wh->GetText());
            map->GenerateMap(uint32_t(w), uint32_t(h));
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
    // �����ͼ
    ctrl = m_pWindow->FindControl("btnMapSave");
    {
        auto map = m_pMapControl;
        ctrl->AddEventCall([map](LongUI::UIControl*) noexcept {
            map->SaveMap();
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
    // �����ͼ
    ctrl = m_pWindow->FindControl("btnMapLoad");
    {
        auto map = m_pMapControl;
        ctrl->AddEventCall([map](LongUI::UIControl*) noexcept {
            map->LoadMap();
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
    // ��������
    ctrl = m_pWindow->FindControl("btnMapRezm");
    {
        auto map = m_pMapControl;
        ctrl->AddEventCall([map](LongUI::UIControl*) noexcept {
            map->ZoomMapTo(1.f, 0.5f);
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
    // ��ʼѰ·
    ctrl =  m_pWindow->FindControl("btnFinderStart");
    {
        auto display = m_pWindow->FindControl("txtDisplay");
        auto map = m_pMapControl;
        ctrl->AddEventCall([map, display](LongUI::UIControl*) noexcept {
            IFDAlgorithm* algorithm = nullptr;
            algorithm = PathFD::CreateAStarAlgorithm();
            if (algorithm) {
                LongUI::CUIString str;
                map->Execute(algorithm, str);
                display->SetText(str);
                algorithm->Dispose();
            }
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
    // ��ʼ��ʾ
    ctrl =  m_pWindow->FindControl("btnFinderShow");
    {
        auto map = m_pMapControl;
        ctrl->AddEventCall([map](LongUI::UIControl*) noexcept {
            auto* algorithm = PathFD::CreateAStarAlgorithm();
            map->BeginShow(std::move(algorithm));
            assert(algorithm == nullptr);
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
    // ������ʾ
    ctrl =  m_pWindow->FindControl("btnFinderStep");
    {
        auto map = m_pMapControl;
        ctrl->AddEventCall([map](LongUI::UIControl*) noexcept {
            map->ExeNextStep();
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
    // ��ͣ/�ָ�
    ctrl =  m_pWindow->FindControl("btnFinderPaRe");
    {
        auto map = m_pMapControl;
        ctrl->AddEventCall([map](LongUI::UIControl*) noexcept {
            map->PauseResume();
            return true;
        }, LongUI::SubEvent::Event_ItemClicked);
    }
}

#include "pdfImpl.h"

// pathfd �����ռ�
namespace PathFD {
    // С���ڴ�����
    auto AllocSmall(size_t length) noexcept -> void * {
        assert(length < 256);
        return LongUI::SmallAlloc(length);
    }
    // С���ڴ��ͷ�
    void FreeSmall(void* address) noexcept {
        return LongUI::SmallFree(address);
    }
    // ��ȡ���ʱ��
    auto GetDeltaTime() noexcept -> float {
        return UIManager.GetDeltaTime();
    }
    // �������
    auto InputCheck() noexcept -> CharacterDirection {
        // �·�������
        if (UIInput.IsKbPressed(UIInput.KB_DOWN)) {
            return Direction_S;
        }
        // ��������
        if (UIInput.IsKbPressed(UIInput.KB_LEFT)) {
            return Direction_W;
        }
        // �ҷ�������
        if (UIInput.IsKbPressed(UIInput.KB_RIGHT)) {
            return Direction_E;
        }
        // �Ϸ�������
        if (UIInput.IsKbPressed(UIInput.KB_UP)) {
            return Direction_N;
        }
        // ��
        return Direction_Nil;
    }
    // ����ƫ��
    extern const DT DIRECTION_OFFSET[DIRECTION_SIZE] = {
        { 0, 1}, {-1, 0}, { 1, 0}, { 0,-1},
        {-1, 1}, { 1, 1}, {-1,-1}, { 1,-1},
    };
    // impl �����ռ�
    namespace impl {
#ifdef _DEBUG
        // �������
        void outputdebug(const wchar_t* a) noexcept {
            UIManager << DL_Log << a << LongUI::endl;
        }
#endif
        // mutex
        struct mutex_impl { CRITICAL_SECTION cs; };
        // ����������
        auto create_mutex() noexcept ->mutex {
            auto ptr = reinterpret_cast<mutex_impl*>(PathFD::AllocSmall(sizeof(mutex_impl)));
            if (ptr) ::InitializeCriticalSection(&ptr->cs);
            return ptr;
        }
        // �ݻٻ�����
        void destroy(mutex& mx) noexcept {
            if (mx) {
                ::DeleteCriticalSection(&mx->cs);
                PathFD::FreeSmall(mx);
            }
            mx = nullptr;
        }
        // �ϻ�����
        void lock(mutex mx) noexcept {
            assert(mx && "bad argment");
            ::EnterCriticalSection(&mx->cs);
        }
        // �»�����
        void unlock(mutex mx) noexcept {
            assert(mx && "bad argment");
            ::LeaveCriticalSection(&mx->cs);
        }
        // windows
        auto windows(event ev) noexcept { return reinterpret_cast<HANDLE>(ev); }
        // pathfd
        auto pathfd(HANDLE ev) noexcept { return reinterpret_cast<event>(ev); }
        // ������ɫ
        void set_cell_color(void* sprite, uint32_t index, const color& c) noexcept {
            assert(sprite && "bad argument");
            auto* sb = reinterpret_cast<ID2D1SpriteBatch*>(sprite);
            assert(sb->GetSpriteCount() > index && "out fo range");
            sb->SetSprites(
                index, 1,
                nullptr,
                nullptr,
                reinterpret_cast<const D2D1_COLOR_F*>(&c),
                nullptr,
                0, 0, 0, 0
            );
        }
        // �����¼�
        auto create_event() noexcept->event {
            static_assert(sizeof(HANDLE) == sizeof(event), "bad action");
            return pathfd(::CreateEventW(nullptr, FALSE, FALSE, nullptr));
        }
        // �����¼�
        void signal(event ev) noexcept {
            assert(ev && "bad argment");
            ::SetEvent(windows(ev));
        }
        // �ȴ��¼�
        void wait(event ev) noexcept {
            assert(ev && "bad argment");
            ::WaitForSingleObject(windows(ev), 1000);
        }
        // �ݻ��¼�
        void destroy(event& ev) noexcept {
            if (ev) {
                ::CloseHandle(windows(ev));
                ev = nullptr;
            }
        }
    }
}

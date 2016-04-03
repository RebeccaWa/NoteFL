#pragma once

#include <LongUI.h>

// pathfd �����ռ�
namespace PathFD {
    // ������ͼ�ؼ�
    auto CreateMapControl(LongUI::CreateEventType, pugi::xml_node) noexcept->LongUI::UIControl*;
    // ����������Ϣ
    class CFDConfig final : public LongUI::CUIDefaultConfigure {
        // ����
        using Super = CUIDefaultConfigure;
    public:
        // template string
        const char*     tmplt = nullptr;
        // ���캯��
        CFDConfig() : Super(UIManager) { }
        // �����������
        auto GetLocaleName(wchar_t name[/*LOCALE_NAME_MAX_LENGTH*/]) noexcept ->void override {
            std::wcscpy(name, L"en-us");
        };
        // ����flag
        auto GetConfigureFlag() noexcept ->ConfigureFlag override { 
            return Flag_OutputDebugString /*| Flag_RenderByCPU /*| Flag_DbgOutputFontFamily*/;
        }
        // ��ȡģ���ַ���
        auto GetTemplateString() noexcept ->const char* override { return tmplt; }
        // ע��ؼ�
        auto RegisterSome() noexcept ->void override {
            m_manager.RegisterControlClass(CreateMapControl, "PathMap");
        };
        // ѡ���Կ�
        auto ChooseAdapter(const DXGI_ADAPTER_DESC1 adapters[], const size_t length) noexcept -> size_t override {
            // Intel ����
            for (size_t i = 0; i < length; ++i) {
                if (!std::wcsncmp(L"Intel", adapters[i].Description, 5))
                    return i;
            }
            // ���Կ�
            for (size_t i = 0; i < length; ++i) {
                if (!std::wcsncmp(L"NVIDIA", adapters[i].Description, 6))
                    return i;
            }
            return length;
        }
    };
}
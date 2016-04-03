#pragma once

#include <luibase.h>
#include <luiconf.h>
#include <Control/UIViewport.h>

// pathfd �����ռ�
namespace PathFD {
    // ��ͼ�ؼ�
    class UIMapControl;
    // ���򴰿���ͼ
    class CFDWndView final : public LongUI::UIViewport {
        // ��������
        using Super = LongUI::UIViewport;
        // ��Ԫ����
        friend class Super;
        // ����ؼ�
        virtual void cleanup() noexcept override;
    public:
        // ���캯��
        CFDWndView(LongUI::XUIBaseWindow* window) : Super(window) { }
    public:
        // �¼�����
        virtual bool DoEvent(const LongUI::EventArgument& arg) noexcept override;
    protected:
        // ɾǰ����
        void before_deleted() noexcept { Super::before_deleted(); }
        // ��ʼ��
        void init_wndview() noexcept;
    private:
        // �������ƹ���
        CFDWndView(const CFDWndView&) = delete;
        // ��������
        ~CFDWndView() = default;
    private:
        // ����ͼ�ؼ�
        UIMapControl*           m_pMapControl = nullptr;
    };
}
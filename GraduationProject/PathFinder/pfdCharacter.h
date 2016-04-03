#pragma once

#include <cstdint>
#include <d2d1_3.h>
#include "pfdUtil.h"

#pragma warning(disable: 4200)

// pathfd �����ռ�
namespace PathFD {
    // ��ͼ����
    struct MapData {
        // ��ͼ����
        const uint8_t*          map_data;
        // ��ͼ���(��Ԫ��)
        uint32_t                map_width;
        // ��ͼ�߶�(��Ԫ��)
        uint32_t                map_height;
        // ��ɫX����(��Ԫ��)
        uint32_t                char_x;
        // ��ɫY����(��Ԫ��)
        uint32_t                char_y;
        // ��Ԫ����(����)
        uint32_t                cell_width;
        // ��Ԫ��߶�(����)
        uint32_t                cell_height;
    };
    // ��ɫ����
    struct CharData {
        // ��ɫ���(����)
        uint32_t                width;
        // ��ɫ�߶�(����)
        uint32_t                height;
        // ԴXƫ��(����)
        uint32_t                src_offsetx;
        // ԴYƫ��(����)
        uint32_t                src_offsety;
        // Ŀ��Xƫ��(����)
        uint32_t                des_offsetx;
        // Ŀ��Yƫ��(����)
        uint32_t                des_offsety;
        // ��ɫ����
        CharacterDirection      direction;
        // ��������
        uint32_t                acount;
        // ��������ʱ��
        float                   atime;
        // �ƶ��ٶ�
        float                   speed;
        // ��������
        uint32_t                action[0];
    };
    // ��ɫ��ʾ
    class CFDCharacter final {
    public:
        // ���캯��
        CFDCharacter() noexcept;
        // ��������
        ~CFDCharacter() noexcept { this->release_data(); }
        // ˢ��, ����Ҫ������Ⱦ�ŷ���true
        bool Update() noexcept;
        // ����
        void Input(CharacterDirection d) noexcept;
        // ��Ⱦ
        void Render() const noexcept;
        // ���õ�ͼ����
        void ResetMap(const MapData& data) noexcept;
        // ���ý�ɫͼ��
        void ResetChar(ID2D1Bitmap1* bitmap, const CharData& data) noexcept;
        // ������ȾĿ�������
        void ResetRenderTarget(ID2D1DeviceContext2* d2ddc) noexcept;
        // ִ�ж���
        void BeginAction();
        // ��������
        void EndAction();
    public:
        // �ƶ���
        bool IsMoving() const noexcept { return m_dtMoving != PathFD::Direction_Nil; }
        // ��ȡX��������
        auto GetPxX() const noexcept { return; }
        // ��ȡX��������
        auto GetPxY() const noexcept { return;}
    private:
        // �ͷ�����
        void release_data() noexcept;
        // ˢ��λ��
        void refresh_position() noexcept;
    private:
        // ��ȾĿ�������
        ID2D1DeviceContext2*    m_pRenderTarget = nullptr;
        // ��ʹ��λͼ
        ID2D1Bitmap1*           m_pBitmap = nullptr;
        // ��ͼ����
        MapData                 m_map;
        // ��ɫ����
        CharData*               m_pCharData = nullptr;
        // ��������
        uint32_t                m_ixAction = 0;
        // �����¼�
        float                   m_fActionTime = 0.f;
        // �ƶ���λ
        CharacterDirection      m_dtMoving = Direction_Nil;
        // �ƶ�ƫ���� ��Χ[0, 1]
        float                   m_fMoveOffset = 0.f;
        // x ����λ��
        float                   m_fPosX = 0.f;
        // y ����λ��
        float                   m_fPosY = 0.f;
        // δʹ��
        //uint32_t                m_unused_u32 = 0;
        // ��ɫ��ֵ�㷨
        uint16_t                m_uCharInter = D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        // ˢ��
        bool                    m_bNeedRefresh = false;
        // ������
        bool                    m_bInAction = false;
    };
}
#include "pfdCharacter.h"
#include <algorithm>
#include <cassert>
#include <memory>
#undef min
#undef max

// pathfd �����ռ�
namespace PathFD {
    // ��ȫ�ͷ�
    template<class T> void SafeRelease(T*& iii) noexcept {
        if (iii) {
            iii->Release();
            iii = nullptr;
        }
    }
    // ��ȫ����
    template<class T> inline auto SafeAcquire(T* pInterfaceToRelease) {
        if (pInterfaceToRelease) {
            pInterfaceToRelease->AddRef();
        }
        return pInterfaceToRelease;
    }
}

/// <summary>
/// <see cref="CFDCharacter"/> �� ���캯��
/// </summary>
PathFD::CFDCharacter::CFDCharacter() noexcept {

}


/// <summary>
/// �ͷ�����
/// </summary>
/// <returns></returns>
void PathFD::CFDCharacter::release_data() noexcept {
    PathFD::SafeRelease(m_pBitmap);
    PathFD::SafeRelease(m_pRenderTarget);
    if (m_pCharData) {
        PathFD::FreeSmall(m_pCharData);
        m_pCharData = nullptr;
    }
}


/// <summary>
/// ���õ�ͼ
/// </summary>
/// <param name="data">The data.</param>
/// <returns></returns>
void PathFD::CFDCharacter::ResetMap(const MapData& data) noexcept {
    // ������
    assert(data.map_data && "bad map data");
    assert(data.map_width && "bad map width");
    assert(data.map_height && "bad map height");
    assert(data.cell_width && "bad cell width");
    assert(data.cell_height && "bad cell height");
    // ��ֵ
    m_map = data;
    // ˢ��
    this->refresh_position();
}


/// <summary>
/// ���ý�ɫͼ��
/// </summary>
/// <param name="bitmap">The bitmap.</param>
/// <returns></returns>
void PathFD::CFDCharacter::ResetChar(ID2D1Bitmap1* bitmap, const CharData& data) noexcept {
    // ������
    assert(data.atime > 0.f && "bad action time");
    assert(data.speed > 0.f && "bad speed");
    assert(data.acount && "bad action count");
    assert(data.width && "bad character width");
    assert(data.height && "bad character height");
    assert(bitmap && "bad bitmap");
    // �ͷ�������
    PathFD::SafeRelease(m_pBitmap);
    if (m_pCharData) PathFD::FreeSmall(m_pCharData);
    // ��������
    m_pBitmap = PathFD::SafeAcquire(bitmap);
    const size_t len = sizeof(CharData) + sizeof(CharData::action[0]) * data.acount;
    m_pCharData = reinterpret_cast<CharData*>(PathFD::AllocSmall(len));
    assert(m_pCharData && "OOM for just 'len' byte");
    std::memcpy(m_pCharData, &data, len);
}

/// <summary>
/// ������ȾĿ�������
/// </summary>
/// <param name="d2ddc">The D2DDC.</param>
/// <returns></returns>
void PathFD::CFDCharacter::ResetRenderTarget(ID2D1DeviceContext2* d2ddc) noexcept {
    PathFD::SafeRelease(m_pRenderTarget);
    m_pRenderTarget = PathFD::SafeAcquire(d2ddc);
}

/// <summary>
/// ��Ⱦ��ɫ
/// </summary>
/// <returns></returns>
void PathFD::CFDCharacter::Render() const noexcept {
    if (!m_pCharData) return;
    const auto& chardata = *m_pCharData;
    // ��ɫ���
    const float charw = float(chardata.width);
    const float charh = float(chardata.height);
    // ����Ŀ�����
    D2D1_RECT_F des;
    {
        des.left = m_fPosX;
        des.top = m_fPosY;
        des.right = des.left + charw;
        des.bottom = des.top + charh;
    }
    // ����Դ����
    D2D1_RECT_F src;
    {
        // X�����ǿ�"ԴXƫ��"��"����"���� 
        src.left = float(
            chardata.src_offsetx +
            chardata.width * chardata.action[m_ixAction]
            );
        // Y�����ǿ�"ԴXƫ��"��"����"���� 
        src.top = float(
            chardata.src_offsety +
            chardata.width * chardata.direction
            );
        // ��߾��ǽ�ɫ���
        src.right = src.left + charw;
        src.bottom = src.top + charh;
    }
    // �̻�ͼ��
    m_pRenderTarget->DrawBitmap(
        m_pBitmap,
        &des,
        1.f,
        D2D1_INTERPOLATION_MODE(m_uCharInter),
        &src,
        nullptr
    );
}

/// <summary>
/// Refresh_positions this instance.
/// </summary>
/// <returns></returns>
void PathFD::CFDCharacter::refresh_position() noexcept {
    const auto& chardata = *m_pCharData;
    DT dt = { 0,0 };
    // �ƶ���
    if (this->IsMoving()) dt = DIRECTION_OFFSET[m_dtMoving];
    // TODO: XY���껹�е�Ԫ��ͽ�ɫ��С�Ƚ�, ����Ĭ��һ��
    float x = float(dt.x) * m_fMoveOffset + float(m_map.char_x);
    float y = float(dt.y) * m_fMoveOffset + float(m_map.char_y);
    // X�����ǿ�"Ŀ��Xƫ��","Xλ��"��"�ƶ�ƫ����"����
    x = float(chardata.des_offsetx) + x * float(m_map.cell_width);
    // Y�����ǿ�"Ŀ��Yƫ��","Yλ��"��"�ƶ�ƫ����"����
    y = float(chardata.des_offsety) + y * float(m_map.cell_height);
    // ��Ҫˢ��
    m_bNeedRefresh |= (int(x) != int(m_fPosX)) | (int(y) != int(m_fPosY));
    // ��������
    m_fPosX = x; m_fPosY = y;
}

/// <summary>
/// ˢ�±�����
/// </summary>
/// <returns></returns>
bool PathFD::CFDCharacter::Update() noexcept {
    const auto& chardata = *m_pCharData;
    // �ƶ�����
    if (this->IsMoving()) {
        m_fMoveOffset += PathFD::GetDeltaTime() * chardata.speed;
        this->refresh_position();
        if (m_fMoveOffset >= 1.f) {
            m_fMoveOffset = 0.f;
            auto dt = DIRECTION_OFFSET[m_dtMoving];
            m_dtMoving = Direction_Nil;
            m_map.char_x += dt.x;
            m_map.char_y += dt.y;
            m_bNeedRefresh = true;
            this->EndAction();
        }
    }
    // ���¶���
    if (m_bInAction) {
        m_fActionTime += PathFD::GetDeltaTime();
        // ��������
        auto findex = m_fActionTime / chardata.atime * float(chardata.acount);
        uint32_t index = uint32_t(findex);
        if (m_fActionTime > chardata.atime) {
            m_fActionTime = 0.f;
            index = chardata.acount - 1;
        }
        // �����µ�����
        if (index != m_ixAction) {
            m_ixAction = index;
            m_bNeedRefresh = true;
        }
    }
    {
        bool bk = m_bNeedRefresh;
        m_bNeedRefresh = false;
        return bk;
    }
}

/// <summary>
/// ��������
/// </summary>
/// <param name="d">The d.</param>
/// <returns></returns>
void PathFD::CFDCharacter::Input(CharacterDirection d) noexcept {
    // �ƶ���
    if (this->IsMoving()) return;
    // ������Ч
    if (d != PathFD::Direction_Nil) {
        if (m_pCharData->direction != d) {
            m_pCharData->direction = d;
            m_bNeedRefresh = true;
        }
        assert(d < DIRECTION_SIZE);
        // ����Ƿ����ͨ��
        auto dt = DIRECTION_OFFSET[d];
        uint32_t x = uint32_t(dt.x + int32_t(m_map.char_x));
        uint32_t y = uint32_t(dt.y + int32_t(m_map.char_y));
        if (x < m_map.map_width && y < m_map.map_height) {
            uint32_t index = x + y * m_map.map_width;
            // ����ͨ��
            if (m_map.map_data[index]) {
                m_dtMoving = d;
                this->BeginAction();
            }
        }
    }
}

/// <summary>
/// ִ�ж���
/// </summary>
void PathFD::CFDCharacter::BeginAction() {
    m_bInAction = true;
    //m_fActionTime = 0.f;
    //m_ixAction = 0;
}

/// <summary>
/// ��������
/// </summary>
void PathFD::CFDCharacter::EndAction() {
    m_bInAction = false;
    //m_fActionTime = 0.f;
    //m_ixAction = 0;
}

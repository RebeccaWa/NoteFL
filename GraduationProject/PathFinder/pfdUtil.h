#pragma once

#include <cstdint>

// pathfd �����ռ�
namespace PathFD {
#pragma warning(disable: 4200)
    // ��ɫ��λ
    enum CharacterDirection : uint32_t {
        Direction_Nil = uint32_t(-1),
        Direction_S = 0,    // �Ϸ�
        Direction_W,        // ����
        Direction_E,        // ����
        Direction_N,        // ����
        Direction_SW,       // ����
        Direction_SE,       // ����
        Direction_NW,       // ����
        Direction_NE,       // ����
        DIRECTION_SIZE,     // ������С
    };
    // ������ʾ������
    struct NodeDisplay {
        // CharacterDirection
        using CD = CharacterDirection;
        // X����
        uint32_t    x;
        // Y����
        uint32_t    y;
        // ����
        uint32_t    i;
        // ����
        CD          d;
        // ���鳤��
        uint32_t    argc;
        // ��������
        uint32_t    argv[0];
    };
    // ����
    struct DT { int32_t x, y; };
    // ����ƫ��
    extern const DT DIRECTION_OFFSET[DIRECTION_SIZE];
    // ��ȡÿ֡���ʱ��
    auto GetDeltaTime() noexcept -> float;
    // ����С���ڴ�
    auto AllocSmall(size_t length) noexcept -> void*;
    // ����С���ڴ�
    void FreeSmall(void* address) noexcept;
    // �������
    auto InputCheck() noexcept ->CharacterDirection;
}
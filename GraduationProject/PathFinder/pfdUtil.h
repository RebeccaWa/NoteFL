#pragma once

#include <cstdint>

// pathfd �����ռ�
namespace PathFD {
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
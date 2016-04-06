#pragma once

#include <cstdint>
#include "pfdUtil.h"

#ifndef PATHFD_NOVTABLE
#ifdef _MSC_VER
#define PATHFD_NOVTABLE __declspec(novtable)
#else
#define PATHFD_NOVTABLE
#endif
#endif
#pragma warning(disable: 4200)

// pathfd �����ռ�
namespace PathFD {
    // FD�ṹ
    struct Finder {
        // �Թ�����
        const uint8_t*  data;
        // �Թ����
        int16_t         width;
        // �Թ��߶�
        int16_t         height;
        // ���λ��X
        int16_t         startx;
        // ���λ��Y
        int16_t         starty;
        // �յ�λ��X
        int16_t         goalx;
        // �յ�λ��Y
        int16_t         goaly;
    };
    // ·����
    struct PathPoint { int16_t x, y; };
    // ·��
    struct Path { uint32_t len; PathPoint pt[0]; };
    // Ѱ·�㷨
    struct PATHFD_NOVTABLE IFDInterface {
        // �ͷŶ���
        virtual void Dispose() noexcept = 0;
    };
    // Ѱ·�㷨
    struct PATHFD_NOVTABLE IFDAlgorithm : IFDInterface {
        // ִ���㷨, ����·��(�ɹ��Ļ�), ��Ҫ�����ߵ���std::free�ͷ�
        virtual auto Execute(const PathFD::Finder& fd) noexcept->Path* = 0;
        // ���ӻ�����
        virtual void BeginStep(const PathFD::Finder& fd) noexcept = 0;
        // ���ӻ�����
        virtual void NextStep() noexcept = 0;
        // �������ӻ�����
        virtual void EndStep() noexcept = 0;
    };
    // ����A*�㷨
    auto CreateAStarAlgorithm() noexcept ->IFDAlgorithm*;
}
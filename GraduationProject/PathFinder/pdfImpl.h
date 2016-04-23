#pragma once

// pathfd::impl �����ռ�
namespace PathFD { namespace impl {
#ifdef _DEBUG
    // ���������
    void outputdebug(const wchar_t* a) noexcept;
#endif
    // ������
    struct mutex_impl; using mutex = mutex_impl*;
    // ����������
    auto create_mutex() noexcept->mutex;
    // �ݻٻ�����
    void destroy(mutex& mx) noexcept;
    // �ϻ�����
    void lock(mutex mx) noexcept;
    // �»�����
    void unlock(mutex mx) noexcept;
    // �¼�
    struct event_impl; using event = event_impl*;
    // ��ɫ
    struct color { float r, g, b, a; };
    // �趨ָ����Ԫ�񸽼���ɫ
    void set_cell_color(void* sprite, uint32_t index, const color& c) noexcept;
    // ����������ʾ
    void set_node_display(void* num, void* display) noexcept;
    // �����¼�
    auto create_event() noexcept->event;
    // �����¼�
    void signal(event ev) noexcept;
    // �ȴ��¼�
    void wait(event ev) noexcept;
    // �ݻ��¼�
    void destroy(event& ev) noexcept;
}}
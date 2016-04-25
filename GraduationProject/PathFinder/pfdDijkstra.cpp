#include "pfdAlgorithm.h"
#include "pdfImpl.h"

#include <algorithm>
#include <cassert>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include <new>

/*

Dijkstra �㷨:
  ��tile��ͼ��, Dijkstra��ʵ���ǹ�������㷨

*/

// pathfd �����ռ�
namespace PathFD {
    // impl
    namespace impl {
        // ����������
        template<typename T> struct dijk_step_op {
            // ����OPEN��
            template<typename Y>inline void set_list(Y& l) {
                list = &l;
            }
            // ����OPEN��
            inline void set_open_list_begin(size_t obegin) {
                openbegin = obegin;
            }
            // ·���Ѿ��ҵ�
            template<typename Y> inline void found(const Y& list) {
                parent.SetFinished();
            }
            // �Ƿ����Ѱ��
            inline bool go_on() {
                return !parent.IsExit();
            }
            // Ѱ��·��ʧ��
            inline void failed() {
                parent.SetFinished();
            }
            // ��ʾ����ִ��
            inline void signal() {
                impl::signal(ev);
            }
            // �ȴ���һ����
            inline void wait() {
                impl::wait(ev);
            }
            // Ϊ���������
            inline void lock() {
                impl::lock(mx);
            }
            // Ϊ���������
            inline void unlock() {
                impl::unlock(mx);
            }
            // ���챾�ຯ��
            dijk_step_op(T& parent) : parent(parent) {}
            // ���챾�ຯ��
            ~dijk_step_op() { impl::destroy(ev); impl::destroy(mx);}
            // �¼�
            event               ev = impl::create_event();
            // ������
            mutex               mx = impl::create_mutex();
            // ������
            T&                  parent;
            // OPEN��
            typename T::List*   list = nullptr;
            // OPEN��λ��
            size_t              openbegin = 0;
        };
        // ������
        struct dijk_op {
            // ����OPEN��
            template<typename Y> inline void set_list(Y& list) {  }
            // ����OPEN��
            inline void set_open_list_begin(size_t obegin) { }
            // ��ȡһ���ڵ�
            template<typename Y> inline void get_node(const Y& node) { }
            // ·���Ѿ��ҵ�
            template<typename Y> inline auto found(const Y& list) {
                return impl::path_found(list);
            }
            // �Ƿ����Ѱ��
            inline bool go_on() { return true; }
            // Ѱ��·��ʧ��
            inline auto failed() ->Path* { return nullptr; }
            // ��ʾ����ִ��
            inline void signal() { }
            // �ȴ���һ����
            inline void wait() { }
            // Ϊ���������
            inline void lock() { }
            // Ϊ���������
            inline void unlock() { }
        };
    }
    // Dijkstra�㷨
    class CFDDijkstra final : public IFDAlgorithm {
    public:
        // �ڵ�
        struct NODE {
            // ����
            int16_t         x, y;
            // ���ڵ��������
            int8_t          px, py;
            // ����
            int16_t         step;
        };
        // �б�
        using List = std::vector<CFDDijkstra::NODE>;
        // ����
        using StepOp = impl::dijk_step_op<CFDDijkstra>;
    public:
        // ���캯��
        CFDDijkstra() noexcept;
        // �ͷŶ���
        void Dispose() noexcept override { delete this; };
        // ִ���㷨, ����·��(�ɹ��Ļ�), ��Ҫ�����ߵ���std::free�ͷ�
        auto Execute(const PathFD::Finder& fd) noexcept->Path* override;
        // ���ӻ�����
        void BeginStep(const PathFD::Finder& fd) noexcept override;
        // ���ӻ�����
        bool NextStep(void* cells, void* num) noexcept override;
        // �������ӻ�����
        void EndStep() noexcept override;
    public:
        // �˳�
        bool IsExit() const { return m_bExit; }
        // ���
        void SetFinished() { m_bFinished = true; }
    private:
        // ��������
        ~CFDDijkstra() noexcept;
    private:
        // ��������
        StepOp                  m_opStep;
        // �߳�����
        PathFD::Finder          m_fdData;
        // ִ���߳�
        std::thread             m_thdStep;
        // �˳��ź�
        std::atomic_bool        m_bExit = false;
        // �˳��ź�
        std::atomic_bool        m_bFinished = false;
    };
}


/// <summary>
/// ����A*�㷨
/// </summary>
/// <returns>A*�㷨�ӿ�</returns>
auto PathFD::CreateDijkstraAlgorithm() noexcept -> IFDAlgorithm* {
    return new(std::nothrow) PathFD::CFDDijkstra;
}


/// <summary>
/// <see cref="CFDDijkstra"/> �๹�캯��
/// </summary>
PathFD::CFDDijkstra::CFDDijkstra() noexcept : m_opStep(*this) {

}


// pathfd::impl �����ռ�
namespace PathFD { namespace impl {
    // �ҵ�·��
    auto path_found(const PathFD::CFDDijkstra::List& list) noexcept {
        assert(!list.empty());
        size_t size = size_t(list.back().step);
        assert(size);
        auto noop = list.crend();
        // ���ҽ��
        auto find_node = [noop](decltype(noop)& itr) noexcept -> decltype(noop)& {
            const auto& node = *itr;
            decltype(node.x) parentx = node.x + node.px;
            decltype(node.y) parenty = node.y + node.py;
            ++itr;
            while (itr != noop) {
                if (itr->x == parentx && itr->y == parenty) {
                    break;
                }
                ++itr;
            }
            return itr;
        };
#ifdef _DEBUG
        size_t debug_count = 0;
        {
            auto itr = list.crbegin();
            while (find_node(itr) != noop) ++debug_count;
        }
        assert(debug_count == size);
#endif
        {
            // ����ռ�
            auto* path = reinterpret_cast<PathFD::Path*>(std::malloc(
                sizeof(PathFD::Path) + sizeof(PathFD::Path::pt[0]) * size
            ));
            // ��Ч
            if (path) {
                path->len = uint32_t(size);
                auto pp = path->pt + size;
                auto itr = list.crbegin();
                while (find_node(itr) != noop) {
                    --pp;
                    pp->x = itr->x;
                    pp->y = itr->y;
                }
            }
            return path;
        }
    }
    // Ѱ��·��ex
    template<typename OP>
    auto dijk_find_ex(OP& op, const PathFD::Finder& fd) {
        // ����յ�����
        const int16_t sx = fd.startx;
        const int16_t sy = fd.starty;
        const int16_t gx = fd.goalx;
        const int16_t gy = fd.goaly;
        // ����������
        auto visited = std::make_unique<uint8_t[]>(fd.width * fd.height);
        std::memset(visited.get(), 0, sizeof(uint8_t) * fd.width * fd.height);
        // �����Ҫ����
        auto mk_ptr = visited.get(); int16_t mk_width = fd.width;
        // ��Ǳ���
        auto mark_visited = [mk_ptr, mk_width](int16_t x, int16_t y) noexcept {
            mk_ptr[x + y * mk_width] = true;
        };
        // �����
        auto check_visited = [mk_ptr, mk_width](int16_t x, int16_t y) noexcept {
            return mk_ptr[x + y * mk_width];
        };
        // ���ͨ��
        auto cp_ptr = fd.data; int16_t cp_width = fd.width; int16_t cp_height = fd.height;
        auto check_pass = [cp_ptr, cp_width, cp_height](int16_t x, int16_t y) noexcept {
            if (x >= 0 && x < cp_width && y >= 0 && y < cp_height) {
                return !!cp_ptr[x + y * cp_width];
            }
            return false;
        };
        // �������
        CFDDijkstra::NODE start; 
        start.x = sx; start.y = sy;
        start.px = 0; start.py = 0; start.step = 0;
        // �յ�����
        struct { decltype(start.x) x, y; } end;
        end.x = gx; end.y = gy;
        // ������OPEN��
        CFDDijkstra::List list;
        mark_visited(start.x, start.y);
        constexpr size_t reserved = 1024 * 64;
        list.reserve(reserved);
        list.push_back(start);
        // OPEN��ʼλ��
        size_t open_begin_index = 0;
        // OPEN�յ�λ��
        size_t open_end_index = 1;
        // Ϊ�������ñ�����
        op.set_list(list);
        // Ϊ���㷨ʧ��
        while (!list.empty() && op.go_on()) {
            // �ӱ�ȡһ�����
            for (auto i = open_begin_index; i != open_end_index; ++i) {
                // �ӱ�ȡһ�����, ����������ʧЧ
                const auto node = list[i];
                // Ŀ���
                if (node.x == end.x && node.y == end.y) {
                    // ����
                    impl::auto_locker<OP> locker(op);
                    // ���ô�С
                    list.resize(i + 1);
                    // ��
                    return op.found(list);
                }
                // �ƶ�
                auto moveto = [&](int8_t xplus, int8_t yplus) {
                    CFDDijkstra::NODE tmp; 
                    tmp.x = node.x + xplus; tmp.y = node.y + yplus; 
                    // ����ͨ�� ����û�б�����
                    if (check_pass(tmp.x, tmp.y) && !check_visited(tmp.x, tmp.y)) {
                        // ���
                        mark_visited(tmp.x, tmp.y);
                        // ��¼���ڵ�λ��
                        tmp.px = -xplus; tmp.py = -yplus;
                        // ����
                        tmp.step = node.step + 1;
                        // ����
                        impl::auto_locker<OP> locker(op);
                        // ��ӵ����
                        list.push_back(tmp);
                    }
                };
                // ��
                moveto( 0,+1);
                // ��
                moveto(-1, 0);
                // ��
                moveto(+1, 0);
                // ��
                moveto( 0,-1);
            }
            // д������
            open_begin_index = open_end_index;
            open_end_index = list.size();
            op.set_open_list_begin(open_begin_index);
            // �ȴ�һ��
            op.wait();
        }
        // ����
        impl::auto_locker<OP> locker(op);
        // ʧ��
        return op.failed();
    }
}}


// ִ���㷨
auto PathFD::CFDDijkstra::Execute(const PathFD::Finder& fd) noexcept -> PathFD::Path* {
    // ��ʽ����
    try { impl::dijk_op op; return impl::dijk_find_ex(op, fd); }
    // �����쳣
    catch (...) { return nullptr; }
}

// ���ӻ�����
void PathFD::CFDDijkstra::BeginStep(const PathFD::Finder& fd) noexcept {
    assert(m_thdStep.joinable() == false);
    std::this_thread::yield();
    m_fdData = fd;
    try {
        auto& data = m_fdData;
        auto& op = m_opStep;
        m_thdStep.std::thread::~thread();
        m_thdStep.std::thread::thread([&op, &data]() {
            impl::dijk_find_ex(op, data);
        });
    }
    catch (...) { m_opStep.failed(); }
}
// ���ӻ�����
bool PathFD::CFDDijkstra::NextStep(void* cells, void* num) noexcept {
    assert(cells && "bad pointer");
    // ��������ǰ����
    if (m_bFinished) return true;
    //auto count = m_fdData.width * m_fdData.height;
    {
        impl::color red;
        red.r = 1.f; red.g = 0.f; red.b = 0.f; red.a = 1.f;
        impl::color green;
        green.r = 0.f; green.g = 1.f; green.b = 0.f; green.a = 1.f;
        // ��ȡ����
        m_opStep.lock();
        assert(m_opStep.list);
        static uint32_t s_index;
        s_index = 0;
        // ��ʾ�ڵ�����
        auto nodedisplay = [num](const NODE& node) noexcept {
            static const CharacterDirection DARRAY[] = {
                Direction_N, Direction_W,  Direction_S, Direction_E
            };
            // ���㷽��
            auto cal_direction = [](int16_t x, int16_t y) noexcept {
                auto a = ((x + 1) >> 1);
                auto b = (y + 1);
                auto i = (a | b) + (a & b) * 2;
                assert(i < 4);
                return DARRAY[i];
            };
            constexpr uint32_t COUNT = 1;
            char buffer[sizeof(NodeDisplay) + sizeof(uint32_t) * COUNT];
            auto& numdis = reinterpret_cast<NodeDisplay&>(*buffer);
            // ����
            numdis.x = node.x;
            numdis.y = node.y;
            numdis.i = s_index++;
            numdis.argc = COUNT;
            // �ڵ�
            numdis.d = cal_direction(node.px, node.py);
            // gn = 
            numdis.argv[0] = 0;
            // ��ʾ����
            impl::set_node_display(num, &numdis);
        };
        auto& list = *m_opStep.list;
        auto openbegin = list.begin() + m_opStep.openbegin;
        // ΪCLOSE�������ɫ
        for (auto itr = list.begin(); itr != openbegin; ++itr) {
            const auto& node = *itr;
            uint32_t index = node.x + node.y * m_fdData.width;
            impl::set_cell_color(cells, index, green);
            nodedisplay(node);
        }
        // ΪOPEN����Ӻ�ɫ
        for (auto itr = openbegin; itr != list.end(); ++itr) {
            const auto& node = *itr;
            uint32_t index = node.x + node.y * m_fdData.width;
            impl::set_cell_color(cells, index, red);
            nodedisplay(node);
        }
        // �����ݷ�����
        m_opStep.unlock();
    }
    // ��ʾ��һ��
    m_opStep.signal();
    return false;
}

// �������ӻ�����
void PathFD::CFDDijkstra::EndStep() noexcept {
    assert(m_thdStep.joinable());
    m_bExit = true;
    m_opStep.signal();
    try { 
        m_thdStep.join();
        m_thdStep.std::thread::~thread();
        m_thdStep.std::thread::thread();
    }
    catch (...) { }
}

// ��������
PathFD::CFDDijkstra::~CFDDijkstra() noexcept {
    m_opStep.signal();
    m_bExit = true;
    if (m_thdStep.joinable()) m_thdStep.join();
}

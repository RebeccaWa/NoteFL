#include "pfdAlgorithm.h"
#include "pdfImpl.h"

#include <algorithm>
#include <cassert>
#include <atomic>
#include <thread>
#include <memory>
#include <list>
#include <new>

/*

A*�㷨���̣�

���Ƚ���ʼ���S����OPEN��CLOSE���ÿգ��㷨��ʼʱ��
1�����OPEN��Ϊ�գ��ӱ�ͷȡһ�����n�����Ϊ���㷨ʧ�ܡ�

2��n��Ŀ������ǣ��ҵ�һ���⣨����Ѱ�ң�����ֹ�㷨����

3����n�����к�̽��չ�������Ǵ�n����ֱ�ӹ����Ľ�㣨�ӽ�㣩��
�������CLOSE���У��ͽ����Ƿ���OPEN������S����CLOSE��
ͬʱ����ÿһ����̽��Ĺ���ֵf(n)����OPEN��f(x)����
��С�ķ��ڱ�ͷ���ظ��㷨���ص�1��
*/

// pathfd �����ռ�
namespace PathFD {
    // impl
    namespace impl {
        // ����������
        template<typename T> struct astar_step_op {
            // ����OPEN��
            template<typename Y>inline void set_open_list(Y& list) {
                openlist = &list;
            }
            // ����CLOSE��
            template<typename Y>inline void set_close_list(Y& list) {
                closelist = &list;
            }
            // ��ȡһ���ڵ�
            template<typename Y> inline void get_node(const Y& node) {
                int bk = 9;
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
            astar_step_op(T& parent) : parent(parent) {}
            // ���챾�ຯ��
            ~astar_step_op() { impl::destroy(ev); impl::destroy(mx);}
            // �¼�
            event               ev = impl::create_event();
            // ������
            mutex               mx = impl::create_mutex();
            // ������
            T&                  parent;
            // OPEN��
            typename T::List*   openlist = nullptr;
            // CLOSE��
            typename T::List*   closelist = nullptr;
        };
    }
    // �Զ��������
    template<typename T> class Allocator {
    public : 
        //    typedefs
        typedef T value_type;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

    public : 
        //    convert an allocator<T> to allocator<U>
        template<typename U>
        struct rebind {
            typedef Allocator<U> other;
        };

    public : 
        inline explicit Allocator() {}
        inline ~Allocator() {}
        inline explicit Allocator(Allocator const&) {}
        template<typename U>
        inline explicit Allocator(Allocator<U> const&) {}

        //    address
        inline pointer address(reference r) { return &r; }
        inline const_pointer address(const_reference r) { return &r; }

        //    memory allocation
        inline pointer allocate(size_type cnt, 
            typename std::allocator<void>::const_pointer = 0) { 
            void* ptr = PathFD::AllocSmall(cnt * sizeof(T));
            if (!ptr) throw(std::bad_alloc());
            return reinterpret_cast<pointer>(ptr); 
        }
        inline void deallocate(pointer p, size_type) {
            PathFD::FreeSmall(p);
        }

        //    size
        inline size_type max_size() const { 
            return std::numeric_limits<size_type>::max() / sizeof(T);
        }

        //    construction/destruction
        inline void construct(pointer p, const T& t) { new(p) T(t); }
        inline void destroy(pointer p) { p->~T(); }

        inline bool operator==(Allocator const&) { return true; }
        inline bool operator!=(Allocator const& a) { return !operator==(a); }

    };
    // A*�㷨
    class CFDAStar final : public IFDAlgorithm {
    public:
        // �ڵ�
        struct NODE {
            // ����
            int16_t         x, y;
            // �ڵ����(gn)
            int16_t         gn;
            // �ڵ��ֵ(fx)
            int16_t         fx;
            // ���ڵ�
            const NODE*     parent;
        };
        // �б�
        using List = std::list<CFDAStar::NODE, Allocator<CFDAStar::NODE>>;
        // ����
        using StepOp = impl::astar_step_op<CFDAStar>;
    public:
        // ���캯��
        CFDAStar() noexcept;
        // �ͷŶ���
        void Dispose() noexcept override { delete this; };
        // ִ���㷨, ����·��(�ɹ��Ļ�), ��Ҫ�����ߵ���std::free�ͷ�
        auto Execute(const PathFD::Finder& fd) noexcept->Path* override;
        // ���ӻ�����
        void BeginStep(const PathFD::Finder& fd) noexcept override;
        // ���ӻ�����
        bool NextStep(void* cells) noexcept override;
        // �������ӻ�����
        void EndStep() noexcept override;
    public:
        // �˳�
        bool IsExit() const { return m_bExit; }
        // ���
        void SetFinished() { m_bFinished = true; }
    private:
        // ��������
        ~CFDAStar() noexcept;
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
auto PathFD::CreateAStarAlgorithm() noexcept -> IFDAlgorithm* {
    return new(std::nothrow) PathFD::CFDAStar;
}


/// <summary>
/// <see cref="CFDAStar"/> �๹�캯��
/// </summary>
PathFD::CFDAStar::CFDAStar() noexcept : m_opStep(*this) {

}


// pathfd::impl �����ռ�
namespace PathFD { namespace impl {
    // �ҵ�·��
    auto path_found(PathFD::CFDAStar::List& close_list) noexcept {
        assert(!close_list.empty());
        size_t size = size_t(close_list.front().gn);
#ifdef _DEBUG
        size_t debug_count = 0;
        {
            const auto* itr = &close_list.front();
            while (itr->parent) {
                ++debug_count;
                itr = itr->parent;
            }
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
                // �������
                const auto* itr = &close_list.front();
                while (itr->parent) {
                    --pp;
                    pp->x = itr->x;
                    pp->y = itr->y;
                    itr = itr->parent;
                }
            }
            return path;
        }
    }
    // Ѱ��·��
    auto a_star_find(const PathFD::Finder& fd) -> PathFD::Path* {
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
        // ��ֵ���� f(n)=g(n)+h(n)
        auto hn = [=](int16_t x, int16_t y) noexcept -> int16_t {
#if 0
            auto xxx = std::abs(x - gx);
            auto yyy = std::abs(y - gy);
            auto maxone = std::max(xxx, yyy);
            auto minone = std::min(xxx, yyy);
            return minone * 3 + (maxone - minone) * 2;
#else
            return std::abs(x - gx) + std::abs(y - gy);
#endif
        };
        // ���ͨ��
        auto cp_ptr = fd.data; int16_t cp_width = fd.width; int16_t cp_height = fd.height;
        auto check_pass = [cp_ptr, cp_width, cp_height](int16_t x, int16_t y) noexcept {
            if (x >= 0 && x < cp_width && y >= 0 && y < cp_height) {
                return !!cp_ptr[x + y * cp_width];
            }
            return false;
        };
        // ����յ�����
        CFDAStar::NODE start, end; 
        start.x = sx; start.y = sy;
        start.gn = 0;
        start.fx = hn(start.x, start.y);
        start.parent = nullptr;
        end.x = gx; end.y = gy;
        // ������OPEN��
        CFDAStar::List open, close; open.push_back(start);
        mark_visited(start.x, start.y);
        // Ϊ���㷨ʧ��
        while (!open.empty()) {
            // �ӱ�ͷȡһ����� ��ӵ�CLOSE��
            close.push_front(open.front());
            // ����
            open.pop_front();
            // ��ȡ
            const auto& node = close.front();
            // Ŀ���
            if (node.x == end.x && node.y == end.y) {
                return impl::path_found(close);
            }
            // �ƶ�
            auto moveto = [&](int16_t xplus, int16_t yplus) {
                CFDAStar::NODE tmp; 
                tmp.x = node.x + xplus; 
                tmp.y = node.y + yplus; 
                // ����ͨ�� ����û�б�����
                if (check_pass(tmp.x, tmp.y) && !check_visited(tmp.x, tmp.y)) {
                    // ���
                    mark_visited(tmp.x, tmp.y);
                    // ��¼���ڵ�λ��
                    tmp.parent = &node;
                    // ����g(n)
                    tmp.gn = node.gn + 1;
                    // f(n) = g(n) + h(n)
                    tmp.fx = tmp.gn + hn(tmp.x, tmp.y);
                    // �����Ķ���?
                    if (open.empty() || tmp.fx >= open.back().fx) {
                        // ��ӵ����
                        open.push_back(tmp);
                        return;
                    }
                    // ��ӽڵ�
                    for (auto itr = open.begin(); itr != open.end(); ++itr) {
                        if (tmp.fx < itr->fx) {
                            open.insert(itr, tmp);
                            return;
                        }
                    }
                    // ������
                    assert(!"Impossible ");
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
        return nullptr;
    }
    // Ѱ��·��ex
    auto a_star_find_ex(CFDAStar::StepOp& op, const PathFD::Finder& fd) {
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
        // ��ֵ���� f(n)=g(n)+h(n)
        auto hn = [=](int16_t x, int16_t y) noexcept -> int16_t {
#if 0
            auto xxx = std::abs(x - gx);
            auto yyy = std::abs(y - gy);
            auto maxone = std::max(xxx, yyy);
            auto minone = std::min(xxx, yyy);
            return minone * 3 + (maxone - minone) * 2;
#else
            return std::abs(x - gx) + std::abs(y - gy);
#endif
        };
        // ���ͨ��
        auto cp_ptr = fd.data; int16_t cp_width = fd.width; int16_t cp_height = fd.height;
        auto check_pass = [cp_ptr, cp_width, cp_height](int16_t x, int16_t y) noexcept {
            if (x >= 0 && x < cp_width && y >= 0 && y < cp_height) {
                return !!cp_ptr[x + y * cp_width];
            }
            return false;
        };
        // ����յ�����
        CFDAStar::NODE start, end; 
        start.x = sx; start.y = sy;
        start.gn = 0;
        start.fx = hn(start.x, start.y);
        start.parent = nullptr;
        end.x = gx; end.y = gy;
        // ������OPEN��
        CFDAStar::List open, close; open.push_back(start);
        mark_visited(start.x, start.y);
        // Ϊ�������ñ�����
        op.set_open_list(open); op.set_close_list(close);
        // Ϊ���㷨ʧ��
        while (!open.empty() && op.go_on()) {
            // ����
            op.lock();
            // �ӱ�ͷȡһ����� ��ӵ�CLOSE��
            close.push_front(open.front());
            // �¼�����
            op.get_node(close.front());
            // ����
            open.pop_front();
            // ��ȡ
            const auto& node = close.front();
            // ����
            op.unlock();
            // Ŀ���
            if (node.x == end.x && node.y == end.y) {
                op.found(close);
                return;
            }
            // �ƶ�
            auto moveto = [&](int16_t xplus, int16_t yplus) {
                CFDAStar::NODE tmp; 
                tmp.x = node.x + xplus; 
                tmp.y = node.y + yplus; 
                // ����ͨ�� ����û�б�����
                if (check_pass(tmp.x, tmp.y) && !check_visited(tmp.x, tmp.y)) {
                    // ���
                    mark_visited(tmp.x, tmp.y);
                    // ��¼���ڵ�λ��
                    tmp.parent = &node;
                    // ����g(n)
                    tmp.gn = node.gn + 1;
                    // f(n) = g(n) + h(n)
                    tmp.fx = tmp.gn + hn(tmp.x, tmp.y);
                    // ����
                    op.lock();
                    // �����Ķ���?
                    if (open.empty() || tmp.fx >= open.back().fx) {
                        // ��ӵ����
                        open.push_back(tmp);
                        // ����
                        op.unlock();
                        return;
                    }
                    // ��ӽڵ�
                    for (auto itr = open.begin(); itr != open.end(); ++itr) {
                        if (tmp.fx < itr->fx) {
                            open.insert(itr, tmp);
                            // ����
                            op.unlock();
                            return;
                        }
                    }
                    // ������
                    assert(!"Impossible ");
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
            // �ȴ�һ��
            op.wait();
        }
        return op.failed();
    }
}}


// ִ���㷨
auto PathFD::CFDAStar::Execute(const PathFD::Finder& fd) noexcept -> PathFD::Path* {
    // ��ʽ����
    try { return impl::a_star_find(fd);  }
    // �����쳣
    catch (...) { return nullptr; }
}

// ���ӻ�����
void PathFD::CFDAStar::BeginStep(const PathFD::Finder& fd) noexcept {
    assert(m_thdStep.joinable() == false);
    std::this_thread::yield();
    m_fdData = fd;
    try {
        auto& data = m_fdData;
        auto& op = m_opStep;
        m_thdStep.std::thread::~thread();
        m_thdStep.std::thread::thread([&op, &data]() {
            impl::a_star_find_ex(op, data);
        });
    }
    catch (...) { m_opStep.failed(); }
}
// ���ӻ�����
bool PathFD::CFDAStar::NextStep(void* cells) noexcept {
    assert(cells && "bad pointer");
    // ��������ǰ����
    if (m_bFinished) return true;
    //auto count = m_fdData.width * m_fdData.height;
    impl::color red;
    red.r = 1.f; red.g = 0.f; red.b = 0.f; red.a = 1.f;
    impl::color green;
    green.r = 0.f; green.g = 1.f; green.b = 0.f; green.a = 1.f;
    impl::color white;
    white.r = white.g = white.b = white.a = 1.4f;
    // ��ȡ����
    m_opStep.lock();
    {
        assert(m_opStep.openlist && m_opStep.closelist);
        // ΪOPEN����Ӻ�ɫ
        for (const auto& node : (*m_opStep.openlist)) {
            uint32_t index = node.x + node.y * m_fdData.width;
            impl::set_cell_color(cells, index, red);
        }
        // ΪCLOSE�������ɫ
        for (const auto& node : (*m_opStep.closelist)) {
            uint32_t index = node.x + node.y * m_fdData.width;
            impl::set_cell_color(cells, index, green);
        }
        // ΪOPEN��ͷ�����ɫ
        if (!m_opStep.openlist->empty()) {
            const auto& node = m_opStep.openlist->front();
            uint32_t index = node.x + node.y * m_fdData.width;
            impl::set_cell_color(cells, index, white);
        }
    }
    // �����ݷ�����
    m_opStep.unlock();
    // ��ʾ��һ��
    m_opStep.signal();
    return false;
}

// �������ӻ�����
void PathFD::CFDAStar::EndStep() noexcept {
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
PathFD::CFDAStar::~CFDAStar() noexcept {
    m_opStep.signal();
    m_bExit = true;
    if (m_thdStep.joinable()) m_thdStep.join();
}

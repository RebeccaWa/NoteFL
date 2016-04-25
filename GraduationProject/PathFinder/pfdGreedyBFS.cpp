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

̰����������㷨���̣�

���Ƚ���ʼ���S����OPEN��CLOSE���ÿգ��㷨��ʼʱ��
1�����OPEN��Ϊ�գ��ӱ�ͷȡһ�����n�����Ϊ���㷨ʧ�ܡ�

2��n��Ŀ������ǣ��ҵ�һ���⣨����Ѱ�ң�����ֹ�㷨����

3����n�����к�̽��չ�������Ǵ�n����ֱ�ӹ����Ľ�㣨�ӽ�㣩��
�������CLOSE���У��ͽ����Ƿ���OPEN������S����CLOSE��
ͬʱ����ÿһ����̽�������ֵh(n)����OPEN��h(x)����
��С�ķ��ڱ�ͷ���ظ��㷨���ص�1��
*/

// pathfd �����ռ�
namespace PathFD {
    // impl
    namespace impl {
        // ����������
        template<typename T> struct gbfs_step_op {
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
                nodex = node.x;
                nodey = node.y;
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
            gbfs_step_op(T& parent) : parent(parent) {}
            // ���챾�ຯ��
            ~gbfs_step_op() { impl::destroy(ev); impl::destroy(mx);}
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
            // ѡ��ڵ�X
            uint32_t            nodex = 0;
            // ѡ��ڵ�Y
            uint32_t            nodey = 0;
        };
        // ������
        struct gbfs_op {
            // ����OPEN��
            template<typename Y> inline void set_open_list(Y& list) {  }
            // ����CLOSE��
            template<typename Y> inline void set_close_list(Y& list) { }
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
    // �Զ��������
    template<typename T> class AllocatorEx {
        // CHAIN
        struct CHAIN { CHAIN* next; size_t used; char buffer[0]; };
        // buffer length
        enum : size_t { CHAIN_SIZE = 1024 * 1024, BUFFER_SIZE = CHAIN_SIZE - sizeof(void*) * 2 };
        // memory chain
        CHAIN*              m_pHeader = nullptr;
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
            typedef AllocatorEx<U> other;
        };

    public : 
        // dtor
        explicit AllocatorEx() {}
        // dtor
        ~AllocatorEx() {
            auto node = m_pHeader;
            while (node) {
                auto tmp = node;
                node = node->next;
                std::free(tmp);
            }
            m_pHeader = nullptr;
        }
        inline explicit AllocatorEx(AllocatorEx const&) = delete;
        template<typename U>
        inline explicit AllocatorEx(AllocatorEx<U> const&) = delete;

        //    address
        inline pointer address(reference r) { return &r; }
        inline const_pointer address(const_reference r) { return &r; }

        //    memory allocation
        inline pointer allocate(size_type cnt, 
            typename std::allocator<void>::const_pointer = 0) { 
            void* ptr = this->alloc(cnt * sizeof(T));
            if (!ptr) throw(std::bad_alloc());
            return reinterpret_cast<pointer>(ptr); 
        }
        inline void deallocate(pointer p, size_type) {
            this->free(p);
        }

        //    size
        inline size_type max_size() const { 
            return std::numeric_limits<size_type>::max() / sizeof(T);
        }

        //    construction/destruction
        inline void construct(pointer p, const T& t) { new(p) T(t); }
        inline void destroy(pointer p) { p->~T(); }

        inline bool operator==(AllocatorEx const&) { return true; }
        inline bool operator!=(AllocatorEx const& a) { return !operator==(a); }
    private:
        // free
        inline void free(const void*) {  }
        // reserve
        auto reserve(size_t len) noexcept ->CHAIN* {
            assert(len < BUFFER_SIZE && "out of range");
            // check
            if (!m_pHeader || (m_pHeader->used + len) > BUFFER_SIZE) {
                auto new_header = reinterpret_cast<CHAIN*>(std::malloc(CHAIN_SIZE));
                if (!new_header) return nullptr;
                new_header->next = m_pHeader;
                new_header->used = 0;
                m_pHeader = new_header;
            }
            return m_pHeader;
        }
        // alloc buffer
        auto alloc(size_t len) noexcept ->void* {
            assert(len < BUFFER_SIZE && "bad action");
            void* address = nullptr;
            auto chian = this->reserve(len);
            if (chian) {
                address = chian->buffer + chian->used;
                chian->used += len;
            }
            return address;
        }
    };
    // GreedyBFS�㷨
    class CFDGreedyBFS final : public IFDAlgorithm {
    public:
        // �ڵ�
        struct NODE {
            // ����
            int16_t         x, y;
            // �ڵ�����ֵ(gn)
            uint32_t        hn;
            // ���ڵ�
            const NODE*     parent;
        };
        // �б�
#ifdef _DEBUG
        using List = std::list<CFDGreedyBFS::NODE>;
#else
        using List = std::list<CFDGreedyBFS::NODE, AllocatorEx<CFDGreedyBFS::NODE>>;
#endif
        // ����
        using StepOp = impl::gbfs_step_op<CFDGreedyBFS>;
    public:
        // ���캯��
        CFDGreedyBFS() noexcept;
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
        ~CFDGreedyBFS() noexcept;
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
        // ���ӻ��׶�
        uint16_t                m_uPhase = 0;
    };
}


/// <summary>
/// ����A*�㷨
/// </summary>
/// <returns>A*�㷨�ӿ�</returns>
auto PathFD::CreateGreedyBFSAlgorithm() noexcept -> IFDAlgorithm* {
    return new(std::nothrow) PathFD::CFDGreedyBFS;
}


/// <summary>
/// <see cref="CFDGreedyBFS"/> �๹�캯��
/// </summary>
PathFD::CFDGreedyBFS::CFDGreedyBFS() noexcept : m_opStep(*this) {

}


// pathfd::impl �����ռ�
namespace PathFD { namespace impl {
    // �ҵ�·��
    auto path_found(const PathFD::CFDGreedyBFS::List& close_list) noexcept {
        assert(!close_list.empty());
        size_t size = 0;
        {
            const auto* itr = &close_list.front();
            while (itr->parent) {
                ++size;
                itr = itr->parent;
            }
        }
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
    // Ѱ��·��ex
    template<typename OP>
    auto gbfs_find_ex(OP& op, const PathFD::Finder& fd) {
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
        // �������
        CFDGreedyBFS::NODE start; 
        start.x = sx; start.y = sy;
        start.hn = hn(start.x, start.y);
        start.parent = nullptr;
        // �յ�����
        struct { decltype(start.x) x, y; } end;
        end.x = gx; end.y = gy;
        // ������OPEN��
        CFDGreedyBFS::List open, close; open.push_back(start);
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
                // ����
                impl::auto_locker<OP> locker(op);
                // ����
                return op.found(close);
            }
            // ���
            auto insert2 = [&](const CFDGreedyBFS::NODE& node) {
                // �����Ķ���?
                if (open.empty() || node.hn >= open.back().hn) {
                    // ��ӵ����
                    open.push_back(node);
                    return;
                }
                // ��ӽڵ�
                for (auto itr = open.begin(); itr != open.end(); ++itr) {
                    if (node.hn <= itr->hn) {
                        open.insert(itr, node);
                        return;
                    }
                }
                // ������
                assert(!"Impossible ");
            };
            // �ƶ�
            auto moveto = [&](int16_t xplus, int16_t yplus) {
                CFDGreedyBFS::NODE tmp; 
                tmp.x = node.x + xplus; 
                tmp.y = node.y + yplus; 
                // ����ͨ�� ����û�б�����
                if (check_pass(tmp.x, tmp.y) && !check_visited(tmp.x, tmp.y)) {
                    // ���
                    mark_visited(tmp.x, tmp.y);
                    // ��¼���ڵ�λ��
                    tmp.parent = &node;
                    // ����h(n)
                    tmp.hn = hn(tmp.x, tmp.y);
                    // ����
                    impl::auto_locker<OP> locker(op);
                    // ���
                    insert2(tmp);
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
        // ����
        impl::auto_locker<OP> locker(op);
        // ʧ��
        return op.failed();
    }
}}


// ִ���㷨
auto PathFD::CFDGreedyBFS::Execute(const PathFD::Finder& fd) noexcept -> PathFD::Path* {
    // ��ʽ����
    try { impl::gbfs_op op; return impl::gbfs_find_ex(op, fd); }
    // �����쳣
    catch (...) { return nullptr; }
}

// ���ӻ�����
void PathFD::CFDGreedyBFS::BeginStep(const PathFD::Finder& fd) noexcept {
    assert(m_thdStep.joinable() == false);
    std::this_thread::yield();
    m_fdData = fd;
    try {
        auto& data = m_fdData;
        auto& op = m_opStep;
        m_thdStep.std::thread::~thread();
        m_thdStep.std::thread::thread([&op, &data]() {
            impl::gbfs_find_ex(op, data);
        });
    }
    catch (...) { m_opStep.failed(); }
}
// ���ӻ�����
bool PathFD::CFDGreedyBFS::NextStep(void* cells, void* num) noexcept {
    assert(cells && "bad pointer");
    // ��������ǰ����
    if (m_bFinished) return true;
    // �׶�0: ��ʾ
    if (m_uPhase == 0) {
        //auto count = m_fdData.width * m_fdData.height;
        {
            impl::color red;
            red.r = 1.f; red.g = 0.f; red.b = 0.f; red.a = 1.f;
            impl::color green;
            green.r = 0.f; green.g = 1.f; green.b = 0.f; green.a = 1.f;
            // ��ȡ����
            m_opStep.lock();
            assert(m_opStep.openlist && m_opStep.closelist);
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
                // ���㷽��Ex
                auto cal_direction_ex = [&cal_direction](const NODE& node) noexcept {
                    if (node.parent) {
                        return cal_direction(node.parent->x - node.x, node.parent->y - node.y);
                    }
                    else {
                        return Direction_NE;
                    }
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
                numdis.d = cal_direction_ex(node);
                // hn = 
                numdis.argv[0] = node.hn;
                // ��ʾ����
                impl::set_node_display(num, &numdis);
            };
            // ΪOPEN����Ӻ�ɫ
            for (const auto& node : (*m_opStep.openlist)) {
                uint32_t index = node.x + node.y * m_fdData.width;
                impl::set_cell_color(cells, index, red);
                nodedisplay(node);
            }
            // ΪCLOSE�������ɫ
            for (const auto& node : (*m_opStep.closelist)) {
                uint32_t index = node.x + node.y * m_fdData.width;
                impl::set_cell_color(cells, index, green);
                nodedisplay(node);
            }
            // �����ݷ�����
            m_opStep.unlock();
        }
        // ��ʾ��һ��
        m_opStep.signal();
    }
    else {
        impl::color blue;
        blue.r = 1.f; blue.g = 1.f;  blue.b = 2.f; blue.a = 1.0f;
        uint32_t index = m_opStep.nodex + m_opStep.nodey * m_fdData.width;
        impl::set_cell_color(cells, index, blue);
    }
    // �����׶�
    m_uPhase = !m_uPhase;
    return false;
}

// �������ӻ�����
void PathFD::CFDGreedyBFS::EndStep() noexcept {
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
PathFD::CFDGreedyBFS::~CFDGreedyBFS() noexcept {
    m_opStep.signal();
    m_bExit = true;
    if (m_thdStep.joinable()) m_thdStep.join();
}

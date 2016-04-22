#pragma once

#include <luibase.h>
#include <luiconf.h>
#include <Control/UIControl.h>
#include <Platonly/luiPoHlper.h>
#include "pfdCharacter.h"

// �ļ��Ի���
struct IFileDialog;

// pathfd �����ռ�
namespace PathFD {
    // ·��
    struct Path;
    // �㷨
    struct IFDAlgorithm;
    // �Թ������㷨
    using DungeonGeneration = bool(*)(uint8_t* data, uint32_t w, uint32_t h, uint32_t []);
    // Ĭ���Թ������㷨
    bool DefaultGeneration(uint8_t* data, uint32_t w, uint32_t h, uint32_t []) noexcept;
    // ��ͼ�ؼ�
    class UIMapControl final : public LongUI::UIControl {
        // ���
        enum ClickinType : uint16_t {
            Type_Cell = 0,      // ��Ԫ��
            Type_Charactar,     // ��ɫ��
            Type_Start,         // ����
            Type_Goal,          // �յ��
        };
        // ����
        using Super = UIControl;
        // ����ؼ�
        virtual void cleanup() noexcept override;
        // ˫��������
        using DoubleClickHelper = LongUI::Helper::DoubleClick;
    public:
        // ѡ�����
        static constexpr float  CEEL_SELECT_WIDTH = 4.f;
        // ����
        enum : uint32_t {
            // ��ͼ�����
            MAX_WIDTH = 1024,
            // ��ͼ���߶�
            MAX_HEIGHT = 1024,
            // ��ͼ��λ��ȳ�ʼ��ֵ
            CELL_WIDTH_INIT = 32,
            // ��ͼ��λ�߶ȳ�ʼ��ֵ
            CELL_HEIGHT_INIT = 32,
            // ·��Ȩ�ؽڵ���
            WEIGHT_NODE_BITMAP_WH = 1024,
            // ����ԴXƫ��(����)
            DIGNUMNER_SRCX_OFFSET = 0,
            // ����ԴYƫ��(����)
            DIGNUMNER_SRCY_OFFSET = 32 * 2,
            // ���ֿ��(����)
            DIGNUMNER_WIDTH = 4,
            // ���ָ߶�(����)
            DIGNUMNER_HEIGHT = 8,
            // ����λ��
            NUMCOUNT = 4,
            // ���ֱ���(����)
            NUMBERTABLE_WIDTH = 1024,
            // ���ֱ�߶�(����)
            NUMBERTABLE_HEIGHT = 1024,
        };
    public:
        // ��Ⱦ 
        virtual void Render() const noexcept override;
        // ˢ��
        virtual void Update() noexcept override;
        // �¼�����
        virtual bool DoEvent(const LongUI::EventArgument& arg) noexcept override;
        // ����¼�
        virtual bool DoMouseEvent(const LongUI::MouseEventArgument& arg) noexcept override;
        // �ؽ�
        virtual auto Recreate() noexcept ->HRESULT override;
    public:
        // �����ͼ
        void LoadMap() noexcept;
        // �����ͼ
        void LoadMap(const wchar_t* filename) noexcept;
        // �����ͼ
        void SaveMap() noexcept;
        // �����ͼ
        void SaveMap(const wchar_t* filename) noexcept;
        // ��ͼ
        void GenerateMap(uint32_t width, uint32_t height) noexcept;
        // ���õ�ͼ��λ��С
        void ResizeCellSize(uint32_t width, uint32_t height) noexcept;
        // ���ŵ�ͼ
        void ZoomMapTo(float zoom, float time) noexcept;
        // ִ��Ѱ·
        void Execute(IFDAlgorithm* algorithm, LongUI::CUIString& info) noexcept;
        // ��ʼ��ʾ
        void BeginShow(IFDAlgorithm*&& algorithm) noexcept;
        // ִ����һ��
        void ExeNextStep() noexcept;
        // ��ͣ�ָ�
        void PauseResume() noexcept;
        // ���ò������ʱ��
        void SetStepDeltaTime(float time) noexcept { m_fAlgorithmStepTimeAll = time; }
    public:
        // ���캯��
        UIMapControl(LongUI::UIContainer* cp) noexcept;
        // �����ؼ�
        static auto CreateControl(LongUI::CreateEventType type, pugi::xml_node) noexcept ->UIControl*;
    protected:
        // ��������
        ~UIMapControl() noexcept;
        // ��Ⱦ��: ��Ⱦ����
        void render_chain_background() const noexcept;
        // ��Ⱦ��: ��Ⱦ����
        void render_chain_main() const noexcept;
        // ��Ⱦ��: ��Ⱦ����
        void render_chain_foreground() const noexcept;
        // ɾ��ǰ����
        void before_deleted() noexcept { Super::before_deleted(); }
        // ��ʼ��
        void initialize(pugi::xml_node node) noexcept;
        // ��֤������Ч
        bool require_mapdata(int32_t width, uint32_t height) noexcept;
        // �ؽ��豸��Դ
        void release_resource() noexcept;
        // ���õ�ͼ
        void reset_map() noexcept;
        // ���þ���
        void reset_sprites() noexcept;
        // ��ȡ��ɫ����
        auto&get_char_data() noexcept { return *reinterpret_cast<CharData*>(m_bufCharData); }
        // ִ����һ��
        void exe_next_step() noexcept;
#ifdef LongUIDebugEvent
    protected:
        // debug infomation
        virtual bool debug_do_event(const LongUI::DebugEventInformation&) const noexcept override;
#endif
    private:
        // ��ǰ�㷨
        IFDAlgorithm*           m_pAlgorithm = nullptr;
        // �ļ��򿪶Ի���
        IFileDialog*            m_pFileOpenDialog = nullptr;
        // �ļ�����Ի���
        IFileDialog*            m_pFileSaveDialog = nullptr;
        // ��ͼ�ֽ��߱�ˢ
        ID2D1BitmapBrush*       m_pCellBoundaryBrush = nullptr;
        // ��ͼ���鼯
        ID2D1SpriteBatch*       m_pMapSpriteBatch = nullptr;
        // ·����ʾ���鼯
        ID2D1SpriteBatch*       m_pPathDisplay = nullptr;
        // ��ͼ�Զ���Ƭ��ͼ����
        ID2D1Bitmap1*           m_pAutoTileCache = nullptr;
        // ��ͼƤ��
        ID2D1Bitmap1*           m_pMapSkin = nullptr;
        // ��ͼ���ͼ��
        ID2D1Bitmap1*           m_pMapIcon = nullptr;
        // ���ֱ�λͼ
        ID2D1Bitmap1*           m_pNumnberTable = nullptr;
        // ��ͼ����
        uint8_t*                m_pMapCells = nullptr;
        // ·����Ϣ
        PathFD::Path*           m_pPath = nullptr;
        // �Թ������㷨
        DungeonGeneration       m_fnGeneration = PathFD::DefaultGeneration;
        // ��ɫ ��ʾ
        CFDCharacter            m_char;
        // ��ͼ����
        PathFD::MapData         m_dataMap;
        // ˫��������
        DoubleClickHelper       m_hlpDbClick;
        // ��ͼ��ǰ��������
        uint32_t                m_uMapSpriteCount = 0;
        // ·����ǰ��������
        //uint32_t                m_uPathSpriteCount = 0;
        // ѡ�е�xλ��
        uint32_t                m_uClickX = uint32_t(-1);
        // ѡ�е�xλ��
        uint32_t                m_uClickY = uint32_t(-1);
        // �յ�xλ��
        uint32_t                m_uGoalX = 0;
        // �յ�xλ��
        uint32_t                m_uGoalY = 0;
        // �㷨 �ܹ����ʱ��
        float                   m_fAlgorithmStepTimeAll = 0.3f;
        // �㷨 ��ǰ���ʱ��
        float                   m_fAlgorithmStepTimeNow = 0.f;
        // ���������
        ClickinType             m_typeClicked = Type_Cell;
        // ��ɫλͼ
        uint16_t                m_uCharBitmap = 0;
        // ��ͼλͼ
        uint16_t                m_uMapBitmap = 0;
        // ͼ��λͼid
        uint16_t                m_idMapIcon = 0;
        // δʹ��
        uint16_t                m_unused_u16_map[2];
        // ��ɫ���ݻ��� :Ŀǰֻ��Ҫ4������
        char                    m_bufCharData[sizeof(CharData) + sizeof(CharData::action[0]) * 4];
    };
}

#ifdef LongUIDebugEvent
// longui namespace
namespace LongUI {
    // ����?������ GetIID
    template<> LongUIInline const IID& GetIID<PathFD::UIMapControl>() {
        // {F46C5DE3-D761-4E9A-9D1A-4A301BB45CBE}
        static const GUID IID_PathFD_UIMapControl = {
            0xf46c5de3, 0xd761, 0x4e9a, { 0x9d, 0x1a, 0x4a, 0x30, 0x1b, 0xb4, 0x5c, 0xbe }
        };
        return IID_PathFD_UIMapControl;
    }
}
#endif

// 画面にステータス情報を表示する。
// ロックマンの体力バーのようなもの
// hudという名前は良くなかった気がするが変えられないのでそのままとする

#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// アイテムを左右の2列で表示する
// 左がインベントリ、右が倉庫
// クリックしたら右か左に移動する。確認は行わない。
namespace NSHud
{
class ISprite
{
public:
    virtual void DrawImage(const int percent,
                           const int x,
                           const int y,
                           const int transparency = 255) = 0;

    virtual void Load(const std::wstring& filepath) = 0;
    virtual ~ISprite() {};
};

class IFont
{
public:
    virtual void DrawText_(const std::wstring& msg, const int x, const int y) = 0;
    virtual void Init(const bool bEnglish) = 0;
    virtual ~IFont() {};
};

class StatusItem
{
public:
    void SetName(const std::wstring& arg);
    std::wstring GetName() const;

    // 0 ~ 100の101段階
    void SetPercent(const int arg);
    int GetPercent() const;

    void SetPercentSub(const int arg);
    int GetPercentSub() const;

    void SetBarVisible(const bool arg);
    bool GetBarVisible() const;

private:

    std::wstring m_name;
    int m_percent = 0;
    int m_percentSub = 0;
    bool m_visible = false;
};

class hud
{
public:

    void Init(IFont* font, ISprite* sprBack, ISprite* sprMiddle, ISprite* sprFront, const bool bEnglish);
    void Finalize();

    void UpsertStatus(const std::wstring& name,
                      const int percent,
                      const int percentSub,
                      const bool visible);

    void RemoveStatus(const std::wstring& name);

    void Draw();
    
private:

    ISprite* m_sprBack;
    ISprite* m_sprMiddle;
    ISprite* m_sprFront;
    IFont* m_font;

    std::vector<StatusItem> m_statusList;

    const int STARTX = 1300;
    const int STARTY = 400;

    const int INTERVAL = 50;
    const int PADDING = 25;

    bool m_bEnglish = false;
};
}


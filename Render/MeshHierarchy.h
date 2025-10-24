#pragma once

#include "Common.h"

namespace NSRender
{

// MeshMixの階層構造に対応したバージョン
// 一つのXファイルに複数のモデルを含ませることができる。
// 木の枝と木の葉っぱは異なる質感で描画されるべき。
// かといって別々のXファイルで扱うのは管理が難しい。
// これに対応するためのメッシュクラス
class MeshHierarchy : public IDeviceResettable
{
};

}


#pragma once

#include "Common.h"

namespace NSRender
{

// D3DXEFFECTファイルはメッシュ一つごとに作成するべきではない。
// 複数のメッシュで一つのD3DXEFFECTファイルを共有するためにこのクラスを定義して使う
class MeshMixManager
{

public:

    // SetTexture関数は初回しかGPUへのロードが行われず
    // 2回目はキャッシュされた内容を使うらしい


};

}


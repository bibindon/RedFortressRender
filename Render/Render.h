
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>

namespace NSRender
{
class Render
{
public:

    void Initialize(HWND hWnd);
    void Finalize();
    void Draw();

private:

};
}


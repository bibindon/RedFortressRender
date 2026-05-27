#pragma comment(lib, "comctl32.lib")


#include "RenderSettingsDialogInternal.h"


namespace NSRender

{

namespace RenderSettingsDialogInternal

{

LRESULT CALLBACK RenderSettingsDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)

{

    switch (msg)

    {

    case WM_NCCREATE:

    {

        const CREATESTRUCT* createStruct = reinterpret_cast<const CREATESTRUCT*>(lParam);

        RenderSettingsDialogState* state = new RenderSettingsDialogState;

        state->render = reinterpret_cast<Render*>(createStruct->lpCreateParams);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

        return TRUE;

    }

    case WM_CREATE:

    {

        RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        InitializeRenderSettingsControls(hWnd, state);

        SyncRenderSettingsDialogFromRender(hWnd);

        CaptureRenderSettingsChildPlacements(hWnd);

        UpdateRenderSettingsScrollBar(hWnd);

        SetTimer(hWnd, RENDER_SETTINGS_SYNC_TIMER_ID, RENDER_SETTINGS_SYNC_INTERVAL_MS, NULL);

        return 0;

    }

    case WM_TIMER:

    {

        if (wParam == RENDER_SETTINGS_SYNC_TIMER_ID && IsWindowVisible(hWnd))

        {

            SyncRenderSettingsDialogFromRender(hWnd);

            return 0;

        }

        break;

    }

    case WM_SIZE:

    {

        ApplyRenderSettingsChildPositions(hWnd);

        UpdateRenderSettingsScrollBar(hWnd);

        return 0;

    }

    case WM_COMMAND:

    {

        HandleRenderSettingsCommand(hWnd, wParam);

        return 0;

    }

    case WM_NOTIFY:

    {

        HandleRenderSettingsNotify(hWnd, lParam);

        return 0;

    }

    case WM_VSCROLL:

    {

        HandleRenderSettingsVScroll(hWnd, wParam);

        return 0;

    }

    case WM_HSCROLL:

    {

        HandleRenderSettingsHScroll(hWnd, lParam);

        return 0;

    }

    case WM_MOUSEWHEEL:

    {

        const short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

        RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        if (state != nullptr)

        {

            ScrollRenderSettingsTo(hWnd, state->scrollPos - ((wheelDelta / WHEEL_DELTA) * 72));

        }

        return 0;

    }

    case WM_CLOSE:

    {

        ShowWindow(hWnd, SW_HIDE);

        return 0;

    }

    case WM_NCDESTROY:

    {

        KillTimer(hWnd, RENDER_SETTINGS_SYNC_TIMER_ID);

        RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        delete state;

        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);

        break;

    }

    }


    return DefWindowProc(hWnd, msg, wParam, lParam);

}


bool EnsureRenderSettingsDialogClass(HINSTANCE hInstance)

{

    WNDCLASSEXW existingClass { };

    existingClass.cbSize = sizeof(existingClass);

    if (GetClassInfoExW(hInstance, RENDER_SETTINGS_DIALOG_CLASS_NAME, &existingClass))

    {

        return true;

    }


    WNDCLASSEXW wc { };

    wc.cbSize = sizeof(wc);

    wc.style = CS_DBLCLKS;

    wc.lpfnWndProc = RenderSettingsDialogProc;

    wc.hInstance = hInstance;

    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);

    wc.lpszClassName = RENDER_SETTINGS_DIALOG_CLASS_NAME;


    return RegisterClassExW(&wc) != 0;

}


void MoveWindowNearParent(HWND hWnd, HWND parent)

{

    if (parent == NULL)

    {

        return;

    }


    RECT parentRect { };

    RECT windowRect { };

    if (!GetWindowRect(parent, &parentRect) || !GetWindowRect(hWnd, &windowRect))

    {

        return;

    }


    int windowW = windowRect.right - windowRect.left;

    int windowH = windowRect.bottom - windowRect.top;

    int gap = 8;


    RECT workArea { };

    HMONITOR monitor = MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST);

    MONITORINFO monitorInfo { };

    monitorInfo.cbSize = sizeof(monitorInfo);

    if (GetMonitorInfo(monitor, &monitorInfo))

    {

        workArea = monitorInfo.rcWork;

    }

    else

    {

        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    }


    int x = parentRect.right + gap;

    int y = parentRect.top;


    if (x + windowW > workArea.right)

    {

        x = parentRect.left + gap;

    }


    if (y + windowH > workArea.bottom)

    {

        y = workArea.bottom - windowH;

    }


    x = (std::max)(static_cast<int>(workArea.left),

                   (std::min)(x, static_cast<int>(workArea.right) - windowW));

    y = (std::max)(static_cast<int>(workArea.top),

                   (std::min)(y, static_cast<int>(workArea.bottom) - windowH));


    SetWindowPos(hWnd,
                 NULL,
                 x,
                 y,
                 windowW,
                 windowH,
                 SWP_NOACTIVATE | SWP_NOZORDER);
}
} // namespace RenderSettingsDialogInternal


void RenderSettingsDialog::Show(HWND parent, Render* render, bool activateDialog)

{

    if (parent == NULL || render == nullptr)

    {

        return;

    }


    if (m_hWnd != NULL && !IsWindow(m_hWnd))

    {

        m_hWnd = NULL;

    }


    if (m_hWnd == NULL)

    {

        HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL));

        if (!RenderSettingsDialogInternal::EnsureRenderSettingsDialogClass(hInstance))

        {

            return;

        }


        RECT rect { 0, 0, 520, 780 };

        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL;

        const DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME;

        AdjustWindowRectEx(&rect, style, FALSE, exStyle);


        m_hWnd = CreateWindowExW(exStyle,

                                 RenderSettingsDialogInternal::RENDER_SETTINGS_DIALOG_CLASS_NAME,

                                 L"Settings",

                                 style,

                                 CW_USEDEFAULT,

                                 CW_USEDEFAULT,

                                 rect.right - rect.left,

                                 rect.bottom - rect.top,

                                 parent,

                                 NULL,

                                 hInstance,

                                 render);

        if (m_hWnd == NULL)

        {

            return;

        }


        RenderSettingsDialogInternal::MoveWindowNearParent(m_hWnd, parent);

    }


    RenderSettingsDialogInternal::SyncRenderSettingsDialogFromRender(m_hWnd);

    ShowWindow(m_hWnd, SW_SHOWNORMAL);

    if (activateDialog)

    {

        SetForegroundWindow(m_hWnd);

        SetFocus(m_hWnd);

    }

    else

    {

        SetForegroundWindow(parent);

        SetFocus(parent);

    }

}


void RenderSettingsDialog::Toggle(HWND parent, Render* render)

{

    if (m_hWnd != NULL && IsWindow(m_hWnd) && IsWindowVisible(m_hWnd))

    {

        ShowWindow(m_hWnd, SW_HIDE);

        SetForegroundWindow(parent);

        SetFocus(parent);

        return;

    }


    Show(parent, render, true);

}


void RenderSettingsDialog::Finalize()

{

    if (m_hWnd != NULL)

    {

        DestroyWindow(m_hWnd);

        m_hWnd = NULL;

    }

}


}


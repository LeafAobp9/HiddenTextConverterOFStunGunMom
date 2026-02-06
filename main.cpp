#include <windows.h>
#include <string>
#include <vector>

using namespace std;

// ================= 零宽字符 =================
#define ZW_ZERO  L'\u200B'
#define ZW_ONE   L'\u200C'
#define ZW_SEP   L'\u200D'
#define ZW_START L'\uFEFF'
#define ZW_END   L'\u2060'

// ================= Base64 =================
static const char* BASE64_TABLE =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

string Base64Encode(const string& in) {
    string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(BASE64_TABLE[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        out.push_back(BASE64_TABLE[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4)
        out.push_back('=');
    return out;
}

string Base64Decode(const string& in) {
    vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[BASE64_TABLE[i]] = i;

    string out;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// ================= 剪贴板 =================
void CopyToClipboard(const wstring& text) {
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();

    size_t size = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem) {
        CloseClipboard();
        return;
    }

    void* ptr = GlobalLock(hMem);
    memcpy(ptr, text.c_str(), size);
    GlobalUnlock(hMem);

    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
}

// ================= 隐写算法 =================
wstring HideText(const string& text) {
    if (text.empty()) return L"";

    string base64 = Base64Encode(text);
    wstring out;
    out.push_back(ZW_START);

    int bits = 0;
    for (unsigned char c : base64) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((c >> i & 1) ? ZW_ONE : ZW_ZERO);
            bits++;
            if (bits % 8 == 0)
                out.push_back(ZW_SEP);
        }
    }
    out.push_back(ZW_END);
    return out;
}

string RevealText(const wstring& hidden) {
    string binary;
    for (wchar_t c : hidden) {
        if (c == ZW_ZERO) binary += '0';
        else if (c == ZW_ONE) binary += '1';
    }

    string base64;
    for (size_t i = 0; i + 7 < binary.size(); i += 8) {
        base64.push_back((char)stoi(binary.substr(i, 8), nullptr, 2));
    }
    return Base64Decode(base64);
}

// ================= WinAPI UI =================
HWND hInput, hOutput;

#define ID_HIDE   1001
#define ID_REVEAL 1002

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL,
            10, 10, 460, 120,
            hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"隐藏文本（自动复制）",
            WS_CHILD | WS_VISIBLE,
            10, 140, 220, 35,
            hwnd, (HMENU)ID_HIDE, NULL, NULL);

        CreateWindowW(L"BUTTON", L"提取文本（自动复制）",
            WS_CHILD | WS_VISIBLE,
            250, 140, 220, 35,
            hwnd, (HMENU)ID_REVEAL, NULL, NULL);

        hOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY,
            10, 185, 460, 140,
            hwnd, NULL, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_HIDE) {
            int len = GetWindowTextLengthW(hInput);
            wstring wbuf(len + 1, 0);
            GetWindowTextW(hInput, &wbuf[0], len + 1);

            int size = WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, NULL, 0, NULL, NULL);
            string text(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, &text[0], size, NULL, NULL);

            wstring hidden = HideText(text);
            SetWindowTextW(hOutput, hidden.c_str());
            CopyToClipboard(hidden);
        }

        if (LOWORD(wParam) == ID_REVEAL) {
            int len = GetWindowTextLengthW(hInput);
            wstring hidden(len + 1, 0);
            GetWindowTextW(hInput, &hidden[0], len + 1);

            string revealed = RevealText(hidden);

            int wlen = MultiByteToWideChar(CP_UTF8, 0, revealed.c_str(), -1, NULL, 0);
            wstring wout(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, revealed.c_str(), -1, &wout[0], wlen);

            SetWindowTextW(hOutput, wout.c_str());
            CopyToClipboard(wout);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ================= 入口 =================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"ZeroWidthStego";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"滚木生成器 - WinAPI（自动复制）",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 380,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

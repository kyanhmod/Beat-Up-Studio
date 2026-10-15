#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#define wWinMain ProductEntry
#include "../src/main.cpp"
#undef wWinMain
static HWND FindProductWindow() {
    HWND result=nullptr;
    EnumWindows([](HWND h,LPARAM p)->BOOL {
        DWORD pid=0;GetWindowThreadProcessId(h,&pid);wchar_t name[128]{};GetClassNameW(h,name,128);
        if(pid==GetCurrentProcessId() && std::wstring(name)==L"AuditionBeatUpSimulatorClass") {*reinterpret_cast<HWND*>(p)=h;return FALSE;}return TRUE;
    },reinterpret_cast<LPARAM>(&result));return result;
}
int main() {
    std::atomic<bool> ok{true};DWORD mainThread=GetCurrentThreadId();
    std::thread check([&]{
        HWND h=nullptr;for(int i=0;i<100 && !h;++i){Sleep(50);h=FindProductWindow();}
        if(!h){ok=false;PostThreadMessageW(mainThread,WM_QUIT,0,0);return;}
        Sleep(1500);
        using GetContext=HANDLE(WINAPI*)(HWND);using Equal=BOOL(WINAPI*)(HANDLE,HANDLE);
        auto get=reinterpret_cast<GetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetWindowDpiAwarenessContext"));
        auto equal=reinterpret_cast<Equal>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"AreDpiAwarenessContextsEqual"));
        if(get && equal && equal(get(h),reinterpret_cast<HANDLE>(-5))) {ok=false;std::cerr<<"FAIL: GDI-scaled DPI mode returned\n";}
        RECT r{},work=WorkArea(h);GetWindowRect(h,&r);
        if(r.left<work.left || r.top<work.top || r.right>work.right || r.bottom>work.bottom){ok=false;std::cerr<<"FAIL: startup exceeds work area\n";}
        auto inspect=[&]{
            RECT client{};GetClientRect(h,&client);int surfaces=0,bars=0;
            struct Bounds {HWND root;RECT client;int* surfaces;int* bars;bool fits=true;} b{h,client,&surfaces,&bars};
            EnumChildWindows(h,[](HWND child,LPARAM p)->BOOL{
                auto& b=*reinterpret_cast<Bounds*>(p);wchar_t name[128]{};GetClassNameW(child,name,128);std::wstring type=name;
                if(type!=L"BeatUpRenderSurface" && type!=L"BeatUpPlaybackPanel")return TRUE;
                RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,b.root,reinterpret_cast<POINT*>(&r),2);
                if(r.left<0 || r.top<0 || r.right>b.client.right || r.bottom>b.client.bottom)b.fits=false;
                if(type==L"BeatUpRenderSurface"){++*b.surfaces;if(r.right-r.left!=b.client.right || r.bottom-r.top!=b.client.bottom)b.fits=false;}
                else ++*b.bars;return TRUE;
            },reinterpret_cast<LPARAM>(&b));
            if(!b.fits || surfaces!=1 || bars!=1) {ok=false;std::cerr<<"FAIL: native child bounds\n";}
        };
        inspect();SetWindowPos(h,nullptr,0,0,640,527,SWP_NOMOVE|SWP_NOZORDER);Sleep(250);inspect();
        PostMessageW(h,WM_CLOSE,0,0);
    });
    int code=ProductEntry(GetModuleHandleW(nullptr),nullptr,const_cast<LPWSTR>(L""),SW_SHOWNORMAL);
    check.join();if(!ok || code)return 1;
    std::cout<<"PASS: actual product startup, default DPI mode, work-area fit, native child bounds and resize\n";
}

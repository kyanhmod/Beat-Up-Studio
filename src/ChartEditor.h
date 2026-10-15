#pragma once
#include <windows.h>
#include <functional>
#include "ChartDocument.h"
#include "SongInfo.h"
#include "SlkParser.h"
namespace BeatUp {
class ChartEditor {
public:
    void Show(HWND owner,const std::wstring& chart,std::function<bool(const std::wstring&,double)> preview,std::function<void()> mixer={},const std::wstring& sessionAudio=L"",std::function<void(int)> transport={},double sessionBpm=0,double sessionOffset=1);
    void SetSavedCallback(std::function<void(const std::wstring&)> callback) {savedCallback_=std::move(callback);}
    int AllCombo() const;
    void ResetSession() {loaded_=false;source_.clear();output_.clear();document_.New();metadataDirty_=false;}
    std::vector<ChartNote> Snapshot() const;
    const SongInfo& Info() const {return info_;}
    void SetLivePreview(std::function<bool(const std::vector<ChartNote>&,const SongInfo&,double)> callback) {livePreview_=std::move(callback);}
    void SetPlaybackTime(double ms,bool playing);
    bool Visible() const { return window_ && IsWindowVisible(window_); }
    HWND Handle() const {return window_;}
    void SetEmbedded(bool enabled) {embedded_=enabled;}
    void Layout();
    void SetBounds(RECT bounds) {bounds_=bounds;Layout();}
    void SetTiming(double bpm,double offset);
    bool HideForPlayback();
    bool CanClose() { return ConfirmClose(); }
    bool Accelerator(const MSG& message);
private:
    HWND window_=nullptr,owner_=nullptr;
    HWND bpmEdit_=nullptr,offsetEdit_=nullptr;
    ChartDocument document_;
    SongInfo info_;
    std::wstring source_,output_;
    std::function<bool(const std::wstring&,double)> preview_;
    std::function<void(int)> transport_;
    double playhead_=-1;
    bool previewPlaying_=false;
    DWORD playbackPaint_=0;
    std::function<void()> mixer_;
    int top_=0,cursor_=0,anchor_=0,step_=1;
    bool selecting_=false,dragged_=false,extendDrag_=false;
    POINT dragStart_{};
    int dragTick_=0,dragLane_=0;
    int LaneHeight() const;
    RECT bounds_{};
    bool embedded_=false;
    HMENU editorMenu_=nullptr,ownerMenu_=nullptr;
    bool loaded_=false;
    std::function<void(const std::wstring&)> savedCallback_;
    std::function<bool(const std::vector<ChartNote>&,const SongInfo&,double)> livePreview_;
    bool outputOwned_=false;
    bool metadataDirty_=false;
    bool syncing_=false;
    int zoom_=32;
    std::wstring status_;
    static LRESULT CALLBACK Proc(HWND,UINT,WPARAM,LPARAM);
    LRESULT Message(UINT,WPARAM,LPARAM);
    void Paint(HDC dc);
    bool Open(const std::wstring& path);
    bool Save();
    bool ConfirmClose();
    void Refresh();
    void Navigate(int tick,bool extend);
    int VisibleRows() const;
    void Command(int command);
    void SyncTiming();
    bool ApplyTiming();
};
}

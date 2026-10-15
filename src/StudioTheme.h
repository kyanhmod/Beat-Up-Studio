#pragma once
#include <windows.h>
namespace BeatUp::Theme {
inline COLORREF Background() {return RGB(18,21,28);}
inline HBRUSH Brush() {static HBRUSH b=CreateSolidBrush(Background());return b;}
inline HFONT Font() {static HFONT f=CreateFontW(-15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");return f;}
inline void Apply(HWND parent) {
    for(HWND child=GetWindow(parent,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
        SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(Font()),TRUE);
        wchar_t name[32]{};GetClassNameW(child,name,32);
        if(lstrcmpiW(name,L"BUTTON")==0) SetWindowLongPtrW(child,GWL_STYLE,(GetWindowLongPtrW(child,GWL_STYLE)&~BS_TYPEMASK)|BS_OWNERDRAW);
    }
}
inline LRESULT Color(WPARAM w) {auto dc=reinterpret_cast<HDC>(w);SetTextColor(dc,RGB(222,230,242));SetBkColor(dc,Background());return reinterpret_cast<LRESULT>(Brush());}
inline bool Draw(LPARAM l,int active=0) {
    auto item=reinterpret_cast<DRAWITEMSTRUCT*>(l);if(item->CtlType!=ODT_BUTTON) return false;
    bool selected=item->CtlID==active,down=item->itemState&ODS_SELECTED,disabled=item->itemState&ODS_DISABLED;
    if(item->CtlID==1010 || item->CtlID==1005 || item->CtlID==1015) {
        auto dc=item->hDC;RECT rect=item->rcItem;FillRect(dc,&rect,Brush());
        int cx=(rect.left+rect.right)/2,cy=(rect.top+rect.bottom)/2;
        COLORREF ink=disabled?RGB(86,103,120):RGB(86,192,244);
        auto pen=CreatePen(PS_SOLID,1,ink);auto fill=CreateSolidBrush(down?RGB(35,74,100):RGB(27,41,57));
        auto oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,fill);Ellipse(dc,rect.left+1,rect.top+1,rect.right-1,rect.bottom-1);
        auto icon=CreateSolidBrush(ink);SelectObject(dc,icon);
        if(item->CtlID==1010) {
            wchar_t text[40]{};GetWindowTextW(item->hwndItem,text,40);
            if(lstrcmpW(text,L"Pause")==0) {Rectangle(dc,cx-6,cy-7,cx-2,cy+7);Rectangle(dc,cx+2,cy-7,cx+6,cy+7);}
            else {POINT points[]={{cx-4,cy-8},{cx+8,cy},{cx-4,cy+8}};Polygon(dc,points,3);}
        } else if(item->CtlID==1005) {
            SelectObject(dc,GetStockObject(HOLLOW_BRUSH));Arc(dc,cx-8,cy-8,cx+8,cy+8,cx-7,cy-3,cx-3,cy-7);SelectObject(dc,icon);
            POINT head[]={{cx-9,cy-7},{cx-9,cy+1},{cx-2,cy-3}};Polygon(dc,head,3);
        } else {
            for(int j=0;j<3;++j) {int y=cy-7+j*7;MoveToEx(dc,cx-8,y,nullptr);LineTo(dc,cx+9,y);int x=cx+(j==1?4:-3);Rectangle(dc,x-2,y-3,x+2,y+3);}
        }
        SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(icon);DeleteObject(fill);DeleteObject(pen);
        if(item->itemState&ODS_FOCUS) {InflateRect(&rect,-4,-4);DrawFocusRect(dc,&rect);}return true;
    }
    COLORREF color=selected?RGB(38,112,165):down?RGB(48,64,84):RGB(34,41,54);
    auto brush=CreateSolidBrush(color);FillRect(item->hDC,&item->rcItem,brush);DeleteObject(brush);
    SetBkMode(item->hDC,TRANSPARENT);SetTextColor(item->hDC,disabled?RGB(104,115,131):RGB(232,239,250));
    auto old=SelectObject(item->hDC,Font());wchar_t text[160]{};GetWindowTextW(item->hwndItem,text,160);
    DrawTextW(item->hDC,text,-1,&item->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    if(item->itemState&ODS_FOCUS) {RECT r=item->rcItem;InflateRect(&r,-3,-3);DrawFocusRect(item->hDC,&r);}
    SelectObject(item->hDC,old);return true;
}
}

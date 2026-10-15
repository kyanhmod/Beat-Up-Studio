#include "ChartDocument.h"
#include "SlkParser.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <climits>
#include <random>
#include <set>

namespace BeatUp {
namespace {
std::vector<std::string> Fields(const std::string& s) {
    std::vector<std::string> out;
    std::string field;
    bool quote=false;
    for(size_t i=0;i<=s.size();++i) {
        char c=i==s.size()?';':s[i];
        if(c=='"') {
            if(quote && i+1<s.size() && s[i+1]=='"') { field+="\"\""; ++i; continue; }
            quote=!quote;
        }
        if(c==';' && !quote) { out.push_back(field); field.clear(); } else field+=c;
    }
    if(quote) throw std::runtime_error("Unclosed quote");
    return out;
}
int Int(const std::string& s) {
    size_t used=0; int n=std::stoi(s,&used);
    if(used!=s.size()) throw std::runtime_error("Invalid integer");
    return n;
}
std::string Kind(bool n,bool s,bool f) {
    std::string k;
    for(auto p : {std::pair<bool,char>{n,'n'},{s,'s'},{f,'f'}})
        if(p.first) { if(!k.empty()) k+=','; k+=p.second; }
    return k;
}
}
std::string ChartDocument::Value(const Row& row,int col) {
    auto it=row.find(col); if(it==row.end()) return {};
    std::string result;
    for(const auto& f:Fields(it->second)) if(!f.empty() && f[0]=='K') {
        auto s=f.substr(1);
        if(s.size()>=2 && s.front()=='"' && s.back()=='"') {
            s=s.substr(1,s.size()-2);
            for(size_t i=0;(i=s.find("\"\"",i))!=std::string::npos;++i) s.erase(i,1);
        }
        result=s;
    }
    return result;
}
void ChartDocument::Put(Row& row,int col,const std::string& raw) {
    auto fields=Fields(row[col]); bool found=false;
    std::string result;
    for(auto& f:fields) {
        if(f.empty()) continue;
        if(f[0]=='K') { f="K"+raw; found=true; }
        if(!result.empty()) result+=';'; result+=f;
    }
    if(!found) { if(!result.empty()) result+=';'; result+='K'+raw; }
    row[col]=result;
}
int ChartDocument::Tick(const Row& row) {
    try { return Int(Value(row,4)); } catch(...) { return -1; }
}
void ChartDocument::Position(Row& row,int tick) {
    Put(row,1,std::to_string(tick/16));
    Put(row,2,std::to_string(tick/4%4));
    Put(row,3,std::to_string(tick%4));
    Put(row,4,std::to_string(tick));
}
void ChartDocument::New() {
    rows_.clear();saved_.clear();undo_.clear();redo_.clear();clipboard_.clear();records_={"ID;PBeatUpStudio"};
    path_.clear();original_.clear();originalRows_.clear();
    const char* names[]={"Measure","Beat","Sub","Location","Type","Key"};
    for(int col=1;col<=6;++col) {Put(rows_[1],col,std::string("\"")+names[col-1]+"\"");Put(rows_[2],col,col==5?"\"enum(n,s,f)\"":col==6?"\"string\"":"\"int\"");}
    saved_=rows_;
}
bool ChartDocument::Load(const std::wstring& path) {
    std::vector<ChartNote> validation;
    if(!SlkParser::LoadChart(path,validation,150,true) || (!validation.empty() && validation.front().tick<0)) return false;
    std::ifstream file(std::filesystem::path(path),std::ios::binary);
    if(!file) return false;
    std::string bytes((std::istreambuf_iterator<char>(file)),{});
    Table table; std::vector<std::string> records;
    int x=1,y=1;
    try {
        std::istringstream stream(bytes); std::string line;
        while(std::getline(stream,line)) {
            if(!line.empty() && line.back()=='\r') line.pop_back();
            if(line.rfind("C;",0)!=0) { records.push_back(line); continue; }
            auto fields=Fields(line.substr(2)); std::string rest;
            for(const auto& f:fields) {
                if(f.size()>1 && f[0]=='X') x=Int(f.substr(1));
                else if(f.size()>1 && f[0]=='Y') y=Int(f.substr(1));
                else { if(!rest.empty()) rest+=';'; rest+=f; }
            }
            if(x<1 || y<1) return false;
            if(!rest.empty()) {
                // Multiple records can contribute value/style fields to one cell.
                if(!table[y][x].empty()) table[y][x]+=';';
                table[y][x]+=rest;
            }
        }
    } catch(...) { return false; }
    if(Value(table[1],6)!="Key") return false;
    rows_=std::move(table); saved_=originalRows_=rows_; records_=std::move(records);
    original_=std::move(bytes); path_=path; undo_.clear(); redo_.clear(); clipboard_.clear();
    return true;
}
bool ChartDocument::Save(const std::wstring& path) {
    if(path.empty() || rows_.empty()) return false;
    std::ostringstream out;
    if(!original_.empty() && rows_==originalRows_) out<<original_;
    else {
        int maxX=6,maxY=2;
        for(const auto& [y,row]:rows_) { maxY=(std::max)(maxY,y); if(!row.empty()) maxX=(std::max)(maxX,row.rbegin()->first); }
        for(const auto& r:records_)
            if(r!="E" && r.rfind("B;",0)!=0) out<<r<<"\r\n";
        out<<"B;Y"<<maxY<<";X"<<maxX<<";D0 0 "<<maxY-1<<' '<<maxX-1<<"\r\n";
        std::vector<const Row*> ordered;
        for(const auto& [y,row]:rows_) if(y>=3) ordered.push_back(&row);
        std::stable_sort(ordered.begin(),ordered.end(),[](const Row* a,const Row* b){return Tick(*a)<Tick(*b);});
        for(int y=1;y<=2;++y) {
            auto it=rows_.find(y); if(it!=rows_.end()) for(const auto& [x,cell]:it->second)
                out<<"C;Y"<<y<<";X"<<x<<';'<<cell<<"\r\n";
        }
        int y=3;
        for(const auto* row:ordered) {
            for(const auto& [x,cell]:*row) out<<"C;Y"<<y<<";X"<<x<<';'<<cell<<"\r\n";
            ++y;
        }
        out<<"E\r\n";
    }
    auto temp=std::filesystem::path(path); temp+=L".tmp";
    { std::ofstream f(temp,std::ios::binary|std::ios::trunc); if(!f) return false;
      auto text=out.str(); f.write(text.data(),text.size()); f.flush(); if(!f) return false; }
    if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str()); return false;
    }
    saved_=rows_; path_=path; return true;
}
void ChartDocument::Checkpoint() {
    undo_.push_back(rows_); if(undo_.size()>100) undo_.erase(undo_.begin()); redo_.clear();
}
bool ChartDocument::Dirty() const { return rows_!=saved_; }
std::vector<EditorEvent> ChartDocument::Events() const {
    std::vector<EditorEvent> out;
    for(const auto& [y,row]:rows_) {
        int tick=Tick(row); if(y<3 || tick<0) continue;
        auto k=Value(row,5); bool finish=k.find('f')!=std::string::npos;
        if(k.find('n')!=std::string::npos) {
            try {out.push_back({tick,Int(Value(row,6)),finish});} catch(...) {}
        }
        if(k.find('s')!=std::string::npos) out.push_back({tick,5,finish});
        if(k=="f") out.push_back({tick,0,true});
    }
    return out;
}
int ChartDocument::LastTick() const {
    int last=0; for(const auto& e:Events()) last=(std::max)(last,e.tick); return last;
}
bool ChartDocument::Toggle(int tick,int lane) {
    if(lane==0) return ToggleFinish(tick);
    if(tick<0 || tick>INT_MAX-16 || (lane!=7 && lane!=4 && lane!=1 && lane!=9 && lane!=6 && lane!=3 && lane!=5)) return false;
    Checkpoint();
    bool finishAtTick=false;
    for(auto it=rows_.begin();it!=rows_.end();++it) {
        if(it->first<3 || Tick(it->second)!=tick) continue;
        auto& row=it->second; auto k=Value(row,5);
        bool n=k.find('n')!=std::string::npos,s=k.find('s')!=std::string::npos,f=k.find('f')!=std::string::npos;
        finishAtTick=finishAtTick || f;
        if((lane==5 && s) || (lane!=5 && n && Value(row,6)==std::to_string(lane))) {
            if(lane==5) s=false; else n=false;
            auto kind=Kind(n,s,f);
            if(kind.empty()) {
                // Keep unknown columns rather than discarding the row's metadata.
                for(int col=1;col<=6;++col) row.erase(col);
                if(row.empty()) rows_.erase(it);
            } else { Put(row,5,'"'+kind+'"'); if(!n) Put(row,6,"\"\""); }
            return true;
        }
    }
    int y=rows_.empty()?3:(std::max)(3,rows_.rbegin()->first+1);
    auto& row=rows_[y]; Position(row,tick);
    Put(row,5,lane==5?(finishAtTick?"\"s,f\"":"\"s\""):(finishAtTick?"\"n,f\"":"\"n\""));
    Put(row,6,lane==5?"\"\"":'"'+std::to_string(lane)+'"');
    return true;
}
bool ChartDocument::ToggleFinish(int tick) {
    if(tick<0 || tick>INT_MAX-16) return false;
    bool has=false;int target=-1;
    for(const auto& [id,row]:rows_) if(id>=3 && Tick(row)==tick) {
        if(target<0) target=id;
        if(Value(row,5).find('f')!=std::string::npos) has=true;
    }
    Checkpoint();
    if(target<0) {
        target=rows_.empty()?3:(std::max)(3,rows_.rbegin()->first+1);
        Position(rows_[target],tick);Put(rows_[target],6,"\"\"");
    }
    for(auto it=rows_.begin();it!=rows_.end();) {
        auto& row=it->second;
        if(it->first>=3 && Tick(row)==tick) {
            auto k=Value(row,5);
            auto kind=Kind(k.find('n')!=std::string::npos,k.find('s')!=std::string::npos,!has);
            if(kind.empty()) {
                for(int col=1;col<=6;++col) row.erase(col);
                if(row.empty()) {it=rows_.erase(it);continue;}
            } else Put(row,5,'"'+kind+'"');
        }
        ++it;
    }
    return true;
}
void ChartDocument::Mirror(int first,int last) {
    if(first>last) std::swap(first,last);Checkpoint();
    for(auto& [id,row]:rows_) if(id>=3 && Tick(row)>=first && Tick(row)<=last && Value(row,5).find('n')!=std::string::npos) {
        int lane=Int(Value(row,6)),mirror=lane==7?9:lane==9?7:lane==4?6:lane==6?4:lane==1?3:1;
        Put(row,6,'"'+std::to_string(mirror)+'"');
    }
}
bool ChartDocument::Shift(int first,int last,int delta) {
    if(first>last) std::swap(first,last);
    for(const auto& [id,row]:rows_) if(id>=3 && Tick(row)>=first && Tick(row)<=last)
        if(static_cast<long long>(Tick(row))+delta<0 || static_cast<long long>(Tick(row))+delta>INT_MAX-16) return false;
    Checkpoint();
    for(auto& [id,row]:rows_) if(id>=3 && Tick(row)>=first && Tick(row)<=last) Position(row,Tick(row)+delta);
    return true;
}
void ChartDocument::Randomize(int first,int last) {
    if(first>last) std::swap(first,last);Checkpoint();
    std::map<int,std::vector<Row*>> groups;
    for(auto& [id,row]:rows_) if(id>=3 && Tick(row)>=first && Tick(row)<=last && Value(row,5).find('n')!=std::string::npos) groups[Tick(row)].push_back(&row);
    std::mt19937 random(std::random_device{}());
    for(auto& [tick,group]:groups) {
        if(group.size()>6) continue;
        std::vector<int> lanes{7,4,1,9,6,3};std::shuffle(lanes.begin(),lanes.end(),random);
        for(size_t i=0;i<group.size();++i) Put(*group[i],6,'"'+std::to_string(lanes[i])+'"');
    }
}
void ChartDocument::GenerateSpaces(int first,int last,int interval) {
    if(first>last) std::swap(first,last);
    if(first<0 || interval<1 || last>INT_MAX-16 || (last-first)/interval>100000) return;
    Checkpoint();std::set<int> spaces,finishes;
    for(auto e:Events()) {if(e.lane==5) spaces.insert(e.tick);if(e.finish) finishes.insert(e.tick);}
    int id=rows_.empty()?3:(std::max)(3,rows_.rbegin()->first+1);
    for(int tick=first;tick<=last;tick+=interval) if(!spaces.count(tick)) {
        auto& row=rows_[id++];Position(row,tick);Put(row,5,finishes.count(tick)?"\"s,f\"":"\"s\"");Put(row,6,"\"\"");
    }
}
std::wstring ChartDocument::Validate() const {
    auto events=Events();size_t duplicates=0,finishes=0;
    std::set<std::pair<int,int>> seen;std::set<int> finishTicks;
    for(auto e:events) {if(e.finish) finishTicks.insert(e.tick);if(!seen.insert({e.tick,e.lane}).second) ++duplicates;}
    finishes=finishTicks.size();
    return L"Events: "+std::to_wstring(events.size())+L"\nFinish markers: "+std::to_wstring(finishes)+
        L"\nDuplicate tick/lane events: "+std::to_wstring(duplicates)+
        (finishes?L"":L"\nWarning: this chart has no Finish marker.");
}
void ChartDocument::Erase(int first,int last) {
    if(first>last) std::swap(first,last); Checkpoint();
    for(auto it=rows_.begin();it!=rows_.end();) {
        int tick=Tick(it->second);
        if(it->first>=3 && tick>=first && tick<=last) {
            for(int col=1;col<=6;++col) it->second.erase(col);
            if(it->second.empty()) { it=rows_.erase(it); continue; }
        }
        ++it;
    }
}
void ChartDocument::Copy(int first,int last) {
    if(first>last) std::swap(first,last); clipboard_.clear();
    for(const auto& [y,row]:rows_) if(y>=3 && Tick(row)>=first && Tick(row)<=last) {
        auto copy=row; Position(copy,Tick(row)-first); clipboard_.push_back(std::move(copy));
    }
}
void ChartDocument::Cut(int first,int last) { Copy(first,last); Erase(first,last); }
void ChartDocument::Paste(int tick) {
    if(clipboard_.empty() || tick<0) return;
    for(const auto& r:clipboard_) if(Tick(r)>INT_MAX-16-tick) return;
    Checkpoint();
    int y=rows_.empty()?3:(std::max)(3,rows_.rbegin()->first+1);
    for(auto row:clipboard_) {
        Position(row,Tick(row)+tick);
        bool duplicate=false;
        for(const auto& [id,existing]:rows_)
            if(id>=3 && Tick(existing)==Tick(row) && Value(existing,5)==Value(row,5) && Value(existing,6)==Value(row,6)) {duplicate=true;break;}
        if(!duplicate) rows_[y++]=std::move(row);
    }
}
bool ChartDocument::Undo() {
    if(undo_.empty()) return false; redo_.push_back(rows_); rows_=std::move(undo_.back()); undo_.pop_back(); return true;
}
bool ChartDocument::Redo() {
    if(redo_.empty()) return false; undo_.push_back(rows_); rows_=std::move(redo_.back()); redo_.pop_back(); return true;
}
}

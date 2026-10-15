#pragma once
#include <map>
#include <string>
#include <vector>

namespace BeatUp {
struct EditorEvent { int tick=0, lane=0; bool finish=false; };
class ChartDocument {
public:
    void New();
    bool Load(const std::wstring& path);
    bool Save(const std::wstring& path);
    bool Toggle(int tick,int lane);
    bool ToggleFinish(int tick);
    void Mirror(int first,int last);
    bool Shift(int first,int last,int delta);
    void Randomize(int first,int last);
    void GenerateSpaces(int first,int last,int interval=16);
    std::wstring Validate() const;
    void Erase(int first,int last);
    void Copy(int first,int last);
    void Cut(int first,int last);
    void Paste(int tick);
    bool Undo();
    bool Redo();
    bool Dirty() const;
    int LastTick() const;
    std::vector<EditorEvent> Events() const;
    const std::wstring& Path() const { return path_; }
private:
    // Cell fields retain original K quoting/bytes and other fields (e.g. style).
    using Row=std::map<int,std::string>;
    using Table=std::map<int,Row>;
    Table rows_, saved_;
    std::vector<Table> undo_,redo_;
    std::vector<Row> clipboard_;
    std::vector<std::string> records_;
    std::wstring path_;
    std::string original_;
    Table originalRows_;
    static std::string Value(const Row& row,int col);
    static void Put(Row& row,int col,const std::string& raw);
    static int Tick(const Row& row);
    static void Position(Row& row,int tick);
    void Checkpoint();
};
}

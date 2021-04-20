#define NO_RTTI
#ifndef TEXT_EDITOR_ACTIONS_H
#define TEXT_EDITOR_ACTIONS_H

#include <cstddef>

class TextEditor;

struct Cursor {
    size_t x_ = 0;
    size_t y_ = 0;
    bool operator==(const Cursor&);
    bool operator!=(const Cursor&);
};

struct IActDescriptor {
    const char* UniqueAddr;
    bool (*kDo)(char*, TextEditor*);
    bool (*kUndo)(char*, TextEditor*);
    void (*Destroyer)(char*);
};

struct IAction {
    IActDescriptor Descriptor;
    char OperStorage[256 - sizeof(IActDescriptor)];
    template <typename TBaser>
    explicit IAction(TBaser r) {
        static_assert(sizeof(TBaser) <= 256 - sizeof(IActDescriptor), "too large Base");
        static const char kUniqueVar = '\0';
        Descriptor.UniqueAddr = &kUniqueVar;
        Descriptor.kDo = [](char* f, TextEditor* text) -> bool { return (reinterpret_cast<TBaser*>(f))->Do(text); };
        Descriptor.kUndo = [](char* f, TextEditor* text) { return (reinterpret_cast<TBaser*>(f))->Undo(text); };
        Descriptor.Destroyer = [](char* f) { (reinterpret_cast<TBaser*>(f))->~TBaser(); };
        new (OperStorage) TBaser(r);
    }
    ~IAction() {
        if (Descriptor.UniqueAddr) {
            Descriptor.Destroyer(OperStorage);
        }
    }
    void Do(TextEditor*);
    void Undo(TextEditor*);
};

class TypeAction {
    char symbol_;
    Cursor cur_;

public:
    explicit TypeAction(const char&);
    bool Do(TextEditor*);
    bool Undo(TextEditor*);
};

class DelAction {
    char symbol_;
    Cursor cur_;

public:
    bool Do(TextEditor*);
    bool Undo(TextEditor*);
};

class BackAction {
    char symbol_;
    Cursor cur_;

public:
    bool Do(TextEditor*);
    bool Undo(TextEditor*);
};

class NewLineAction {
    Cursor cur_;

public:
    bool Do(TextEditor*);
    bool Undo(TextEditor*);
};

#endif  // TEXT_EDITOR_ACTIONS_H
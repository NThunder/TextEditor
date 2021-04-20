#ifndef TEXT_EDITOR_TEXT_EDITOR_H
#define TEXT_EDITOR_TEXT_EDITOR_H

#include <stack>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <actions.h>

struct ActionHistory {
    std::stack<IAction*> for_undo;
    std::stack<IAction*> for_redo;
    ~ActionHistory();
};

class TextEditor {
    std::vector<std::string> text_;

public:
    ActionHistory storage_;
    Cursor cur_;
    TextEditor();
    void ShiftLeft();
    void ShiftRight();
    void ShiftUp();
    void ShiftDown();
    void Type(char);
    void Delete();
    void BackSpace();
    void PasteNewLine();
    char Delete(Cursor&);
    char BackSpace(Cursor&);
    void ReverseNewLine(Cursor&);
    void PasteNewLine(Cursor&);
    void Type(char, Cursor&);
    void Undo();
    void Redo();
    void Print(std::ostream& os) const;
    void CheckRedo();
};

#endif  // TEXT_EDITOR_TEXT_EDITOR_H
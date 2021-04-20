#include <text_editor.h>

bool Cursor::operator==(const Cursor& s) {
    return x_ == s.x_ && y_ == s.y_;
}

bool Cursor::operator!=(const Cursor& s) {
    return !(*this == s);
}

TypeAction::TypeAction(const char& symbol) : symbol_(symbol) {
}

bool TypeAction::Do(TextEditor* text) {
    text->Type(symbol_, cur_);
    return true;
}

bool DelAction::Do(TextEditor* text) {
    symbol_ = text->Delete(cur_);
    return symbol_ != '\0';
}

bool BackAction::Do(TextEditor* text) {
    symbol_ = text->BackSpace(cur_);
    return symbol_ != '\0';
}

bool NewLineAction::Do(TextEditor* text) {
    text->PasteNewLine(cur_);
    return true;
}

bool TypeAction::Undo(TextEditor* text) {
    symbol_ = text->BackSpace(cur_);
    return symbol_ != '\0';
}

bool DelAction::Undo(TextEditor* text) {
    text->Type(symbol_, cur_);
    text->ShiftLeft();
    cur_ = text->cur_;
    return true;
}

bool BackAction::Undo(TextEditor* text) {
    text->Type(symbol_, cur_);
    return true;
}

bool NewLineAction::Undo(TextEditor* text) {
    text->ReverseNewLine(cur_);
    return true;
}

void IAction::Do(TextEditor* text) {
    if (Descriptor.kDo(const_cast<char*>(OperStorage), text)) {
        text->storage_.for_undo.push(this);
    } else {
        delete this;
    }
}

void IAction::Undo(TextEditor* text) {
    if (Descriptor.kUndo(const_cast<char*>(OperStorage), text)) {
        text->storage_.for_redo.push(this);
    } else {
        delete this;
    }
}
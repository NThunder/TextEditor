#include "text_editor.h"

Cursor cur;

TextEditor::TextEditor() {
    if (text_.empty()) {
        text_.resize(1);
    }
}

ActionHistory::~ActionHistory() {
    while (!for_undo.empty()) {
        delete for_undo.top();
        for_undo.pop();
    }
    while (!for_redo.empty()) {
        delete for_redo.top();
        for_redo.pop();
    }
}

void TextEditor::ShiftLeft() {
    if (cur_.x_ != 0) {
        --cur_.x_;
    } else if (cur_.y_ > 0) {
        --cur_.y_;
        cur_.x_ = text_[cur_.y_].size();
    }
}

void TextEditor::ShiftRight() {
    if (cur_.y_ < text_.size() && cur_.x_ < text_[cur_.y_].size()) {
        ++cur_.x_;
    } else if (cur_.y_ < text_.size() - 1 && text_[cur_.y_].size() == cur_.x_) {
        ++cur_.y_;
        cur_.x_ = 0;
    }
}

void TextEditor::ShiftUp() {
    if (cur_.y_ != 0) {
        --cur_.y_;
        if (cur_.y_ < text_.size() && cur_.x_ > text_[cur_.y_].size()) {
            cur_.x_ = text_[cur_.y_].size();
        }
    }
}

void TextEditor::ShiftDown() {
    if (cur_.y_ < text_.size() - 1) {
        ++cur_.y_;
        if (cur_.y_ < text_.size() && cur_.x_ > (text_[cur_.y_].size())) {
            cur_.x_ = text_[cur_.y_].size();
        }
    }
}

void TextEditor::CheckRedo() {
    while (!storage_.for_redo.empty()) {
        delete storage_.for_redo.top();
        storage_.for_redo.pop();
    }
}

void TextEditor::Delete() {
    CheckRedo();
    IAction* a = new IAction(DelAction());
    a->Do(this);
}

void TextEditor::BackSpace() {
    CheckRedo();
    IAction* a = new IAction(BackAction());
    a->Do(this);
}

void TextEditor::PasteNewLine() {
    CheckRedo();
    IAction* a = new IAction(NewLineAction());
    a->Do(this);
}

void TextEditor::Type(char symbol) {
    CheckRedo();
    IAction* a = new IAction(TypeAction(symbol));
    a->Do(this);
}

char TextEditor::Delete(Cursor& cursor) {
    if (cursor != cur) {
        cur_ = cursor;
    }
    char symbol_ = '\0';
    if (cur_.x_ < text_[cur_.y_].size()) {
        symbol_ = text_[cur_.y_][cur_.x_];
        text_[cur_.y_].erase(cur_.x_, 1);
    } else if (cur_.y_ < text_.size() - 1) {
        symbol_ = '\n';
        text_[cur_.y_] += text_[cur_.y_ + 1];
        text_.erase(text_.begin() + cur_.y_ + 1);
    }
    cursor = cur_;
    return symbol_;
}

char TextEditor::BackSpace(Cursor& cursor) {
    if (cursor != cur) {
        cur_ = cursor;
    }
    char symbol_ = '\0';
    if (cur_.x_ > 0) {
        symbol_ = text_[cur_.y_][cur_.x_ - 1];
        text_[cur_.y_].erase(cur_.x_ - 1, 1);
        ShiftLeft();
    } else if (cur_.y_ > 0) {
        --cur_.y_;
        cur_.x_ = text_[cur_.y_].size();
        text_[cur_.y_] += text_[cur_.y_ + 1];
        text_.erase(text_.begin() + cur_.y_ + 1);
        symbol_ = '\n';
    }
    cursor = cur_;
    return symbol_;
}

void TextEditor::PasteNewLine(Cursor& cursor) {
    if (cursor != cur) {
        cur_ = cursor;
    }
    text_.insert(text_.begin() + cur_.y_ + 1, text_[cur_.y_].substr(cur_.x_, text_[cur_.y_].size() - cur_.x_));
    text_[cur_.y_].resize(cur_.x_);
    cur_.y_++;
    cur_.x_ = 0;
    cursor = cur_;
}

void TextEditor::ReverseNewLine(Cursor& cursor) {
    if (cursor != cur) {
        cur_ = cursor;
    }
    --cur_.y_;
    cur_.x_ = text_[cur_.y_].size();
    text_[cur_.y_] += text_[cur_.y_ + 1];
    text_.erase(text_.begin() + cur_.y_ + 1);
    cursor = cur_;
}

void TextEditor::Type(char symbol, Cursor& cursor) {
    if (cursor != cur) {
        cur_ = cursor;
    }
    if (symbol == '\n') {
        text_.insert(text_.begin() + cur_.y_ + 1, text_[cur_.y_].substr(cur_.x_, text_[cur_.y_].size() - cur_.x_));
        text_[cur_.y_].resize(cur_.x_);
        cur_.y_++;
        cur_.x_ = 0;
    } else {
        text_[cur_.y_].insert(cur_.x_, std::string(1, symbol));
        ShiftRight();
    }
    cursor = cur_;
}

void TextEditor::Undo() {
    if (!storage_.for_undo.empty()) {
        storage_.for_undo.top()->Undo(this);
        storage_.for_undo.pop();
    }
}

void TextEditor::Redo() {
    if (!storage_.for_redo.empty()) {
        storage_.for_redo.top()->Do(this);
        storage_.for_redo.pop();
    }
}

void TextEditor::Print(std::ostream& os) const {
    for (size_t i = 0; i < text_.size(); ++i) {
        os << text_[i];
        if (i + 1 != text_.size()) {
            os << '\n';
        }
    }
}
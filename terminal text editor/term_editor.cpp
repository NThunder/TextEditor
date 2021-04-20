/*** includes ***/

#define _DEFAULT_SOURSE
#define _BSD_SOURSE
#define _GNU_SOURSE

#include<cctype>
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <vector>
#include <stack>
#include <iostream>
#include <sstream>

/*** defines **/

#define TERM_EDITOR_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

enum EditorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

/*** data ***/

class TextEditor;

class IAction {

public:
    virtual void Do(TextEditor*) = 0;
    virtual void Undo(TextEditor*) = 0;
    virtual ~IAction() = default;
};

struct ActionHistory {
    std::stack<IAction*> for_undo;
    std::stack<IAction*> for_redo;
    ~ActionHistory();
};

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

struct Cursor {
    size_t x_ = 0;
    size_t y_ = 0;
    bool operator==(const Cursor&);
    bool operator!=(const Cursor&);
};

bool Cursor::operator==(const Cursor& s) {
    return x_ == s.x_ && y_ == s.y_;
}

bool Cursor::operator!=(const Cursor& s) {
    return !(*this == s);
}

Cursor cur;

class TextEditor {
    std::vector<std::string> text_;
    size_t screenrows;
    size_t screencols;
    size_t rowoff;
    size_t coloff;

public:
    ActionHistory storage_;
    Cursor cur_;
    void EditorOpen(char*);
    void EditorAppendRow(char*, size_t);
    void EditorScroll();
    void EditorDrawRows(std::string&);
    void EditorRefreshScreen();
    void EditorMoveCursor(int);
    void EditorProcessKeypress();
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

class TypeAction : public IAction {
    char symbol_;
    Cursor cur_;

public:
    explicit TypeAction(const char&);
    void Do(TextEditor*) override;
    void Undo(TextEditor*) override;
};

class DelAction : public IAction {
    char symbol_;
    Cursor cur_;

public:
    DelAction() = default;
    void Do(TextEditor*) override;
    void Undo(TextEditor*) override;
};

class BackAction : public IAction {
    char symbol_;
    Cursor cur_;

public:
    BackAction() = default;
    void Do(TextEditor*) override;
    void Undo(TextEditor*) override;
};

class NewLineAction : public IAction {
    Cursor cur_;

public:
    NewLineAction() = default;
    void Do(TextEditor*) override;
    void Undo(TextEditor*) override;
};

TypeAction::TypeAction(const char& symbol) : symbol_(symbol) {
}

void TypeAction::Do(TextEditor* text) {
    text->Type(symbol_, cur_);
    text->storage_.for_undo.push(this);
}

void DelAction::Do(TextEditor* text) {
    symbol_ = text->Delete(cur_);
    if (symbol_ != '\0') {
        text->storage_.for_undo.push(this);
    } else {
        delete this;
    }
}

void BackAction::Do(TextEditor* text) {
    symbol_ = text->BackSpace(cur_);
    if (symbol_ != '\0') {
        text->storage_.for_undo.push(this);
    } else {
        delete this;
    }
}

void NewLineAction::Do(TextEditor* text) {
    text->PasteNewLine(cur_);
    text->storage_.for_undo.push(this);
}

void TypeAction::Undo(TextEditor* text) {
    symbol_ = text->BackSpace(cur_);
    if (symbol_ != '\0') {
        text->storage_.for_redo.push(this);
    } else {
        delete this;
    }
}

void DelAction::Undo(TextEditor* text) {
    text->Type(symbol_, cur_);
    text->ShiftLeft();
    cur_ = text->cur_;
    text->storage_.for_redo.push(this);
}

void BackAction::Undo(TextEditor* text) {
    text->Type(symbol_, cur_);
    text->storage_.for_redo.push(this);
}

void NewLineAction::Undo(TextEditor* text) {
    text->ReverseNewLine(cur_);
    text->storage_.for_redo.push(this);
}
/*** terminal ***/

struct termios orig_termios;

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void DisableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        die("tcsetattr");
    }
}

void EnableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        die("tcgetattr");
    }
    atexit(DisableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON | BRKINT |INPCK |ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

int EditorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)  {
                    return '\x1b';
                }
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

int GetWindowSize(size_t* rows, size_t* cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** text_ operations ***/

void TextEditor::EditorAppendRow(char *s, size_t len) {
    text_.resize(text_.size() + 1);
    text_[text_.size() - 1] += std::string(s, len);
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
    auto a = new DelAction;
    a->Do(this);
}

void TextEditor::BackSpace() {
    CheckRedo();
    auto a = new BackAction;
    a->Do(this);
}

void TextEditor::PasteNewLine() {
    CheckRedo();
    auto a = new NewLineAction;
    a->Do(this);
}

void TextEditor::Type(char symbol) {
    CheckRedo();
    auto a = new TypeAction(symbol);
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
    text_.insert(text_.begin() + cur_.y_ + 1,
    text_[cur_.y_].substr(cur_.x_, text_[cur_.y_].size() - cur_.x_));
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
        text_.insert(text_.begin() + cur_.y_ + 1,
        text_[cur_.y_].substr(cur_.x_, text_[cur_.y_].size() - cur_.x_));
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

/*** file i/o ***/

void TextEditor::EditorOpen(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        die("fopen");
    }
    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r')) {
            linelen--;
        }
        EditorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}

/*** output ***/

void TextEditor::EditorScroll() {
    if (cur_.y_ < rowoff) {
        rowoff = cur_.y_;
    }
    if (cur_.y_ >= rowoff + screenrows) {
        rowoff = cur_.y_ - screenrows + 1;
    }
    if (cur_.x_ < coloff) {
        coloff = cur_.x_;
    }
    if (cur_.x_ >= coloff + screencols) {
        coloff = cur_.x_ - screencols + 1;
    }
}

void TextEditor::EditorDrawRows(std::string& ab) {
    for (size_t y = 0; y < screenrows; y++) {
        int filerow = y + rowoff;
        if (filerow >= static_cast<int>(text_.size())) {
            if (text_.size() == 1 && text_[0].size() == 0 && y == screenrows / 3) {
                char welcome[80];
                size_t welcomelen = snprintf(welcome, sizeof(welcome),
                    "term editor -- version %s", TERM_EDITOR_VERSION);
                if (welcomelen > screencols) {
                    welcomelen = screencols;
                }
                int padding = (screencols - welcomelen) / 2;
                if (padding) {
                    ab += "~";
                    padding--;
                }
                while (padding--) {
                    ab += " ";
                }
                ab.append(welcome, welcomelen);
            } else {
                ab += "~";
            }
        } else {
            int len = text_[filerow].size() - coloff;
            if (len < 0) {
                len = 0;
            }
            if (len > static_cast<int>(screencols)) {
                len = screencols;
            }
            if (len > 0) {
                ab.append(text_[filerow], coloff, len);
            }
        }

        ab += "\x1b[K";
        if (y < screenrows - 1) {
            ab += "\r\n";
        }
    }
}

void TextEditor::EditorRefreshScreen() {
    EditorScroll();
    std::string ab;

    ab += "\x1b[?25l";
    ab += "\x1b[H";

    EditorDrawRows(ab);
    char buff[32];
    snprintf(buff, sizeof(buff), "\x1b[%ld;%ldH", (cur_.y_ - rowoff) + 1, (cur_.x_ - coloff) + 1);
    ab.append(buff, strlen(buff));

    ab += "\x1b[?25h";

    write(STDOUT_FILENO, ab.c_str(), ab.size());
}

void TextEditor::Print(std::ostream& os) const {
    for (auto a : text_) {
        os << a  << '\n';
    }
}

/*** input ***/

void TextEditor::EditorMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            ShiftLeft();
            break;
        case ARROW_RIGHT:
            ShiftRight();
            break;
        case ARROW_UP:
            ShiftUp();
            break;
        case ARROW_DOWN:
            ShiftDown();
            break;
    }
}

void TextEditor::EditorProcessKeypress() {
    int symbol = EditorReadKey();

    switch (symbol) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case CTRL_KEY('z'):
            Undo();
            break;
        case CTRL_KEY('y'):
            Redo();
            break;

        case DEL_KEY:
            Delete();
            break;

        case HOME_KEY:
            cur_.x_ = 0;
            break;
        case END_KEY:
            cur_.x_ = screencols - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = screenrows;
                while (--times) {
                    EditorMoveCursor(symbol == PAGE_UP ? ARROW_UP : ARROW_DOWN);
                }
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            EditorMoveCursor(symbol);
            break;
        case 13:
            PasteNewLine();
            break;
        case 127:
            BackSpace();
            break;
        default:
            Type(symbol);
    }
}

/*** init ***/

TextEditor::TextEditor() {
    rowoff = 0;
    coloff = 0;
    text_.resize(1);
    if (GetWindowSize(&screenrows, &screencols) == -1) {
        die("GetWindowSize");
    }
}

int main(int argc, char *argv[]) {
    EnableRawMode();
    TextEditor text;
    if (argc >= 2) {
        text.EditorOpen(argv[1]);
    }

    while (1) {
        text.EditorRefreshScreen();
        text.EditorProcessKeypress();
    }
    return 0;
}
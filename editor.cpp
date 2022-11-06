#include <conio.h>
#include <iostream>
#include <windows.h>
#include <string>
#include <algorithm>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <fstream>
#define Singleton(name) \
private:\
name() = default;\
~name() = default;\
public:\
name(const name&) = delete;\
name(const name&&) = delete;\
name& operator=(const name&) = delete;\
name& operator=(const name&&) = delete;\
public:\
	static auto& getInstance() {\
		static name instance;\
		return instance;\
	}\
private:

class File {
private:
	const std::string name;
public:
	File(decltype(name) name) :name(name) {

	}
	std::string read() {
		std::fstream file(name.c_str());
		if (!file)return "";
		return std::string(std::istreambuf_iterator<decltype(name)::value_type >(file),std::istreambuf_iterator<decltype(name)::value_type>());
	}
	auto write(decltype(name) str) {
		std::ofstream file(name.c_str());
		if (!file)return false;
		file << str;
		return true;
	}
};
class Console final {
private:
	Singleton(Console);
public:
	inline auto& scroll(SMALL_RECT range, const COORD pos) noexcept(ScrollConsoleScreenBuffer) {
		CHAR_INFO info;
		info.Attributes = 0;
		info.Char.AsciiChar = ' ';
		ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE), &range, nullptr,pos, &info);
		return *this;
	}
	inline auto read(const std::size_t size, const COORD pos) {
		std::string content;
		DWORD length;
		content.resize(size);
		ReadConsoleOutputCharacterA(GetStdHandle(STD_OUTPUT_HANDLE), content.data(), content.size(), pos, &length);
		return std::move(content);
	}
	inline auto& setCursorPos(COORD pos) {
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
		return *this;
	}
	inline auto& setCodePage(decltype(CP_ACP) codePage) {
		SetConsoleCP(codePage);
		SetConsoleOutputCP(codePage);
		return *this;
	}
	inline auto getCursorPos() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return info.dwCursorPosition;
	}
	inline auto getScreenSize() {//•ª‚¯‚é‚Æ“ñ‰ñŒÄ‚Ño‚µ‚Ä‚¢‚é‚Ì‚ÅŒø—¦‚ªˆ«‚¢
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return info.dwSize;
	}
	inline auto getTitle() {
		std::string title;
		title.resize(MAX_PATH);
		title.resize(GetConsoleTitleA(title.data(),title.size()));
		title.shrink_to_fit();
		return std::move(title);
	}
	inline auto getDefaultTitle() {
		std::string title;
		title.resize(MAX_PATH);
		title.resize(GetConsoleOriginalTitleA(title.data(), title.size()));
		title.shrink_to_fit();
		return std::move(title);
	}
	inline auto setTitle(std::string title) {
		SetConsoleTitleA(title.c_str());
	}
	inline auto setScrollSize(COORD size)const {
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),size);
	}
};

class Event {
private:
public:
	using eventT = decltype(INPUT_RECORD::Event)&;
	virtual bool excute(eventT) = 0;
};
class Input {
private:
	std::unordered_map<DWORD, std::unique_ptr<Event> > events;
	DWORD oldMode;
public:
	Input() {
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldMode);
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);//ENABLE_ECHO_INPUT |ENABLE_LINE_INPUT |ENABLE_QUICK_EDIT_MODE| ENABLE_INSERT_MODE|
	}
	~Input() {
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), oldMode);
	}
	template <class T, class... ArgT>
	auto emplace(decltype(events)::key_type key, ArgT... args) {
		events.emplace(key, std::make_unique<T>(args...));
	}
	auto input() {
		while (true) {
			INPUT_RECORD record;
			DWORD length;
			if (!ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &length) || !events.count(record.EventType)) {
				continue;
			}
			if (events[record.EventType]->excute(record.Event))break;
		}
	}
};
class TextEditor {
private:
	static constexpr auto END_LINE = '|';
	inline auto readAll() {
		std::string text;
		for (short y = 0;; ++y) {
			const auto line = Console::getInstance().read(Console::getInstance().getScreenSize().X, {0,y});
			const auto endLine = line.find_last_of(END_LINE);
			if (endLine == std::string::npos)return text.substr(0,text.size()-1/*\n size*/);
			text.append(line.substr(0,endLine)+"\n");
		}
	}
public:
	TextEditor() {
		auto screen = Console::getInstance().getScreenSize();
		Console::getInstance().scroll({ 0,0,screen.X,screen.Y }, { 0,static_cast<decltype(screen.Y)>(-screen.Y) });
		putchar(END_LINE);
		Console::getInstance().setCursorPos({0,0});
	}
	auto enter() {
		auto cursor = Console::getInstance().getCursorPos();
		auto screen = Console::getInstance().getScreenSize();
		SMALL_RECT range;
		range.Left = 0;
		range.Top = ++cursor.Y;
		range.Right = screen.X;
		range.Bottom = screen.Y;
		Console::getInstance().scroll(range, { 0,++cursor.Y });
		range.Left = cursor.X;
		cursor.X = 0;
		cursor.Y = range.Top;
		range.Bottom = --range.Top;
		Console::getInstance().scroll(range, cursor).read(0, { 0,0 });
		putchar(END_LINE);
		Console::getInstance().setCursorPos(cursor);
		if (Console::getInstance().read(screen.X, { 0,static_cast<decltype(screen.Y)>(screen.Y - 1) }).find(END_LINE) != std::string::npos) {
			++screen.Y;
			Console::getInstance().setScrollSize(screen);
		}
	}
	inline auto move(COORD pos) {
		const auto line = Console::getInstance().read(Console::getInstance().getScreenSize().X, { 0,pos.Y });
		const short endLine = line.find_last_of(END_LINE);
		if (endLine <= pos.X) {
			pos.X = endLine;
		}
		if (IsDBCSLeadByte(line[pos.X-1])) {
			--pos.X;
		}
		Console::getInstance().setCursorPos(pos);
	}
	inline auto up() {
		auto cursor = Console::getInstance().getCursorPos();
		if (cursor.Y <= 0)return;
		cursor.X -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.X)>(cursor.X - 1),--cursor.Y }).front());
		const auto endLine = Console::getInstance().read(Console::getInstance().getScreenSize().X, {0,cursor.Y}).find_last_of(END_LINE);
		if (endLine <= cursor.X) {
			cursor.X = endLine;
		}
		Console::getInstance().setCursorPos(cursor);
	}
	inline auto down() {
		auto cursor = Console::getInstance().getCursorPos();
		cursor.X -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.X)>(cursor.X - 1),++cursor.Y }).front());
		const auto endLine = Console::getInstance().read(Console::getInstance().getScreenSize().X, { 0,cursor.Y }).find_last_of(END_LINE);
		if (endLine == std::string::npos) {
			--cursor.Y;
		}
		else if (endLine <= cursor.X) {
			cursor.X = endLine;
		}
		Console::getInstance().setCursorPos(cursor);
	}
	auto left() {
		auto cursor = Console::getInstance().getCursorPos();
		if (cursor.Y > 0 && !cursor.X) {
			--cursor.Y;
			cursor.X = Console::getInstance().read(Console::getInstance().getScreenSize().X, cursor).find_last_of(END_LINE);
		}
		else if (cursor.X < 0)return;
		else {
			cursor.X -= 2;
			cursor.X += !IsDBCSLeadByte(Console::getInstance().read(2, cursor).front());
		}
		Console::getInstance().setCursorPos(cursor);
	}
	inline auto right() {
		auto cursor = Console::getInstance().getCursorPos();
		const auto endLine = cursor.X+ Console::getInstance().read(Console::getInstance().getScreenSize().X - cursor.X, cursor).find_last_of(END_LINE);
		if (endLine == cursor.X) {
			if (Console::getInstance().read(Console::getInstance().getScreenSize().X, { 0,static_cast<decltype(cursor.Y)>(cursor.Y + 1) }).find_last_of(END_LINE) == std::string::npos) return;
			++cursor.Y;
			cursor.X = 0;
		}
		else if (endLine <= cursor.X)return;
		else {
			++cursor.X += IsDBCSLeadByte(Console::getInstance().read(2, cursor).front());
		}
		Console::getInstance().setCursorPos(cursor);
	}
	auto backspace() {
		auto cursor = Console::getInstance().getCursorPos();
		auto screen = Console::getInstance().getScreenSize();
		if (cursor.X) {
			SMALL_RECT range;
			range.Left = cursor.X--;
			range.Right = screen.X;
			range.Top = range.Bottom = cursor.Y;
			Console::getInstance()
				.scroll(range, { cursor.X -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.X)>(cursor.X - 1),cursor.Y }).front()),cursor.Y })
				.setCursorPos(cursor);
			return;
		}
		if (!cursor.Y)return;
		SMALL_RECT range;
		range.Left = cursor.X;
		range.Top = cursor.Y;
		range.Right = screen.X;
		range.Bottom = cursor.Y;
		--cursor.Y;
		auto content = Console::getInstance().read(screen.X, cursor);
		content = content.substr(0, content.find_last_of(END_LINE));
		cursor.X = content.size();
		Console::getInstance()
			.setCursorPos(cursor)
			.scroll(range, cursor);
		++range.Top;
		range.Bottom = screen.Y;
		Console::getInstance().scroll(range, { 0,static_cast<decltype(range.Top)>(range.Top - 1) });

	}
	auto insert(const char c) {
		auto cursor = Console::getInstance().getCursorPos();
		auto screen = Console::getInstance().getScreenSize();
		SMALL_RECT range;
		range.Left = cursor.X++;
		range.Right = screen.X;
		range.Top = range.Bottom = cursor.Y;
		if (Console::getInstance().read(1/*END_LINE.size*/, { static_cast<decltype(screen.X)>(screen.X - 1),cursor.Y }).front() == END_LINE) {
			++screen.X;
			Console::getInstance().setScrollSize(screen);
		}
		Console::getInstance().scroll(range, cursor);
		putchar(c);
	}
};

class KeyEvent:public Event{
private:
	TextEditor &editor;
public:
	KeyEvent(decltype(editor) editor) :editor(editor) {

	}
	bool excute(Event::eventT e)override {
		if (!e.KeyEvent.bKeyDown)return false;
		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_RETURN:
			editor.enter();
			break;
		case VK_UP:
			editor.up();
			break;
		case VK_DOWN:
			editor.down();
			break;
		case VK_LEFT:
			editor.left();
			break;
		case VK_RIGHT:
			editor.right();
			break;
		case VK_BACK:
			editor.backspace();
			break;
		default:
			editor.insert(e.KeyEvent.uChar.AsciiChar);
		}
		return false;
	}
};
class MouseEvent:public Event{
private:
	TextEditor& editor;
public:
	MouseEvent(decltype(editor) editor):editor(editor){}
	bool excute(Event::eventT e) {
		if (e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			editor.move(e.MouseEvent.dwMousePosition);
		}
		return false;
	}
};
int main() {
	TextEditor editor;
	Input input;
	input.emplace<KeyEvent>(KEY_EVENT,editor);
	input.emplace<MouseEvent>(MOUSE_EVENT,editor);
	input.input();
	return EXIT_SUCCESS;
}

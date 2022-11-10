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
#include <regex>
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
	inline auto readColor(const std::size_t size, const COORD pos) {
		std::vector<WORD> content;
		DWORD length;
		content.resize(size);
		ReadConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE),content.data(), content.size(), pos, &length);
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
	inline auto getScreenSize() {//ï™ÇØÇÈÇ∆ìÒâÒåƒÇ—èoÇµÇƒÇ¢ÇÈÇÃÇ≈å¯ó¶Ç™à´Ç¢
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
	//enum class COLOR {
	//	BLACK = 0,
	//	DARKBLUE = FOREGROUND_BLUE,
	//	DARKGREEN = FOREGROUND_GREEN,
	//	DARKCYAN = FOREGROUND_GREEN | FOREGROUND_BLUE,
	//	DARKRED = FOREGROUND_RED,
	//	DARKMAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
	//	DARKYELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
	//	DARKGRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	//	GRAY = FOREGROUND_INTENSITY,
	//	BLUE = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
	//	GREEN = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
	//	CYAN = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
	//	RED = FOREGROUND_INTENSITY | FOREGROUND_RED,
	//	MAGENTA = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
	//	YELLOW = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
	//	WHITE = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	//};

	inline auto& setColor(WORD color, DWORD length,COORD pos) {
		FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color, length, pos, &length);
		return *this;
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
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT |ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);//ENABLE_ECHO_INPUT |ENABLE_LINE_INPUT |ENABLE_QUICK_EDIT_MODE| ENABLE_INSERT_MODE|
	}
	~Input() {
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), oldMode);
	}
	template <class T, class... ArgT>
	auto emplace(decltype(events)::key_type key, ArgT&&... args) {
		events.emplace(key, std::make_unique<T>(std::forward<ArgT...>(args)...));
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
class CommandMode{
private:
	static constexpr auto CURSOR = '|';
	std::size_t pos;
public:
	CommandMode() {
		Console::getInstance().setTitle(std::string(1,CURSOR));
	}
	auto cancel() {
		Console::getInstance().setTitle("");
	}
	auto backspace() {
		if (pos <= 0)return;
		auto text = Console::getInstance().getTitle();
		const auto isMultiByte = --pos > 0 ? IsDBCSLeadByte(text[pos - 1]) : false;
		text.erase(pos -= isMultiByte, 1 + isMultiByte);
		Console::getInstance().setTitle(text);
	}
	auto insert(const char c) {
		auto text = Console::getInstance().getTitle();
		if (text.size() >= MAX_PATH)return;
		text.erase(pos, 1);
		std::string add(1, c);
		if (IsDBCSLeadByte(c))add.push_back(_getch());
		add.push_back(CURSOR);
		text.insert(pos, add);
		pos += add.size() - 1;
		Console::getInstance().setTitle(text);
	}
	auto left() {
		if (!pos)return;
		auto text=Console::getInstance().getTitle();
		std::swap(text[pos], text[pos-1]);
		--pos;
		Console::getInstance().setTitle(text);
	}
	auto right() {
		auto text = Console::getInstance().getTitle();
		if (text.size()-1 <= pos)return;
		std::swap(text[pos],text[pos+1]);
		++pos;
		Console::getInstance().setTitle(text);
	}
	auto enter() {
		auto text=Console::getInstance().getTitle();
		text.erase(pos);
		Console::getInstance().setTitle(text);
	}
};
class Split {
private:
	std::string str,
		        separator;
public:
	Split(decltype(str) str,decltype(str) separator):str(str),separator(separator) {}
	const auto next() {
		const auto found = str.find_first_of(separator);
		if (found == std::string::npos)return std::exchange(str,"");
		const auto item = str.substr(0,found);
		str.erase(0,found+separator.size());
		return std::move(item);
	}
	auto get() {
		const auto found = str.find_first_of(separator);
		if (found == std::string::npos)return std::string();
		return std::move(str.substr(0, found));
	}
};
class Command {
private:
	static constexpr auto SEPARATOR = " ";
	std::unordered_map<std::string, std::function<std::string(Split&)> > commands;
public:
	template <class keyT,class valueT>
	auto emplace(keyT key,valueT value) {
		commands.emplace(key,value);
	}
	auto operator[](std::string str) {
		Split split(std::move(str), SEPARATOR);
		if (!commands.count(split.get()))return std::string("error:command not found");
		return commands.at(split.next())(split);
	}
};
class TextEditor {
private:
	static constexpr auto END_LINE = '|';
	Command cmd;
	inline auto readAll() {
		std::string text;
		for (short y = 0;; ++y) {
			const auto line = Console::getInstance().read(Console::getInstance().getScreenSize().X, { 0,y });
			const auto endLine = line.find_last_of(END_LINE);
			if (endLine == std::string::npos)return text.substr(0, text.size() - 1/*\n size*/);
			text.append(line.substr(0, endLine) + "\n");
		}
	}
	auto reset() {
		auto screen = Console::getInstance().getScreenSize();
		Console::getInstance().scroll({ 0,0,screen.X,screen.Y }, { 0,static_cast<decltype(screen.Y)>(-screen.Y) });
		putchar(END_LINE);
		Console::getInstance().setCursorPos({ 0,0 });
	}
public:
	TextEditor() {
		reset();
		Console::getInstance().setCursorPos({ 0,0 });
		cmd.emplace(
			"save",
			[this](Split& args) {
				return File(args.next()).write(readAll()) ? Console::getInstance().getDefaultTitle() : "failed";
			}
		);
		cmd.emplace(
			"read",
			[this](Split& args) {
				reset();
				auto text = File(args.next()).read();
				text.push_back('\n');
				for (auto pos = text.find_first_of('\n'); pos != std::string::npos; text.erase(0, pos + 1), pos = text.find_first_of('\n')) {
					if (Console::getInstance().getScreenSize().X < pos) {
						Console::getInstance().setScrollSize({ static_cast<short>(pos),Console::getInstance().getScreenSize().Y });
					}
					std::cout << text.substr(0, pos) << END_LINE << std::endl;
				}
				return Console::getInstance().setCursorPos({0,0}).getDefaultTitle();

			}
		);
	}
	auto run(std::string text) {
		Console::getInstance().setTitle(std::move(cmd[std::move(text)]));
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
		auto cursor=Console::getInstance().getCursorPos();
		--cursor.Y;
		move(cursor);
	}
	inline auto down() {
		auto cursor = Console::getInstance().getCursorPos();
		++cursor.Y;
		move(cursor);
	}
	auto left() {
		auto cursor = Console::getInstance().getCursorPos();
		--cursor.X;
		move(cursor);
		if (cursor.X==Console::getInstance().getCursorPos().X)return;
		--cursor.Y;
		cursor.X = Console::getInstance().getScreenSize().X;
		move(cursor);
	}
	inline auto right() {
		auto cursor = Console::getInstance().getCursorPos();
		++cursor.X;
		move(cursor);
		if (cursor.X == Console::getInstance().getCursorPos().X)return;
		++cursor.Y;
		cursor.X = 0;
		move(cursor);
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
class CommandModeKeyEvent :public Event {
private:
	CommandMode cmd;
public:
	bool excute(Event::eventT e) {
		if (!e.KeyEvent.bKeyDown)return false;
		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_MENU:
			cmd.cancel();
			return true;
			break;
		case VK_RETURN:
			cmd.enter();
			return true;
			break;
		case VK_LEFT:
			cmd.left();
			break;
		case VK_RIGHT:
			cmd.right();
			break;
		case VK_BACK:
			cmd.backspace();
			break;
		default:
			cmd.insert(e.KeyEvent.uChar.AsciiChar);
			break;
		}
		return false;
	}
};
class KeyEvent:public Event{
private:
	TextEditor &editor;//éQè∆Ç…Ç∑ÇÈÇ∆cmd.runÇÃmapÇÃì‡óeÇ§Ç‹Ç≠ì«Ç›çûÇﬂÇ»Ç¢
public:
	KeyEvent(decltype(editor) editor):editor(editor) {

	}
	bool excute(Event::eventT e)override {
		if (!e.KeyEvent.bKeyDown)return false;
		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_ESCAPE:
			return true;
			break;
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
		case VK_MENU:
		{
			Input input;
			input.emplace<CommandModeKeyEvent>(KEY_EVENT);
			input.input();
			editor.run(std::move(Console::getInstance().getTitle()));
		}
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
/*		if (e.MouseEvent.dwEventFlags == MOUSE_MOVED && e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			Console::getInstance().setColor(~Console::getInstance().readColor(1,e.MouseEvent.dwMousePosition).front(), 1, e.MouseEvent.dwMousePosition);
		}
		else */if (e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
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

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
	inline auto getScreenSize() {//分けると二回呼び出しているので効率が悪い
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
	inline auto& setColor(WORD color, DWORD length,COORD pos) {
		FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color, length, pos, &length);
		return *this;
	}
	inline auto getMode() {
		DWORD mode;
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
		return std::move(mode);
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
		//selectを自作する場合,ctrl+cがEND_LINEの"|"もコピーしてしまうのでENABLE_PROCESSED_INPUTは入れない
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),  ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);//ENABLE_ECHO_INPUT |ENABLE_LINE_INPUT |ENABLE_QUICK_EDIT_MODE| ENABLE_INSERT_MODE|
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
		Console::getInstance().setTitle("cancel");
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
		if (found == std::string::npos)return str;
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
public:
	static constexpr auto END_LINE = '|';
private:
	static constexpr auto NO_SELECT = -1;
	Command cmd;
	COORD selectBegin;
	inline auto readAll() {
		std::string text;
		for (short y = 0;; ++y) {
			const auto line = Console::getInstance().read(Console::getInstance().getScreenSize().X, { 0,y });
			const auto endLine = line.find_last_of(END_LINE);
			if (endLine == std::string::npos)return text.substr(0, text.size() - 1/*\n size*/);
			text.append(line.substr(0, endLine) + "\n");
		}
	}
	auto read(COORD& start, COORD& end) {
		std::string text = std::move(Console::getInstance().read(Console::getInstance().getScreenSize().X, start));
		const auto found = text.find_last_of(END_LINE);
		if (start.Y == end.Y && end.X-start.X < found) {
			return std::move(text.erase(end.X-start.X, text.size() - end.X+start.X));
		}
		text.erase(found, text.size()-found).push_back('\n');
		++start.Y;
		start.X = 0;
		for (; start.Y < end.Y; ++start.Y)
		{
			auto line = Console::getInstance().read(Console::getInstance().getScreenSize().X, start);
			const auto found = line.find_last_of(END_LINE);
			line.erase(found, line.size()-found).push_back('\n');
			text.append(std::move(line));
		}
		return std::move(text.append(std::move(Console::getInstance().read(end.X, { 0,end.Y }))));
	}
	auto reset() {
		auto screen = Console::getInstance().getScreenSize();
		Console::getInstance().scroll({ 0,0,screen.X,screen.Y }, { 0,static_cast<decltype(screen.Y)>(-screen.Y) });
		putchar(END_LINE);
		Console::getInstance().setCursorPos({ 0,0 });
	}
public:
	auto selectRange() {
		if (Console::getInstance().getCursorPos().Y < selectBegin.Y || (Console::getInstance().getCursorPos().Y == selectBegin.Y && (Console::getInstance().getCursorPos().X < selectBegin.X))) {
			return std::make_pair(Console::getInstance().getCursorPos(), selectBegin);//swap
		}
		return std::make_pair(selectBegin, Console::getInstance().getCursorPos());
	}
	auto cancelSelect() {
		auto range = selectRange();
		for (
			decltype(range.first.X) x = range.first.X,
			y = range.first.Y,
			length = range.first.X + Console::getInstance().read(Console::getInstance().getScreenSize().X, range.first).find_last_of(END_LINE);//editor.getend
			y < range.second.Y;
			++y,
			x = 0,
			length=Console::getInstance().read(Console::getInstance().getScreenSize().X, { x,y }).find_last_of(END_LINE)
			)
		{
			for (; x < length; ++x) {
				Console::getInstance().setColor(~Console::getInstance().readColor(1, { x,y }).front(), 1, { x,y });
			}
		}
		for (decltype(range.first.X) x = 0; x < range.second.X; ++x) {
			Console::getInstance().setColor(~Console::getInstance().readColor(1, { x,range.second.Y }).front(), 1, { x,range.second.Y });
		}
		selectBegin.X = NO_SELECT;
	}
	TextEditor() {
		cancelSelect();
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
		cmd.emplace(
		"cancel",
			[this](Split& args) {
				return Console::getInstance().getDefaultTitle();
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
	auto insert(std::string str) {
		auto cursor = Console::getInstance().getCursorPos();
		auto screen = Console::getInstance().getScreenSize();
		SMALL_RECT range;
		range.Left = cursor.X;
		range.Right = screen.X;
		range.Top = range.Bottom = cursor.Y;
		cursor.X += str.size();
		if (Console::getInstance().read(screen.X, { 0,cursor.Y }).find_last_of(END_LINE)<=str.size()) {
			screen.X+=str.size();
			Console::getInstance().setScrollSize(screen);
		}
		Console::getInstance().scroll(range, cursor);
		std::cout << str;
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

	auto deleteSelect() {
		auto range=selectRange();
		Console::getInstance()
			.setCursorPos(range.first)
			.scroll(
			{
				range.second.X,
				range.second.Y,
				Console::getInstance().getScreenSize().X,
				range.second.Y
			},
			range.first
		)
			.scroll(
			{
				0,
				++range.second.Y,
				Console::getInstance().getScreenSize().X,
				Console::getInstance().getScreenSize().Y
			},
			{0,++range.first.Y}
		);
		selectBegin.X = NO_SELECT;

	}
   	inline auto is_selecting() {
		return selectBegin.X != NO_SELECT;
	}
	auto select(decltype(selectBegin) pos) {
		if (!is_selecting()) {
			selectBegin = pos;
			return;
		}
	}
	auto getSelect() {
		auto range = selectRange();
		return std::move(read(range.first,range.second));
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
	TextEditor &editor;
public:
	KeyEvent(decltype(editor) editor):editor(editor) {

	}
	bool excute(Event::eventT e)override {
		if (!e.KeyEvent.bKeyDown)return false;
		if (e.KeyEvent.wVirtualKeyCode=='C' && (e.KeyEvent.dwControlKeyState == LEFT_CTRL_PRESSED || e.KeyEvent.dwControlKeyState == RIGHT_CTRL_PRESSED)&& OpenClipboard(nullptr)) {
			const auto data = editor.getSelect();
			//HGLOBAL textData = GlobalAlloc(GHND,data.size()+1);
			//data.copy(static_cast<PSTR>(GlobalLock(textData)),data.size());
			//GlobalUnlock(textData);
			EmptyClipboard();
			SetClipboardData(CF_TEXT, (void*)(&data[0]));
			CloseClipboard();
			return false;
		}
		else if (e.KeyEvent.wVirtualKeyCode == 'V' && (e.KeyEvent.dwControlKeyState == LEFT_CTRL_PRESSED || e.KeyEvent.dwControlKeyState == RIGHT_CTRL_PRESSED) && OpenClipboard(nullptr)) {
			Split split(static_cast<const char*>(GetClipboardData(CF_TEXT)),"\r\n");
			CloseClipboard();
			while (true) {
				editor.insert(split.next());
				if (split.get().empty())break;
				editor.enter();
			}
		}

		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_ESCAPE:
			return true;
			break;
		case VK_RETURN:
			if (editor.is_selecting()) {
				editor.deleteSelect();
			}
			editor.enter();
			break;
		case VK_UP:
			if (editor.is_selecting()) {
				const auto left = editor.selectRange().first;
				editor.cancelSelect();
				Console::getInstance().setCursorPos(left);
			}
			editor.up();
			break;
		case VK_DOWN:
			if (editor.is_selecting()) {
				const auto right = editor.selectRange().second;
				editor.cancelSelect();
				Console::getInstance().setCursorPos(right);
			}
			editor.down();
			break;
		case VK_LEFT:
			if (editor.is_selecting()) {
				const auto left = editor.selectRange().first;
				editor.cancelSelect();
				Console::getInstance().setCursorPos(left);
				break;
			}
			editor.left();
			break;
		case VK_RIGHT:
			if (editor.is_selecting()) {
				const auto right = editor.selectRange().second;
				editor.cancelSelect();
				Console::getInstance().setCursorPos(right);
				break;
			}
			editor.right();
			break;
		case VK_BACK:
			if (editor.is_selecting()) {
				editor.deleteSelect();
				break;
			}
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
			if (!(e.KeyEvent.dwControlKeyState == SHIFT_PRESSED || !e.KeyEvent.dwControlKeyState))break;
			if (editor.is_selecting()) {
				editor.deleteSelect();
			}
			editor.insert(e.KeyEvent.uChar.AsciiChar);//selectされているときにeditorへの参照先を変えるとifで切り替えなくてもいい
		}
		return false;
	}
};
class MouseEvent:public Event{
private:
	enum class MOTION {
		ANY,
		LEFT,
		RIGHT
	};
	TextEditor& editor;
	COORD before;
	MOTION motion;
public:
	MouseEvent(decltype(editor) editor):editor(editor){
	
	}
	bool excute(Event::eventT e) {
		if (e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			editor.move(e.MouseEvent.dwMousePosition);
		}
		if (e.MouseEvent.dwEventFlags == MOUSE_MOVED && e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			if (editor.is_selecting()&& before.Y != Console::getInstance().getCursorPos().Y) {
				auto changeColors=[this](auto start,auto end) mutable{
					for (
						decltype(start.X) x = start.X,
						y = start.Y,
						length = start.X + Console::getInstance().read(Console::getInstance().getScreenSize().X, start).find_last_of(editor.END_LINE);//editor.getend
						y < end.Y;
						++y,
						x = 0,
						length=Console::getInstance().read(Console::getInstance().getScreenSize().X, { x,y }).find_last_of(editor.END_LINE)
						)
					{
						for (; x < length; ++x) {
							Console::getInstance().setColor(~Console::getInstance().readColor(1, { x,y }).front(), 1, { x,y });
						}
					}
					for (decltype(start.X) x = 0; x < end.X; ++x) {
						Console::getInstance().setColor(~Console::getInstance().readColor(1, { x,end.Y }).front(), 1, { x,end.Y });
					}
				};
				if (before.Y < Console::getInstance().getCursorPos().Y) {
					changeColors(before,Console::getInstance().getCursorPos());
				}
				else {
					changeColors(Console::getInstance().getCursorPos(),before);
				}
			}
			else if (editor.is_selecting() &&(before.X != Console::getInstance().getCursorPos().X )) {
				const auto changeColors = [this](auto start, auto end) {
					for (auto x = start; x < end; ++x) {
						Console::getInstance().setColor(~Console::getInstance().readColor(1, {x,before.Y}).front(), 1, {x,before.Y});
					}
				};
				if (before.X < Console::getInstance().getCursorPos().X) {
					changeColors(before.X, Console::getInstance().getCursorPos().X);
				}
				else {
					changeColors(Console::getInstance().getCursorPos().X,before.X);
				}
			}
			before = Console::getInstance().getCursorPos();
			editor.select(Console::getInstance().getCursorPos());
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
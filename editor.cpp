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

bool operator<(const COORD&a,const COORD &b) {
	return a.Y < b.Y || (b.Y == a.Y && a.X < b.X);
}
template <std::ranges::range T>
auto adjacent_split(const T range) {
	std::vector<std::pair<std::ranges::range_value_t<T>, std::size_t> > data;
	if (range.empty())return data;
	data.emplace_back(range.front(), 0);
	for (auto & i:range) {
		if (i == data.back().first) {
			++data.back().second;
			continue;
		}
		data.emplace_back(i,1);
	}
	return std::move(data);
}
class Split {
private:
	std::string str,
		separator;
public:
	Split(decltype(str) str, decltype(str) separator) :str(str), separator(separator) {}
	const auto next() {
		const auto found = str.find_first_of(separator);
		if (found == std::string::npos)return std::exchange(str, "");
		const auto item = str.substr(0, found);
		str.erase(0, found + separator.size());
		return std::move(item);
	}
	auto get() {
		const auto found = str.find_first_of(separator);
		if (found == std::string::npos)return str;
		return std::move(str.substr(0, found));
	}
	auto empty() {
		return str.empty();
	}
};
class File {
private:
	const std::string name;
public:
	File(decltype(name) name) :name(name) {

	}
	std::string read() {
		std::fstream file(name.c_str());
		if (!file)return "";
		return std::move(std::string(std::istreambuf_iterator<decltype(name)::value_type >(file), std::istreambuf_iterator<decltype(name)::value_type>()));
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

	inline auto& scroll(SMALL_RECT range, const COORD pos)noexcept(ScrollConsoleScreenBuffer) {
		CHAR_INFO info;
		info.Attributes = 0;
		info.Char.AsciiChar = ' ';
		ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE), &range, nullptr, pos, &info);
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
		ReadConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), content.data(), content.size(), pos, &length);
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
	inline auto getScreenSize() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return info.dwSize;
	}
	inline auto getTitle() {
		std::string title;
		title.resize(MAX_PATH);
		title.resize(GetConsoleTitleA(title.data(), title.size()));
		title.shrink_to_fit();
		return std::move(title);
	}
	inline auto getTitleW() {
		std::wstring title;
		title.resize(MAX_PATH);
		title.resize(GetConsoleTitleW(title.data(), title.size()));
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

	inline auto setTitleW(std::wstring title) {
		SetConsoleTitleW(title.c_str());
	}
	inline auto setScrollSize(COORD size)const {
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);
	}
	inline auto& setColor(WORD color, DWORD length, COORD pos) {
		FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color, length, pos, &length);
		return *this;
	}
	inline auto getMode() {
		DWORD mode;
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
		return mode;
	}
	inline auto &setMode(const DWORD mode) {
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
		return *this;
	}
};		
class Clipboard {
private:
	Singleton(Clipboard);
public:
	auto setString(const std::string text) {
		if (!OpenClipboard(nullptr))return false;
		EmptyClipboard();
		const auto copy = GlobalAlloc(GMEM_MOVEABLE, text.size());
		if (!copy) {
			CloseClipboard();
			return false;
		}
		auto strCopy = GlobalLock(copy);
		memcpy(strCopy, &text.front(), text.size());
		GlobalUnlock(copy);
		SetClipboardData(CF_TEXT, copy);
		CloseClipboard();
		return true;
	}
	auto getData() {
		if (!OpenClipboard(nullptr))return "";
		const auto data=GetClipboardData(CF_TEXT);
		CloseClipboard();
		return static_cast<const char*>(data);
	}
};
class ConsoleEditor {
private:
	Singleton(ConsoleEditor);
	COORD base={-1}, before;
public:
	
	static constexpr auto END_LINE = '|';
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
		if (start.Y == end.Y && end.X - start.X < found) {
			return std::move(text.erase(end.X - start.X, text.size() - end.X + start.X));
		}
		text.erase(found, text.size() - found).push_back('\n');
		++start.Y;
		start.X = 0;
		for (; start.Y < end.Y; ++start.Y)
		{
			auto line = Console::getInstance().read(Console::getInstance().getScreenSize().X, start);
			const auto found = line.find_last_of(END_LINE);
			line.erase(found, line.size() - found).push_back('\n');
			text.append(std::move(line));
		}
		return std::move(text.append(std::move(Console::getInstance().read(end.X, { 0,end.Y }))));
	}
	auto &reset() {
		auto screen = Console::getInstance().getScreenSize();
		Console::getInstance().scroll({ 0,0,screen.X,screen.Y }, { 0,static_cast<decltype(screen.Y)>(-screen.Y) });
		putchar(END_LINE);
		Console::getInstance().setCursorPos({ 0,0 });
		return *this;
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
	inline auto getEndLine(COORD pos) {
		for (pos.X = Console::getInstance().getScreenSize().X; 0 <= pos.X && Console::getInstance().read(1, pos).front() != END_LINE; --pos.X) {}
		return pos.X;
	}

	inline auto move(COORD pos) {
		const auto endLine = getEndLine(pos);
		if (endLine < pos.X)pos.X = endLine;
		else if (IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(pos.X)>(pos.X - 1),pos.Y }).front())) {
			--pos.X;
		}
		Console::getInstance().setCursorPos(pos);
	}
	
	inline auto &up() {
		auto cursor = Console::getInstance().getCursorPos();
		if (!cursor.Y)return *this;
		--cursor.Y;
		move(cursor);
		return *this;
	}
	inline auto &down() {
		auto cursor = Console::getInstance().getCursorPos();
		++cursor.Y;
		move(cursor);
		return *this;
	}
	auto &left() {
		auto cursor = Console::getInstance().getCursorPos();
		if (cursor.X) {
			cursor.X -= 1 + IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.X)>(cursor.X - 2),cursor.Y }).front());
		}
		else if (!cursor.Y)return *this;
		else {
			--cursor.Y;
			for (cursor.X = Console::getInstance().getScreenSize().X; 0 <= cursor.X&& Console::getInstance().read(1, cursor).front() != END_LINE; --cursor.X) {}
		}
		Console::getInstance().setCursorPos(cursor);
		return *this;
	}
	inline auto &right() {
		auto cursor = Console::getInstance().getCursorPos();
		if (IsDBCSLeadByte(Console::getInstance().read(2, cursor).front())) {
			cursor.X += 2;
		}
		else if(cursor.X<getEndLine(cursor))++cursor.X;
		else{
			for (short i = 0; i < Console::getInstance().getScreenSize().X; ++i) {
				if (Console::getInstance().read(1, { i,static_cast<decltype(cursor.Y)>(1 + cursor.Y) }).front() == ' ')continue;
				cursor.X = 0;
				++cursor.Y;
				break;
			}
		}
		Console::getInstance().setCursorPos(cursor);
		return *this;
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
		if (Console::getInstance().read(screen.X, { 0,cursor.Y }).find_last_of(END_LINE) <=cursor.X) {
			screen.X = cursor.X;
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
		if (Console::getInstance().read(1, { static_cast<decltype(screen.X)>(screen.X - 1),cursor.Y }).front() == END_LINE) {
			++screen.X;
			Console::getInstance().setScrollSize(screen);
		}
		Console::getInstance().scroll(range, cursor);
		putchar(c);
	}
	const auto& getSelectBefore() {
		return before;
	}
	auto &reverseColor(COORD start,COORD end) {
		if (end.Y < start.Y || (start.Y == end.Y && end.X < start.X))std::swap(start,end);
		const auto reverse = [&start](const auto size) {
			for (auto& [color, size] : adjacent_split(Console::getInstance().readColor(size - start.X, start))) {
				Console::getInstance().setColor(~color, size, start);
				start.X += size;
			}
		};
		for (; start.Y < end.Y; ++start.Y) {
			reverse(getEndLine(start)+1 );
			start.X = 0;
		}
		start.Y = end.Y;
		reverse(end.X);
		return *this;
	}
	auto is_selecting()const{
		return base.X != -1;
	}
	auto &select() {
		if (is_selecting() && (before.X != Console::getInstance().getCursorPos().X || before.Y != Console::getInstance().getCursorPos().Y))ConsoleEditor::getInstance().reverseColor(before, Console::getInstance().getCursorPos());
		before = Console::getInstance().getCursorPos();
		if (!is_selecting())base = before;
		return *this;
	}
	const auto& getSelectStart()const {
		return base;
	}
	auto& cancelSelect() {
		reverseColor(base,before);
		base.X = -1;
		return*this;
	}
	auto deleteSelect() {
		if (before.Y < base.Y || (base.Y == before.Y && before.X < base.X))std::swap(base, before);
		Console::getInstance()
			.setCursorPos(base)
			.scroll(
				{
					before.X,
					before.Y,
					Console::getInstance().getScreenSize().X,
					before.Y
				},
				base
			)
			.scroll(
				{
					0,
					++before.Y,
					Console::getInstance().getScreenSize().X,
					Console::getInstance().getScreenSize().Y
				},
				{ 0,++base.Y }
		);
		base.X = -1;

	}
	auto getSelect() {
		auto start = base, end = before;
		if (end<start/*end.Y < start.Y || (start.Y == end.Y && end.X < start.X)*/) {
			using namespace std;
			swap(start, end);
		}
		std::string text;
		const auto reverse = [&start,&text](const auto size) {
			text+=std::move(Console::getInstance().read(size - start.X, start));
			text.back() = '\n';
		};
		for (; start.Y < end.Y; ++start.Y) {
			reverse(getEndLine(start) + 1);
			start.X = 0;
		}
		start.Y = end.Y;
		reverse(end.X);
		return std::move(text);
	}
	inline auto &write(const std::string&& text) {
		Split split(std::move(text), "\n");
		while (true) {
			insert(split.next());
			if (split.empty())break;
			enter();
		}
		return *this;
	}
};
class Command {
private:
	using FunctionT = std::function<bool(Split&)> ;
	const FunctionT not_found_callback;
	std::unordered_map<std::string, FunctionT > commands;
public:
	Command(decltype(not_found_callback) callback):not_found_callback(callback){}
	auto emplace(const decltype(commands)::key_type &command,const decltype(commands)::mapped_type &function) {
		commands.emplace(command,function);
	}
	auto excute(const std::string text) {
		Split data(std::move(text), " ");
		if (data.empty()|| !commands.contains(data.get()))return not_found_callback(data);//std::move
		return commands[data.get()]((data.next(),data));
	}
};
class Event {
private:
public:
	using eventType = decltype(INPUT_RECORD::Event)&;
	virtual bool excute(eventType) = 0;
};
class ConsoleInput {
private:
	std::unordered_map<DWORD, std::unique_ptr<Event> > callbacks;
public:
	template <class T,class... Args>
	auto emplace(decltype(callbacks)::key_type type,Args&&... args) {
		callbacks.emplace(type, std::make_unique<T>(std::forward<Args...>(args)...));
	}
#define LOOP_GENERATOR(ascii_or_wide) \
    auto loop##ascii_or_wide() {\
	INPUT_RECORD record;\
	DWORD length;\
	while (\
		!(\
			ReadConsoleInput##ascii_or_wide(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &length)\
			&&\
			callbacks.contains(record.EventType)\
			&& callbacks.at(record.EventType)->excute(record.Event)\
			)\
		) {\
	}\
	return;\
	}
	LOOP_GENERATOR(A)
	LOOP_GENERATOR(W)
#undef LOOP_GENERATOR
};
class TitleKeyEvent :public Event{
private:
	std::size_t pos=0;
public:
	TitleKeyEvent() {
		Console::getInstance().setTitle("|");
	}
	bool excute(eventType e)override {
		if (!e.KeyEvent.bKeyDown)return false;
		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_LEFT:
			if (pos) {
				using namespace std;
				auto title = Console::getInstance().getTitleW();
				swap(title[--pos],title[pos]);
				Console::getInstance().setTitleW(title);
			}
			break;
		case VK_RIGHT:
			{
				using namespace std;
				auto title = Console::getInstance().getTitleW();
				if (pos+1 >= title.size())break;
				swap(title[++pos], title[pos]);
				Console::getInstance().setTitleW(title);
			}
			break;
		case VK_BACK:
			if (!pos)break;
			Console::getInstance().setTitle(Console::getInstance().getTitle().erase(--pos,1));
			break;
		case VK_RETURN:
			Console::getInstance().setTitleW(Console::getInstance().getTitleW().erase(pos));
			return true;
			break;
		default:
			if (e.KeyEvent.uChar.UnicodeChar) {
				Console::getInstance().setTitleW(Console::getInstance().getTitleW().insert(pos, 1, e.KeyEvent.uChar.UnicodeChar));
				++pos;
			}
			break;
		}
		return false;
	}
};
class KeyEvent :public Event {
private:
	Command cmd;
public:
	KeyEvent():cmd([](Split& data){
		Console::getInstance().setTitle("not found:"+data.get());
		return false;
		}) {
		cmd.emplace("save", [](Split& data) {
			if (data.empty())return false;
			File file(data.get());
			file.write(ConsoleEditor::getInstance().readAll());
			return false;
			});
		cmd.emplace("open", [](Split& data) {
			if (data.empty())return false;
			ConsoleEditor::getInstance()
				.reset()
				.write(File(data.get()).read());
			return false;
			});
		cmd.emplace("exit", [](Split& data) {
			return true;
			});
	}
	bool excute(eventType e)override {
		if (!e.KeyEvent.bKeyDown)return false;
		const auto isControll = [&e](const decltype(e.KeyEvent.wVirtualKeyCode) key) {
			return e.KeyEvent.wVirtualKeyCode == key
				&&
				(
					e.KeyEvent.dwControlKeyState == LEFT_CTRL_PRESSED
					||
					e.KeyEvent.dwControlKeyState == RIGHT_CTRL_PRESSED
				);
		};
		if (isControll('C')) {
			Clipboard::getInstance().setString(ConsoleEditor::getInstance().getSelect());
			return false;
		}
		else if (isControll('V')) {
			if (ConsoleEditor::getInstance().is_selecting())ConsoleEditor::getInstance().deleteSelect();
			ConsoleEditor::getInstance().write(std::move(Clipboard::getInstance().getData()));
			return false;
		}
		const auto pressing_shift = e.KeyEvent.dwControlKeyState & SHIFT_PRESSED;
		if (pressing_shift && VK_LEFT <= e.KeyEvent.wVirtualKeyCode && e.KeyEvent.wVirtualKeyCode <= VK_DOWN) {
			ConsoleEditor::getInstance().select();
		}
		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_UP:
			if ((!pressing_shift && ConsoleEditor::getInstance().is_selecting())&& ConsoleEditor::getInstance().getSelectStart() < Console::getInstance().getCursorPos()) {
				Console::getInstance().setCursorPos(ConsoleEditor::getInstance().getSelectStart());
			}

			ConsoleEditor::getInstance().up();
			break;
		case VK_DOWN:
			if ((!pressing_shift && ConsoleEditor::getInstance().is_selecting()) && Console::getInstance().getCursorPos() < ConsoleEditor::getInstance().getSelectStart() ) {
				Console::getInstance().setCursorPos(ConsoleEditor::getInstance().getSelectStart());
			}
			ConsoleEditor::getInstance().down();
			break;
		case VK_LEFT:
			if (!pressing_shift && ConsoleEditor::getInstance().is_selecting()) {
				if (ConsoleEditor::getInstance().getSelectStart() < Console::getInstance().getCursorPos()) {
					Console::getInstance().setCursorPos(ConsoleEditor::getInstance().getSelectStart());
				}
				break;
			}
			ConsoleEditor::getInstance().left();
			break;
		case VK_RIGHT:
			if (!pressing_shift && ConsoleEditor::getInstance().is_selecting()) {
				if (Console::getInstance().getCursorPos() < ConsoleEditor::getInstance().getSelectStart() ) {
					Console::getInstance().setCursorPos(ConsoleEditor::getInstance().getSelectStart());
				}
				break;
			}
			ConsoleEditor::getInstance().right();
			break;
		case VK_ESCAPE:
		{
			ConsoleInput input;
			input.emplace<TitleKeyEvent>(KEY_EVENT);
			input.loopW();
			return cmd.excute(Console::getInstance().getTitle());
		}
		return false;
		case VK_RETURN:
			ConsoleEditor::getInstance().enter();
			return false;
		case VK_BACK:
			if (ConsoleEditor::getInstance().is_selecting())ConsoleEditor::getInstance().deleteSelect();
			else ConsoleEditor::getInstance().backspace();
			return false;
		default:
			if (e.KeyEvent.uChar.AsciiChar) {
				if (ConsoleEditor::getInstance().is_selecting())ConsoleEditor::getInstance().deleteSelect();
				ConsoleEditor::getInstance().insert(e.KeyEvent.uChar.AsciiChar);
			}
			return false;
		}
		if (pressing_shift)ConsoleEditor::getInstance().select();
		else if (ConsoleEditor::getInstance().is_selecting()) {
			ConsoleEditor::getInstance().cancelSelect();
		}
		return false;
	}
};
class MouseEvent :public Event {
private:
public:
	bool excute(eventType e)override {
		if (e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			ConsoleEditor::getInstance().move(e.MouseEvent.dwMousePosition);
			if (e.MouseEvent.dwEventFlags != MOUSE_MOVED && ConsoleEditor::getInstance().is_selecting())ConsoleEditor::getInstance().cancelSelect();
		}
		if (e.MouseEvent.dwEventFlags == MOUSE_MOVED && e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			ConsoleEditor::getInstance().select();
			//idea:other input class
		}
		return false;
	}
};
int main() {
	const auto mode = Console::getInstance().getMode();
	Console::getInstance()
		.setMode(ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT)
	    .setCodePage(CP_ACP);
	ConsoleInput input;
	input.emplace<KeyEvent>(KEY_EVENT);
	input.emplace<MouseEvent>(MOUSE_EVENT);
	ConsoleEditor::getInstance().reset();
	input.loopA();
	Console::getInstance().setMode(mode);
	return EXIT_SUCCESS;
}
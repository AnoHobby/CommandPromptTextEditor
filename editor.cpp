#include <conio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <Psapi.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <regex>
#include <fstream>
#include <string_view>
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

bool operator<(const COORD& a, const COORD& b) {
	return a.Y < b.Y || (b.Y == a.Y && a.X < b.X);
}
template <std::ranges::range T>
auto adjacent_split(const T range) {
	std::vector<std::pair<std::ranges::range_value_t<T>, std::size_t> > data;
	if (range.empty())return data;
	data.emplace_back(range.front(), 0);
	for (auto& i : range) {
		if (i == data.back().first) {
			++data.back().second;
			continue;
		}
		data.emplace_back(i, 1);
	}
	return std::move(data);
}
inline auto replace(std::string& str, const std::string&& before, const std::string&& after) {
	for (auto pos = str.find(before); pos != std::string::npos; pos = str.find(before, pos)) {
		str.replace(pos, before.size(), after);
	}
}
inline auto&& replace(std::string&& str, const std::string&& before, const std::string&& after) {
	for (auto pos = str.find(before); pos != std::string::npos; pos = str.find(before, pos)) {
		str.replace(pos, before.size(), after);
	}
	return std::move(str);
}
class Clipboard {
private:
	Singleton(Clipboard);
public:
	auto setString(std::string text) {
		text.resize(text.size() + 1);
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
		const auto data = GetClipboardData(CF_TEXT);
		CloseClipboard();
		return static_cast<const char*>(data);
	}
};
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
	template <class F>
	auto read(F&& lambda) {
		std::fstream file(name.c_str());
		if (!file)return;
		std::for_each(std::istreambuf_iterator<decltype(name)::value_type >(file), std::istreambuf_iterator<decltype(name)::value_type>(), lambda);
	}
	template <class F>
	auto write(F&& lambda) {
		std::ofstream file(name.c_str());
		if (!file)return false;
		while (1) {
			const auto c = lambda();
			if (c == '\0')break;
			file << c;
		}
	}
};
template <class T>
class Handle {
private:
	const T handle;
public:
	Handle(T&& handle) :handle(handle) {

	}
	~Handle() {
		CloseHandle(handle);
	}
	const auto& get() const {
		return handle;
	}
};

namespace Console {
	constexpr auto FILLER_CHARACTER = ' ';
	inline auto scroll(SMALL_RECT range, const COORD pos)noexcept(ScrollConsoleScreenBuffer) {
		CHAR_INFO info;
		info.Attributes = 0;
		info.Char.AsciiChar = FILLER_CHARACTER;
		ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE), &range, nullptr, pos, &info);
	}
	inline auto fileName() {
		char buffer[MAX_PATH];
		DWORD id;
		GetWindowThreadProcessId(GetConsoleWindow(), &id);
		Handle handle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, id));
		std::string path(buffer, GetModuleFileNameExA(handle.get(), nullptr, buffer, MAX_PATH));
		return std::move(path);
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
	inline auto setCursorPos(COORD pos) {
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
	}
	inline auto setCodePage(decltype(CP_ACP) codePage) {
		SetConsoleCP(codePage);
		SetConsoleOutputCP(codePage);
	}
	inline auto getCursorPos() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return info.dwCursorPosition;
	}
	inline auto getBufferSize() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return info.dwSize;
	}
	inline auto getScreen() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return info.srWindow;
	}
	inline auto getScreenSize() {
		auto info = getScreen();
		info.Right -= info.Left;
		info.Bottom -= info.Top;
		return COORD{ info.Right,info.Bottom };
	}
	inline auto getWindow() {
		RECT window;
		GetWindowRect(GetConsoleWindow(), &window);
		return window;
	}
	inline auto getWindowSize() {
		auto window = getWindow();
		window.right -= window.left;
		window.bottom -= window.right;
		return SIZE{ window.right, window.bottom };
	}
	inline auto setScreenSize(const COORD size) {
		const SMALL_RECT windowSize = { 0, 0, size.X, size.Y };
		SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), 1, &windowSize);
	}

	inline auto getDefaultColor() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return info.wAttributes;
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
	inline auto setTitle(const std::string title) {
		SetConsoleTitleA(title.c_str());
	}

	inline auto setTitleW(const std::wstring title) {
		SetConsoleTitleW(title.c_str());
	}
	inline auto setScrollSize(const COORD size) {
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);
	}
	inline auto setColor(const WORD color, DWORD length, const COORD pos) {
		FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color, length, pos, &length);
	}
	inline auto getMode() {
		DWORD mode;
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
		return mode;
	}
	inline auto setMode(const DWORD mode) {
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
	}
	namespace Editor {
		static constexpr auto TAB = "    ";
		static constexpr auto END_LINE = 0x7f;
		static constexpr auto NOT_FOUND_END_LINE = -1;
		inline auto getEndLine(COORD pos) {
			for (pos.X = Console::getBufferSize().X; 0 <= pos.X && Console::read(1, pos).front() != END_LINE; --pos.X) {}
			return pos.X;
		}
		inline short getLineCount() {
			if (Console::getScreenSize().Y + 1 <= Console::getBufferSize().Y) {
				for (short y = 0;; ++y) {
					if ([&y] {
						for (short x = 0; x < Console::getBufferSize().X; ++x) {
							if (Console::read(1, { x,y }).front() != Console::FILLER_CHARACTER)return true;
						}
						return false;
						}())continue;
					return y - 1;
				}
			}
			return Console::getBufferSize().Y;
		}

		inline auto read(COORD start, COORD end) {
			std::string text;
			const auto reverse = [&start, &text](const auto size) {
				text += std::move(Console::read(size - start.X, start));
			};
			for (; start.Y < end.Y; ++start.Y) {
				reverse(Console::Editor::getEndLine(start) + 1);
				text.back() = '\n';
				start.X = 0;
			}
			start.Y = end.Y;
			reverse(end.X);
			return std::move(text);
		}
		inline auto expandScrollH(const short size) {
			auto screen = Console::getBufferSize();
			if (size < screen.X)return;
			screen.X = size + 1;
			Console::setScrollSize(screen);
		}
		inline auto fitScrollH(short y,short size) {
			auto screen = Console::getBufferSize();
			--screen.X;
			size= screen.X-size;
			if (Console::read(1, { size,y }).front() != Console::Editor::END_LINE)return;
			if (size < Console::getScreenSize().X)size =Console::getScreenSize().X;
			[&] {
				for (; size < screen.X; --screen.X) {
					for (y = getLineCount(); 0 <= y; --y) {
						if (Console::Editor::END_LINE == Console::read(1, { screen.X,y }).front())return;
					}
				}
			}();
			SendMessage(GetConsoleWindow(), WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, screen.X - Console::getScreenSize().X), 0);
			++screen.X;
			Console::setScrollSize(screen);
		}
		auto reset() {
			auto screen = Console::getBufferSize();
			Console::scroll({ 0,0,screen.X,screen.Y }, { 0,static_cast<decltype(screen.Y)>(-screen.Y) });
			Console::setCursorPos({ 0,0 });
			putchar(END_LINE);
			Console::setCursorPos({ 0,0 });
			screen = Console::getScreenSize();
			++screen.X;
			++screen.Y;
			Console::setScrollSize(screen);

		}
		auto enter() {
			auto cursor = Console::getCursorPos();
			auto screen = Console::getBufferSize();
			SMALL_RECT range;
			range.Left = 0;
			range.Top = ++cursor.Y;
			range.Right = screen.X;
			range.Bottom = screen.Y;
			Console::scroll(range, { 0,++cursor.Y });
			range.Left = cursor.X;
			cursor.X = 0;
			cursor.Y = range.Top;
			range.Bottom = --range.Top;
			Console::scroll(range, cursor);
			if (Console::read(screen.X, { 0,short(screen.Y - 1) }).find(END_LINE) != std::string::npos) {
				++screen.Y;
				Console::setScrollSize(screen);
			}
			if (cursor.Y == Console::getScreen().Bottom) {
				SendMessage(GetConsoleWindow(), WM_VSCROLL, SB_LINEDOWN, 0);
			}
			putchar(END_LINE);
			fitScrollH(range.Bottom, getEndLine(cursor));
			Console::setCursorPos(cursor);
		}


		inline auto move(COORD pos) {
			const auto endLine = getEndLine(pos);
			if (endLine < pos.X)pos.X = endLine;
			else if (IsDBCSLeadByte(Console::read(2, { static_cast<decltype(pos.X)>(pos.X - 1),pos.Y }).front())) {
				--pos.X;
			}
			Console::setCursorPos(pos);
		}

		inline auto up() {
			auto cursor = Console::getCursorPos();
			if (!cursor.Y)return;
			--cursor.Y;
			move(cursor);
		}
		inline auto down() {
			auto cursor = Console::getCursorPos();
			++cursor.Y;
			move(cursor);
			if (Console::getScreen().Bottom <= cursor.Y) {
				SendMessage(GetConsoleWindow(), WM_VSCROLL, SB_LINEDOWN, 0);
			}
		}
		auto left() {
			auto cursor = Console::getCursorPos();
			if (cursor.X) {
				cursor.X -= 1 + IsDBCSLeadByte(Console::read(2, { static_cast<decltype(cursor.X)>(cursor.X - 2),cursor.Y }).front());
			}
			else if (!cursor.Y)return;
			else {
				--cursor.Y;
				for (cursor.X = Console::getBufferSize().X; 0 <= cursor.X && Console::read(1, cursor).front() != END_LINE; --cursor.X) {}
			}
			Console::setCursorPos(cursor);
		}
		inline auto right() {
			auto cursor = Console::getCursorPos();
			if (IsDBCSLeadByte(Console::read(2, cursor).front())) {
				cursor.X += 2;
			}
			else if (cursor.X < getEndLine(cursor))++cursor.X;
			else {
				for (short i = 0; i < Console::getBufferSize().X; ++i) {
					if (Console::read(1, { i,static_cast<decltype(cursor.Y)>(1 + cursor.Y) }).front() == ' ')continue;
					cursor.X = 0;
					++cursor.Y;
					break;
				}
			}
			Console::setCursorPos(cursor);
			if (Console::getScreen().Right <= cursor.X) {
				SendMessage(GetConsoleWindow(), WM_HSCROLL, SB_LINEDOWN, 0);
			}
		}
		auto backspace() {
			auto cursor = Console::getCursorPos();
			auto screen = Console::getBufferSize();
			if (cursor.X) {
				SMALL_RECT range;
				range.Left = cursor.X--;
				range.Right = screen.X;
				range.Top = range.Bottom = cursor.Y;
				const auto is_multibyte = IsDBCSLeadByte(Console::read(2, { static_cast<decltype(cursor.X)>(cursor.X - 1),cursor.Y }).front());
				Console::scroll(range, { cursor.X -= is_multibyte,cursor.Y });
				Console::setCursorPos(cursor);
				fitScrollH(cursor.Y, 1 + is_multibyte);
				return;
			}
			if (!cursor.Y)return;
			SMALL_RECT range;
			range.Left = cursor.X;
			range.Top = cursor.Y;
			range.Right = screen.X;
			range.Bottom = cursor.Y;
			--cursor.Y;
			cursor.X = Console::Editor::getEndLine(cursor);
			const auto endLine = cursor.X + Console::Editor::getEndLine({ 0,range.Top });
			if (screen.X <= endLine) {
				screen.X = endLine + 1;
				Console::setScrollSize(screen);
			}
			Console::setCursorPos(cursor);
			Console::scroll(range, cursor);
			++range.Top;
			range.Bottom = screen.Y;
			Console::scroll(range, { 0,static_cast<decltype(range.Top)>(range.Top - 1) });
			SendMessage(GetConsoleWindow(), WM_VSCROLL, SB_LINEUP, 0);
			--screen.Y;
			Console::setScrollSize(screen);
		}
		void insert(std::string str) {
			replace(str, "\t", TAB);
			auto cursor = Console::getCursorPos();
			auto screen = Console::getBufferSize();
			SMALL_RECT range;
			range.Left = cursor.X;
			range.Right = screen.X;
			range.Top = range.Bottom = cursor.Y;
			cursor.X += str.size();
			Console::Editor::expandScrollH(Console::Editor::getEndLine(cursor) + str.size());
			Console::scroll(range, cursor);
			if (Console::getScreen().Right < cursor.X) {
				SendMessage(GetConsoleWindow(), WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, cursor.X - Console::getScreenSize().X), 0);
			}
			std::cout << str;
		}
		void insert(const char c) {
			auto cursor = Console::getCursorPos();
			auto screen = Console::getBufferSize();
			SMALL_RECT range;
			range.Left = cursor.X++;
			range.Right = screen.X;
			range.Top = range.Bottom = cursor.Y;
			if (Console::read(1, { static_cast<decltype(screen.X)>(screen.X - 1),cursor.Y }).front() == END_LINE) {
				++screen.X;
				Console::setScrollSize(screen);
			}
			if (Console::getScreen().Right <= cursor.X) {
				SendMessage(GetConsoleWindow(), WM_HSCROLL, SB_LINEDOWN, 0);
			}
			Console::scroll(range, cursor);
			putchar(c);
		}
		auto reverseColor(COORD start, COORD end) {
			if (end.Y < start.Y || (start.Y == end.Y && end.X < start.X))std::swap(start, end);
			const auto reverse = [&start](const auto size) {
				for (auto& [color, size] : adjacent_split(Console::readColor(size - start.X, start))) {
					Console::setColor(~color, size, start);
					start.X += size;
				}
			};
			for (; start.Y < end.Y; ++start.Y) {
				reverse(getEndLine(start) + 1);
				start.X = 0;
			}
			start.Y = end.Y;
			reverse(end.X);
		}
		inline auto write(const std::string&& text) {
			Split split(std::move(text), "\n");
			while (true) {
				insert(split.next());
				if (split.empty())break;
				enter();
			}
		}
	};
	class Select {
	private:
		Singleton(Select);
		static constexpr auto NOT_SELECTED = -1;
		COORD base = { NOT_SELECTED }, before;
	public:
		auto is_selecting()const {
			return base.X != NOT_SELECTED;
		}
		auto& select(decltype(base) start, decltype(before) end) {
			base = start;
			before = end;
			return *this;
		}
		auto& select() {
			if (is_selecting() && (before.X != Console::getCursorPos().X || before.Y != Console::getCursorPos().Y))Console::Editor::reverseColor(before, Console::getCursorPos());
			before = Console::getCursorPos();
			if (!is_selecting())base = before;
			return *this;
		}
		const auto& getStart()const {
			return base;
		}
		auto& cancel() {
			Console::Editor::reverseColor(base, before);
			base.X = NOT_SELECTED;
			return*this;
		}
		auto remove() {
			if (before.Y < base.Y || (base.Y == before.Y && before.X < base.X))std::swap(base, before);
			const auto endLine = Console::Editor::getEndLine(base);
			const auto lineCount = Console::Editor::getLineCount();
			if(before.Y!=base.Y)Console::Editor::expandScrollH(endLine + Console::Editor::getEndLine(before) - before.X);
			const auto screen = Console::getBufferSize();
			Console::setCursorPos(base);
			Console::scroll(
				{
					before.X,
					before.Y,
				    screen.X,
					before.Y
				},
				base
			);
			Console::scroll(
				{
					0,
					static_cast<short>(before.Y+1),
					screen.X,
					screen.Y
				},
				{ 0,static_cast<short>(1+base.Y)}
			);
			Console::Editor::fitScrollH(base.Y, (base.Y == before.Y ? before.X : endLine) - base.X);
			if (screen.X - endLine < endLine - Console::Editor::getEndLine(base)) {
				Console::scroll(
					{
						static_cast<short>(base.X+screen.X - endLine),
						base.Y,
						before.X,
						base.Y
					},
					{0,-1}
				);
			}
			if (screen.Y - (before.Y - base.Y)<=Console::getScreen().Bottom) {
				SendMessage(
					GetConsoleWindow(),
					WM_VSCROLL,
					MAKEWPARAM(SB_THUMBPOSITION, screen.Y - (before.Y - base.Y) - Console::getScreenSize().Y - 1)
					, 0);
			}
			Console::setScrollSize({ Console::getBufferSize().X,static_cast<short>(screen.Y - (before.Y - base.Y)) });
			
			if (screen.Y - lineCount < lineCount - Console::Editor::getLineCount()) {
				Console::scroll(
					{
						0,
						static_cast<short>(Console::Editor::getLineCount()+screen.Y - lineCount),
						Console::getBufferSize().X,
						Console::getBufferSize().Y
					},
					Console::getBufferSize()
				);
			}
			base.X = NOT_SELECTED;

		}
		auto getSelection() {
			if (before < base) {
				return std::move(Console::Editor::read(before, base));
			}
			return std::move(Console::Editor::read(base, before));
		}
	};
};

class Event {
private:
public:
	using eventType = decltype(INPUT_RECORD::Event)&;
	virtual bool excute(eventType) = 0;
	virtual ~Event() {};
};
class ConsoleInput {
private:
	std::unordered_map<DWORD, std::unique_ptr<Event> > callbacks;
public:
	template <class T, class... Args>
	auto& emplace(decltype(callbacks)::key_type type, Args&&... args) {
		callbacks.emplace(type, std::make_unique<T>(std::forward<Args...>(args)...));
		return *this;
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
class ResizeEvent :public Event {
private:
	COORD size;
	SIZE windowSize;
public:
	ResizeEvent() :size(Console::getBufferSize()), windowSize(Console::getWindowSize()) {
	}
	bool excute(eventType e)override {
		const auto windowSize = Console::getWindowSize();
		if (windowSize.cx != this->windowSize.cx || windowSize.cy != this->windowSize.cy) {
			Console::setScrollSize(size);
			this->windowSize = windowSize;
		}
		else size = e.WindowBufferSizeEvent.dwSize;
		return false;
	}
};
class ScrollEvent :public Event {
private:
public:
	bool excute(eventType e)override {
		switch (e.MouseEvent.dwEventFlags) {
		case MOUSE_WHEELED:
			SendMessage(GetConsoleWindow(), WM_VSCROLL, e.MouseEvent.dwButtonState & 0x80000000 ? SB_LINEDOWN : SB_LINEUP, 0);
			break;
		case MOUSE_HWHEELED:
			SendMessage(GetConsoleWindow(), WM_HSCROLL, e.MouseEvent.dwButtonState & 0x80000000 ? SB_LINEUP : SB_LINEDOWN, 0);
			break;
		}
		return false;
	}
};
class MouseEvent :public Event {
private:
public:
	bool excute(eventType e)override {
		if (e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			Console::Editor::move(e.MouseEvent.dwMousePosition);
			if (e.MouseEvent.dwEventFlags != MOUSE_MOVED && Console::Select::getInstance().is_selecting())Console::Select::getInstance().cancel();
		}
		if (e.MouseEvent.dwEventFlags == MOUSE_MOVED && e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			Console::Select::getInstance().select();
		}
		ScrollEvent().excute(e);
		return false;
	}
};
class TitleKeyEvent :public Event {
private:
	std::size_t pos = 0;
public:
	TitleKeyEvent() {
		Console::setTitle("|");
	}
	bool excute(eventType e)override {
		if (!e.KeyEvent.bKeyDown)return false;
		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_LEFT:
			if (pos) {
				using namespace std;
				auto title = Console::getTitleW();
				swap(title[--pos], title[pos]);
				Console::setTitleW(title);
			}
			break;
		case VK_RIGHT:
		{
			using namespace std;
			auto title = Console::getTitleW();
			if (pos + 1 >= title.size())break;
			swap(title[++pos], title[pos]);
			Console::setTitleW(title);
		}
		break;
		case VK_BACK:
			if (!pos)break;
			Console::setTitle(Console::getTitle().erase(--pos, 1));
			break;
		case VK_RETURN:
			Console::setTitleW(Console::getTitleW().erase(pos));
			return true;
			break;
		default:
			if (e.KeyEvent.uChar.UnicodeChar) {
				Console::setTitleW(Console::getTitleW().insert(pos, 1, e.KeyEvent.uChar.UnicodeChar));
				++pos;
			}
			break;
		}
		return false;
	}
};
class FindMouseEvent :public Event {
private:
public:
	bool excute(eventType e)override {
		if (e.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
			MouseEvent().excute(e);
			return true;
		}
		ScrollEvent().excute(e);
		return false;
	}
};
class KeyEvent :public Event {
private:
public:
	KeyEvent() {

	}
	bool excute(eventType e)override;
};
class FindEvent :public Event {
private:
	static constexpr auto NOT_FOUND_MESSAGE = "";
	static constexpr auto LINE = -1;
	COORD offset;
	short backOffset = LINE;
	auto addOffset(COORD pos)const {
		if (offset.Y) {
			pos.X = offset.X;
			pos.Y += offset.Y;
		}
		else {
			pos.X += offset.X;
		}
		return pos;
	}
	inline auto move(const COORD& pos) const {
		Console::setCursorPos(pos);
		Console::Select::getInstance().select(pos, addOffset(pos));
	}
public:
	~FindEvent() {
		if (Console::getTitle() == NOT_FOUND_MESSAGE) return;
		COORD pos{ 0,0 };
		for (
			auto endLine = Console::Editor::getEndLine(pos);
			Console::Editor::NOT_FOUND_END_LINE != endLine;
			endLine = Console::Editor::getEndLine(pos)
			) {
			if ([&] {
				for (; pos.X < endLine; ) {
					if (Console::readColor(1, pos).front() == Console::getDefaultColor()) {
						++pos.X;
							continue;
					}
					if (!(pos.Y == Console::getCursorPos().Y && pos.X == Console::getCursorPos().X && Console::Select::getInstance().is_selecting()))Console::Editor::reverseColor(pos, offset.Y ? COORD{offset.X, static_cast<short>(pos.Y + offset.Y)} : COORD{ static_cast<short>(pos.X + offset.X),pos.Y });
						if (offset.Y) {
							pos.X = offset.X;
								pos.Y += offset.Y;
								return true;
						}
					pos.X += offset.X;
				}
				return false;
				}())continue;
			pos.X = 0;
			++pos.Y;
		}
	}
	FindEvent(std::string&& findText) :offset{ static_cast<short>(findText.size()),0 } {
		for (auto pos = findText.find('\\'); std::string::npos != pos; pos = findText.find('\\', pos + 1)) {
			if (*(findText.cbegin() + pos + 1) == '\\') {
				findText.erase(findText.cbegin() + pos);
			}
			else if (*(findText.cbegin() + pos + 1) == 'n') {
				findText.erase(findText.cbegin() + pos);
				findText[pos] = '\n';
				offset.X = findText.size() - pos - 1;
				++offset.Y;
				if (backOffset == LINE)backOffset = pos;
			}
		}
		COORD pos{ 0,0 };
		for (auto endLine = Console::Editor::getEndLine(pos); Console::Editor::NOT_FOUND_END_LINE != endLine; endLine = Console::Editor::getEndLine(pos)) {
			for (; pos.X < endLine;) {
				if (Console::read(1, pos).front() != findText.front()) {
					++pos.X;
					continue;
				}
				const auto end = addOffset(pos);
				if (Console::Editor::read(pos, end) == findText) {
					Console::Editor::reverseColor(pos, end);
					if (Console::readColor(1, Console::getCursorPos()).front() == Console::getDefaultColor()) {
						move(pos);
					}
					if (offset.Y)break;
					pos.X += offset.X;
				}
				else {
					++pos.X;
				}
			}
			if (offset.Y) {
				pos.Y += offset.Y;
				continue;
			}
			pos.X = 0;
			++pos.Y;
		}
		if (Console::readColor(1, Console::getCursorPos()).front() == Console::getDefaultColor()) {
			Console::setTitle(NOT_FOUND_MESSAGE);
		}
	}
	bool excute(eventType e) override {
		if (Console::getTitle() == NOT_FOUND_MESSAGE) return true;
		if (!e.KeyEvent.bKeyDown)return false;
		if (e.KeyEvent.dwControlKeyState & SHIFT_PRESSED && e.KeyEvent.wVirtualKeyCode == VK_RETURN) {
			e.KeyEvent.wVirtualKeyCode = VK_UP;
		}
		switch (e.KeyEvent.wVirtualKeyCode) {
		case VK_SHIFT:
			break;
		case VK_RIGHT:
			if (offset.Y) {
				Console::setCursorPos({ offset.X,static_cast<short>(offset.Y + Console::getCursorPos().Y) });
			}
			else {
				auto pos = Console::getCursorPos();
				pos.X += offset.X - 1;
				Console::setCursorPos(pos);
			}
		case VK_LEFT:
		case VK_ESCAPE:
			Console::Select::getInstance().cancel();
			return true;
			break;
		case VK_UP:
		{
			auto pos = Console::getCursorPos();
			while (1) {
				const auto colors = Console::readColor(pos.X, { 0,pos.Y });
				const auto next = std::find(colors.rbegin(), colors.rend(), Console::readColor(1, Console::getCursorPos()).front());
				if (next == colors.rend()) {
					pos.X = 0;
					if (!pos.Y) {
						for (; Console::Editor::NOT_FOUND_END_LINE != Console::Editor::getEndLine(pos); ++pos.Y);
					}
					--pos.Y;
					pos.X = Console::Editor::getEndLine(pos);
					continue;
				}
				else if (offset.Y) {
					pos.Y -= offset.Y;
					pos.X = Console::Editor::getEndLine(pos) - backOffset;
				}
				else pos.X -= offset.X;
				move(pos);
				break;
			}
		}
		break;
		case VK_DOWN:
		case VK_RETURN:
		{
			auto pos = addOffset(Console::getCursorPos());
			while (1) {
				const auto endLine = Console::Editor::getEndLine(pos);
				if (Console::Editor::NOT_FOUND_END_LINE == endLine) {
					pos.Y = pos.X = 0;
					continue;
				}
				const auto colors = Console::readColor(endLine - pos.X, pos);
				const auto next = std::ranges::find(colors, Console::readColor(1, Console::getCursorPos()).front());
				if (next == colors.end()) {
					pos.X = 0;
					++pos.Y;
					continue;
				}
				pos.X += std::distance(colors.begin(), next);
				move(pos);
				break;
			}
		}
		break;
		default:
		{
			KeyEvent().excute(e);
			return true;
		}

		}
		return false;
	}
};
bool KeyEvent::excute(eventType e) {
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
		Clipboard::getInstance().setString(Console::Select::getInstance().getSelection());
		return false;
	}
	else if (isControll('V')) {
		if (Console::Select::getInstance().is_selecting())Console::Select::getInstance().remove();
		Console::Editor::write(std::move(Clipboard::getInstance().getData()));
		return false;
	}
	else if (isControll('F')) {
		ConsoleInput inputText;
		inputText.emplace<TitleKeyEvent>(KEY_EVENT);
		inputText.emplace<ResizeEvent>(WINDOW_BUFFER_SIZE_EVENT);
		inputText.loopW();
		if (Console::getTitle().empty())return false;
		ConsoleInput findMode;
		findMode.emplace<ResizeEvent>(WINDOW_BUFFER_SIZE_EVENT);
		findMode
			.emplace<FindEvent>(KEY_EVENT, Console::getTitle())
			.emplace<FindMouseEvent>(MOUSE_EVENT)
			.loopA();
			return false;
	}
	const auto pressing_shift = e.KeyEvent.dwControlKeyState & SHIFT_PRESSED;
	if (pressing_shift && VK_LEFT <= e.KeyEvent.wVirtualKeyCode && e.KeyEvent.wVirtualKeyCode <= VK_DOWN) {
		Console::Select::getInstance().select();
	}
	switch (e.KeyEvent.wVirtualKeyCode) {
	case VK_UP:
		if ((!pressing_shift && Console::Select::getInstance().is_selecting()) && Console::Select::getInstance().getStart() < Console::getCursorPos()) {
			Console::setCursorPos(Console::Select::getInstance().getStart());
		}

		Console::Editor::up();
		break;
	case VK_DOWN:
		if ((!pressing_shift && Console::Select::getInstance().is_selecting()) && Console::getCursorPos() < Console::Select::getInstance().getStart()) {
			Console::setCursorPos(Console::Select::getInstance().getStart());
		}
		Console::Editor::down();
		break;
	case VK_LEFT:
		if (!pressing_shift && Console::Select::getInstance().is_selecting()) {
			if (Console::Select::getInstance().getStart() < Console::getCursorPos()) {
				Console::setCursorPos(Console::Select::getInstance().getStart());
			}
			break;
		}
		Console::Editor::left();
		break;
	case VK_RIGHT:
		if (!pressing_shift && Console::Select::getInstance().is_selecting()) {
			if (Console::getCursorPos() < Console::Select::getInstance().getStart()) {
				Console::setCursorPos(Console::Select::getInstance().getStart());
			}
			break;
		}
		Console::Editor::right();
		break;
	case VK_ESCAPE:
	{
		ConsoleInput input;
		input.emplace<TitleKeyEvent>(KEY_EVENT);
		input.emplace<ResizeEvent>(WINDOW_BUFFER_SIZE_EVENT);
		input.loopW();
		const auto  text = Console::getTitle();
		if (text == "exit")return true;
		if (text.size() < 5) {
			return false;
		}
		const std::string_view command{text.c_str(), 4};
		if (command == "save") {
			File file(text.substr(5));
			COORD pos{ 0,0 };
			file.write([&pos]()mutable {
				const auto c = Console::read(1, pos).front();
				if (c != Console::Editor::END_LINE) {
					++pos.X;
					return c;
				}
				if (Console::Editor::getLineCount() < ++pos.Y)return '\0';
				pos.X = 0;
				return '\n';
				});
		}
		else if (command == "open") {
			Console::Editor::reset();
			File(text.substr(5)).read([](const char& s) {
				if (s == '\n')Console::Editor::enter();
				else if (s == '\t')Console::Editor::insert("\t");
				else Console::Editor::insert(s);
				});
			Console::setCursorPos({ 0,0 });
		}
	}
	return false;
	case VK_RETURN:
		if (Console::Select::getInstance().is_selecting())Console::Select::getInstance().remove();
		Console::Editor::enter();
		return false;
	case VK_BACK:
		if (Console::Select::getInstance().is_selecting())Console::Select::getInstance().remove();
		else Console::Editor::backspace();
		return false;
	case VK_TAB:
		Console::Editor::insert("\t");
		return false;
	default:
		if (e.KeyEvent.uChar.AsciiChar) {
			if (Console::Select::getInstance().is_selecting())Console::Select::getInstance().remove();
			Console::Editor::insert(e.KeyEvent.uChar.AsciiChar);
		}
		return false;
	}
	if (pressing_shift)Console::Select::getInstance().select();
	else if (Console::Select::getInstance().is_selecting()) {
		Console::Select::getInstance().cancel();
	}
	return false;
}

int main(int argc,char *argv[]) {
	DWORD value, size = sizeof(value);
	RegGetValueA(
		HKEY_CURRENT_USER,
		std::string("Console\\" + replace(Console::fileName(), "\\", "_")).c_str(),
		"LineWrap",
		RRF_RT_REG_DWORD,
		nullptr,
		&value,
		&size);
	if (value) {
		//あった方が確実、なくても良い。
		//あった場合、自動操作が失敗しても再起動時に値が変わっている
		/*HKEY key;
		value = 0;
		RegOpenKeyExA(
			HKEY_CURRENT_USER,
			std::string("Console\\" + replace(Console::fileName(), "\\", "_")).c_str(),
			0,
			KEY_SET_VALUE,
			&key
			);
		RegSetValueExA(
			key,
			"LineWrap",
			0,
			REG_DWORD,
			(LPBYTE)&value,
			sizeof(size)
		);
		RegCloseKey(key);*/
		PostMessage(
			GetConsoleWindow(),
			WM_SYSCOMMAND,
			GetMenuItemID(GetSystemMenu(GetConsoleWindow(), 0), GetMenuItemCount(GetSystemMenu(GetConsoleWindow(), 0)) - 1),
			0);
		while (GetConsoleWindow() == GetForegroundWindow());
		PropSheet_SetCurSel(GetForegroundWindow(), PropSheet_IndexToHwnd(GetForegroundWindow(), 2), 2);
		while (Button_GetCheck(FindWindowEx(PropSheet_IndexToHwnd(GetForegroundWindow(), 2), FindWindowEx(PropSheet_IndexToHwnd(GetForegroundWindow(), 2), nullptr, TEXT("Button"), nullptr), TEXT("Button"), nullptr)) == BST_CHECKED) {
			PostMessage(
				FindWindowEx(PropSheet_IndexToHwnd(GetForegroundWindow(), 2), FindWindowEx(PropSheet_IndexToHwnd(GetForegroundWindow(), 2), nullptr, TEXT("Button"), nullptr), TEXT("Button"), nullptr)
				, WM_KEYDOWN, 'R', 0);
		}
		SendMessage(
			GetForegroundWindow(),
			WM_COMMAND,
			MAKEWPARAM(GetDlgCtrlID(FindWindowEx(GetForegroundWindow(), nullptr, TEXT("Button"), TEXT("OK"))), BN_CLICKED),
			(LPARAM)FindWindowEx(GetForegroundWindow(), nullptr, TEXT("Button"), TEXT("OK"))
		);
	}
	const auto mode = Console::getMode();
	Console::setMode(ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
	Console::setCodePage(CP_ACP);
	Console::Editor::reset();
	ConsoleInput input;
	input.emplace<KeyEvent>(KEY_EVENT);
	input.emplace<MouseEvent>(MOUSE_EVENT);
	input.emplace<ResizeEvent>(WINDOW_BUFFER_SIZE_EVENT);
	input.loopA();
	Console::setMode(mode);
	Console::Editor::reset();
	return EXIT_SUCCESS;
}
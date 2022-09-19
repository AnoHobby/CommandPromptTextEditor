#include <conio.h>
#include <iostream>
#include <windows.h>
#include <string>
#include <algorithm>
#include <functional>
#include <unordered_map>
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
template <class T>
struct Point {
	T x, y;
};
template <class T>
struct Rect {
	Point<T> pos,size;
	//T width, height;
};
class Console final {
private:
	Singleton(Console);
public:
	inline auto& scroll(const Rect<decltype(SMALL_RECT::Left)> range, const Point<decltype(COORD::X)> pos) noexcept(ScrollConsoleScreenBuffer) {
		CHAR_INFO info;
		SMALL_RECT _range;
		info.Attributes = 0;
		info.Char.AsciiChar = ' ';
		_range.Left = range.pos.x;
		_range.Right = range.size.x;
		//_range.Right = range.width;
		_range.Top = range.pos.y;
		//_range.Bottom = range.height;
		_range.Bottom = range.size.y;
		ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE), &_range, nullptr, { pos.x,pos.y }, &info);
		return *this;
	}
	inline auto read(const std::size_t size, const Point<decltype(COORD::X)> pos) {
		std::string content;
		DWORD length;
		content.resize(size);
		ReadConsoleOutputCharacterA(GetStdHandle(STD_OUTPUT_HANDLE), content.data(), content.size(), { pos.x,pos.y }, &length);
		return std::move(content);
	}
	inline auto& setCursorPos(Point<short> pos) {
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { pos.x,pos.y });
		return *this;
	}
	inline auto& setCodePage(decltype(CP_ACP) codePage) {
		SetConsoleCP(codePage);
		return *this;
	}
	inline auto getCursorPos() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return std::move(Point{ info.dwCursorPosition.X,info.dwCursorPosition.Y });
	}
	inline auto getScreenSize() {//ï™ÇØÇÈÇ∆ìÒâÒåƒÇ—èoÇµÇƒÇ¢ÇÈÇÃÇ≈å¯ó¶Ç™à´Ç¢
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return std::move(Point{ info.dwSize.X,info.dwSize.Y });
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
};
template <class T>
class KeyInput {
private:
	T proccessor;
public:
	auto &input() {
		while (true) {
			auto c = _getch();
			switch (c) {
			case 0x1b:
				if(!proccessor.esc())return proccessor;
				continue;
			case 0x0d:
				if(!proccessor.enter())return proccessor;
				continue;
			case 8://backspace
				proccessor.backspace();
				continue;
			}
			if (224 != c) {
				proccessor.insert(c);
				continue;
			}
			proccessor.arrow(_getch());

		}
	}
};
class CmdLine {
private:
	inline static constexpr auto CURSOR = '|';
	std::size_t pos;//KeyInputÇ…ìnÇ∑ÇΩÇﬂÇ…
public:
	CmdLine() {
		Console::getInstance().setTitle(std::string(1, CURSOR));
	}
	auto esc() {
		return false;
	}
	auto enter() {
		return false;
	}
	auto backspace() {
		if (pos <= 0)return;
		auto text = Console::getInstance().getTitle();
		const auto isMultiByte =--pos>0?IsDBCSLeadByte(text[pos - 1]):false;
		text.erase(pos-=isMultiByte, 1+isMultiByte);
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
	auto arrow(const char c) {
		switch (c) {
		case 0x4b:
		{
			if (pos <= 0)return;
			auto text = Console::getInstance().getTitle();
			text.erase(pos--, 1);
			text.insert(pos -= pos > 0 ? IsDBCSLeadByte(text[pos - 1]) : false, 1, CURSOR);
			Console::getInstance().setTitle(text);
		}
		break;;
		case 0x4d:
		{
			auto text = Console::getInstance().getTitle();
			if (text.size() - 1 <= pos)return;
			text.erase(pos, 1);
			text.insert(pos += IsDBCSLeadByte(text[pos++]), 1, CURSOR);
			Console::getInstance().setTitle(text);
		}
		break;
		}
	}
	auto getText() {
		auto cmd = Console::getInstance().getTitle();
		cmd.erase(pos,1);
		//Console::getInstance().setTitle(Console::getInstance().getDefaultTitle());//L""
		return std::move(cmd);
	}
	//std::string proccess() {
	//	std::size_t pos=0;
	//	while (true) {
	//		auto c = _getch();
	//		switch (c) {
	//		case 0x1b:
	//			return "";
	//			break;
	//		case 0x0d:
	//		{
	//			auto cmd = Console::getInstance().getTitle();
	//			cmd.erase(pos,1);//next begin, pos
	//			return std::move(cmd);
	//		}
	//		case 8://backspace
	//		{

	//		}
	//			continue;
	//		}
	//		if (c != 224) {
	//			auto text=Console::getInstance().getTitle();
	//			text.erase(pos,1);//CURSOR.size()
	//			std::string add(1,c);
	//			if (IsDBCSLeadByte(c))add.push_back(_getch());
	//			add.push_back(CURSOR);
	//			text.insert(pos,add);
	//			pos += add.size()-1;
	//			//text.insert(pos,c,1);
	//			Console::getInstance().setTitle(text);
	//			continue;
	//		}
	//		switch (_getch()) {
	//		case 0x4b:
	//		{
	//			if (pos <= 0)continue;
	//			auto text = Console::getInstance().getTitle();
	//			text.erase(pos--,1);
	//			text.insert(pos-=pos>0?IsDBCSLeadByte(text[pos-1]) : false, 1, CURSOR);
	//			Console::getInstance().setTitle(text);
	//		}
	//			continue;
	//		case 0x4d:
	//		{
	//			auto text = Console::getInstance().getTitle();
	//			if (text.size()-1 <= pos)continue;
	//			text.erase(pos, 1);
	//			text.insert(pos += IsDBCSLeadByte(text[pos++]), 1, CURSOR);
	//			Console::getInstance().setTitle(text);
	//		}
	//			continue;
	//		}
	//		
	//	}
	//	return "";
	//}
};
class TextEditor {
private:
	inline static constexpr auto END_LINE = '|';
	Point<short> cursor,screenSize;
public:
	inline auto esc() {
		return KeyInput<CmdLine>().input().getText() !="end";
	}
	inline auto enter() {
		Rect<short> range;
		range.pos = cursor;
		range.size = screenSize;
		cursor.x = 0;
		++cursor.y;
		Console::getInstance().scroll(range, cursor);
		putchar(END_LINE);
		Console::getInstance().setCursorPos(cursor);
		return true;
	}
	inline auto backspace() {
		if (!cursor.x) {
			Rect<short> range;
			range.size.x = screenSize.x;
			range.size.y = (range.pos = cursor).y;
			--cursor.y;
			auto content = Console::getInstance().read(screenSize.x, cursor);
			content = content.substr(0, content.find_last_of(END_LINE));
			cursor.x = content.size();
			Console::getInstance()
				.setCursorPos(cursor)
				.scroll(range,cursor);
			++range.pos.y;
			range.size.y = screenSize.y;
			Console::getInstance().scroll(range, { 0,static_cast<decltype(range.pos.y)>(range.pos.y - 1) });
			return;
		}
		Rect<short> range;
		range.pos.x = cursor.x--;
		range.size.x = screenSize.x;
		range.pos.y = range.size.y = cursor.y;
		Console::getInstance()
			.scroll(range, { cursor.x -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.x)>(cursor.x - 1),cursor.y }).front()),cursor.y })
			.setCursorPos(cursor);
	}
	inline auto insert(const char c) {
		Rect<decltype(SMALL_RECT::Left)> range;
		range.pos.x = cursor.x++;
		range.size.x = screenSize.x;
		range.pos.y = range.size.y = cursor.y;
		cursor.x += IsDBCSLeadByte(c);
		Console::getInstance().scroll(range, cursor);
		putchar(c);
		if (!IsDBCSLeadByte(c))return;
		putchar(_getch());
	}
	inline auto up() {
		if (cursor.y <= 0)return;
		cursor.x -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.x)>(cursor.x- 1),--cursor.y }).front());
		const auto endLine = Console::getInstance().read(screenSize.x, { 0,cursor.y }).find_last_of(END_LINE);
		if (endLine <= cursor.x) {
			cursor.x = endLine;
		}
		Console::getInstance().setCursorPos(cursor);
	}
	inline auto down() {
		cursor.x -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.x)>(cursor.x- 1),++cursor.y }).front());
		const auto endLine = Console::getInstance().read(screenSize.x, { 0,cursor.y }).find_last_of(END_LINE);
		if (endLine == std::string::npos) {
			--cursor.y;
		}
		else if (endLine <= cursor.x) {
			cursor.x = endLine;
		}
		Console::getInstance().setCursorPos(cursor);
	}
	inline auto left() {
		if (cursor.y > 0 && !cursor.x) {
			--cursor.y;
			cursor.x += Console::getInstance().read(screenSize.x - cursor.x, cursor).find_last_of(END_LINE);
		}
		else if (cursor.x <= 0)return;
		else {
			cursor.x -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(cursor.x)>(--cursor.x- 1),cursor.y }).front());
		}
		Console::getInstance().setCursorPos(cursor);
	}
	inline auto right() {
		const auto endLine = cursor.x + Console::getInstance().read(screenSize.x - cursor.x, cursor).find_last_of(END_LINE);
		if (endLine == cursor.x) {
			down();
		}
		else if (endLine <= cursor.x)return;
		else {
			++cursor.x+= IsDBCSLeadByte(Console::getInstance().read(2, cursor).front());
		}
		Console::getInstance().setCursorPos(cursor);
	}
	inline auto arrow(const int c) {
		switch (c) {
		case 0x48://up
			up();
			break;;
		case 0x50://down
			down();
			break;;
		case 0x4b:
			left();
			break;
		case 0x4d://right
			right();
			break;
		}
	}
public:
	TextEditor() {
		putchar(END_LINE);
		Console::getInstance().setCursorPos({0,0});
		cursor = Console::getInstance().getCursorPos();
		screenSize = Console::getInstance().getScreenSize();
	}
};

int main() {
	KeyInput<TextEditor> editor;
	editor.input();
	//TextEditor editor;
	//while (true) {
	//    auto c = _getch();
	//	switch (c) {
	//	case 0x1b:
	//		editor.esc();
	//		continue;
	//	case 0x0d:
	//		editor.enter();
	//	continue;
	//	case 8://backspace
	//		editor.backspace();
	//	continue;
	//	}
	//	if (224 != c) {
	//		editor.insert(c);
	//		continue;
	//	}
	//	switch (_getch()) {
	//	case 0x48://up
	//		editor.up();
	//		continue;
	//	case 0x50://down
	//		editor.down();
	//		continue;
	//	case 0x4b:
	//		editor.left();
	//		continue;
	//	case 0x4d://right
	//		editor.right();
	//		continue;
	//	}
	//}
	return EXIT_SUCCESS;
}

#include <conio.h>
#include <iostream>
#include <windows.h>
#include <string>
#include <algorithm>
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
	inline auto getScreenSize() {//分けると二回呼び出しているので効率が悪い
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		return std::move(Point{ info.dwSize.X,info.dwSize.Y });
	}
};

class TextEditor {
private:
	inline static constexpr auto END_LINE = '|';
	Point<short> cursor,screenSize;
public:
	inline auto enter() {
		Rect<short> range;
		range.pos = cursor;
		range.size = screenSize;
		cursor.x = 0;
		++cursor.y;
		Console::getInstance().scroll(range, cursor);
		putchar(END_LINE);
		Console::getInstance().setCursorPos(cursor);

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
	inline auto insert(char c) {
		Rect<decltype(SMALL_RECT::Left)> range;
		range.pos.x = cursor.x++;
		range.size.x = screenSize.x;
		range.pos.y = range.size.y = cursor.y;
		cursor.x += IsDBCSLeadByte(c);
		Console::getInstance().scroll(range, cursor);
		putchar(c);
		if (!IsDBCSLeadByte(c))return;//cinfo-curPos
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
		if (cursor.y > 0/*upを使えばいいが、こちらの方が効率がいい*/ && !cursor.x) {
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
public:
	TextEditor() {
		putchar(END_LINE);
		Console::getInstance().setCursorPos({0,0});
		cursor = Console::getInstance().getCursorPos();
		screenSize = Console::getInstance().getScreenSize();
	}
};

int main() {
	TextEditor editor;
	while (true) {
		char c = _getch();
		switch (c) {
		case 0x0d:
			editor.enter();
		continue;
		case 8://backspace
			editor.backspace();
		continue;
		}
		if (static_cast<decltype(c)>(224) != c) {
			editor.insert(c);
			continue;
		}
		switch (_getch()) {
		case 0x48://up
			editor.up();
			continue;
		case 0x50://down
			editor.down();
			continue;
		case 0x4b:
			editor.left();
			continue;
		case 0x4d://right
			editor.right();
			continue;
		}
	}
	//std::cout << "|";
	//SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0,0 });
	//SetConsoleCP(CP_ACP);
	//CONSOLE_SCREEN_BUFFER_INFO info;
	//GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
	//while (true) {
	//	char c = _getch();
	//	switch (c) {
	//	case 0x1b:
	//		//esc
	//		continue;
	//	case 0x0d://enter
	//	{
	//		Rect<short> range;
	//		range.pos.x = info.dwCursorPosition.X;
	//		range.width = info.dwSize.X;
	//		range.pos.y = info.dwCursorPosition.Y;
	//		range.height = info.dwSize.Y;
	//		Console::getInstance().scroll(range, { info.dwCursorPosition.X = 0,(++info.dwCursorPosition.Y) });
	//		putchar('|');
	//		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), info.dwCursorPosition);
	//	}
	//	continue;
	//	case 8://backspace
	//	{
	//		if (!info.dwCursorPosition.X) {
	//			Rect<short> range;
	//			range.pos.x = info.dwCursorPosition.X;
	//			range.width = info.dwSize.X;
	//			range.pos.y = range.height = info.dwCursorPosition.Y;
	//			auto content = Console::getInstance().read(info.dwSize.X, { info.dwCursorPosition.X,--info.dwCursorPosition.Y });
	//			content = content.substr(0, content.find_last_of('|'));
	//			info.dwCursorPosition.X = content.size();
	//			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), info.dwCursorPosition);
	//			Console::getInstance().scroll(range, { info.dwCursorPosition.X,info.dwCursorPosition.Y });
	//			++range.pos.y;
	//			range.height = info.dwSize.Y;
	//			Console::getInstance().scroll(range, { 0,static_cast<decltype(range.pos.y)>(range.pos.y - 1) });
	//			continue;
	//		}
	//		Rect<short> range;
	//		range.pos.x = info.dwCursorPosition.X--;
	//		range.width = info.dwSize.X;
	//		range.pos.y = range.height = info.dwCursorPosition.Y;
	//		Console::getInstance().scroll(range, { info.dwCursorPosition.X -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(info.dwCursorPosition.X)>(info.dwCursorPosition.X - 1),info.dwCursorPosition.Y }).front()),info.dwCursorPosition.Y });
	//		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), info.dwCursorPosition);
	//	}
	//	continue;
	//	}
	//	if (static_cast<decltype(c)>(224) != c) {
	//		Rect<decltype(SMALL_RECT::Left)> range;
	//		range.pos.x = info.dwCursorPosition.X++;
	//		range.width = info.dwSize.X;
	//		range.pos.y = range.height = info.dwCursorPosition.Y;
	//		Console::getInstance().scroll(range, { info.dwCursorPosition.X += IsDBCSLeadByte(c),info.dwCursorPosition.Y });
	//		putchar(c);
	//		if (!IsDBCSLeadByte(c))continue;//cinfo-curPos
	//		putchar(_getch());
	//		continue;
	//	}
	//	switch (_getch()) {
	//	case 0x48://up
	//		if (info.dwCursorPosition.Y <= 0)continue;
	//		info.dwCursorPosition.X -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(info.dwCursorPosition.X)>(info.dwCursorPosition.X - 1),--info.dwCursorPosition.Y }).front());
	//		{
	//			const auto endLine = Console::getInstance().read(info.dwSize.X, { 0,info.dwCursorPosition.Y }).find_last_of('|');
	//			if (!(endLine <= info.dwCursorPosition.X)) {
	//				break;
	//			}
	//			info.dwCursorPosition.X = endLine;
	//		}
	//		break;
	//	case 0x50://down
	//		info.dwCursorPosition.X -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(info.dwCursorPosition.X)>(info.dwCursorPosition.X - 1),++info.dwCursorPosition.Y }).front());
	//		{
	//			const auto endLine = Console::getInstance().read(info.dwSize.X, { 0,info.dwCursorPosition.Y }).find_last_of('|');
	//			if (endLine == std::string::npos) {
	//				--info.dwCursorPosition.Y;
	//				break;
	//			}
	//			else if (!(endLine <= info.dwCursorPosition.X)) {
	//				break;
	//			}
	//			info.dwCursorPosition.X = endLine;
	//		}
	//		break;//left
	//	case 0x4b:
	//		if (info.dwCursorPosition.Y > 0/*upを使えばいいが、こちらの方が効率がいい*/ && !info.dwCursorPosition.X) {
	//			--info.dwCursorPosition.Y;
	//			info.dwCursorPosition.X += Console::getInstance().read(info.dwSize.X - info.dwCursorPosition.X, { info.dwCursorPosition.X,info.dwCursorPosition.Y }).find_last_of('|');
	//			break;
	//		}
	//		else if (info.dwCursorPosition.X <= 0)continue;
	//		info.dwCursorPosition.X -= IsDBCSLeadByte(Console::getInstance().read(2, { static_cast<decltype(info.dwCursorPosition.X)>(--info.dwCursorPosition.X - 1),info.dwCursorPosition.Y }).front());
	//		break;
	//	case 0x4d://right
	//	{
	//		auto endLine = info.dwCursorPosition.X + Console::getInstance().read(info.dwSize.X - info.dwCursorPosition.X, { info.dwCursorPosition.X,info.dwCursorPosition.Y }).find_last_of('|');
	//		if (endLine == info.dwCursorPosition.X) {
	//			info.dwCursorPosition.X = 0;
	//			++info.dwCursorPosition.Y;//call down
	//			break;
	//		}
	//		else if (endLine <= info.dwCursorPosition.X)continue;//速度を気にするならば、行変更の時に変数に格納する
	//	}
	//	++info.dwCursorPosition.X += IsDBCSLeadByte(Console::getInstance().read(2, { info.dwCursorPosition.X,info.dwCursorPosition.Y }).front());
	//	break;
	//	default:
	//		continue;
	//	}
	//	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), info.dwCursorPosition);
	//}
	return EXIT_SUCCESS;
}

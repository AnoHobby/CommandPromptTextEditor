#include <conio.h>
#include <iostream>
#include <windows.h>
#include <string>
#include <algorithm>
#include <vector>
#include <functional>
#include <unordered_map>
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
template <class T>
struct Point {
	T x, y;
};
template <class T>
struct Rect {
	Point<T> pos,size;
};
auto split(std::string str,const std::string cut) {
	std::vector<decltype(str)> data;
	for (auto pos = str.find(cut); pos != std::string::npos; pos = str.find(cut)) {
		data.push_back(str.substr(0,pos));
		str = str.substr(pos+cut.size());
	}
	if (!str.empty())data.push_back(str);
	return std::move(data);
}
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
	inline auto& scroll(const Rect<decltype(SMALL_RECT::Left)> range, const Point<decltype(COORD::X)> pos) noexcept(ScrollConsoleScreenBuffer) {
		CHAR_INFO info;
		SMALL_RECT _range;
		info.Attributes = 0;
		info.Char.AsciiChar = ' ';
		_range.Left = range.pos.x;
		_range.Right = range.size.x;
		_range.Top = range.pos.y;
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
	inline auto setScrollSize(const Point<short> size)const {
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),{size.x,size.y});
	}
};
class Command {
private:
	std::unordered_map<std::string,std::function<bool(std::vector<std::string>)> > commands;
public:
	Command(decltype(commands) commands):commands(commands){}
	auto operator[](std::vector<std::string> arguments) {
		if (!commands.count(arguments.front()))return false;
		return commands[arguments.front()](arguments);
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
				if constexpr (requires(T instance) { instance.esc(); }) {
					if (!proccessor.esc())return proccessor;
				}
				continue;
			case 0x0d:
				if constexpr (requires(T instance) { instance.enter(); }) {
					if (!proccessor.enter())return proccessor;
				}
				continue;
			case 8://backspace
				if constexpr (requires(T instance) { instance.backspace(); }) {
					proccessor.backspace();
				}
				continue;
			}
			if (224 != c) {
				if constexpr (requires(T instance) { instance.insert(c); }) {
					proccessor.insert(c);//マルチバイト判断をこっちでやる
				}
				continue;
			}
			switch (_getch()) {
			case 0x48://up
				if constexpr (requires(T instance) { instance.up(); }) {
					proccessor.up();
				}
				break;;
			case 0x50://down
				if constexpr (requires(T instance) { instance.down(); }) {
					proccessor.down();
				}
				break;;
			case 0x4b:
				if constexpr (requires(T instance) { instance.left(); }) {
					proccessor.left();
				}
				break;
			case 0x4d://right
				if constexpr (requires(T instance) { instance.right(); }) {
					proccessor.right();
				}
				break;
			}

		}
	}
};
class CmdLine {
private:
	inline static constexpr auto CURSOR = '|';
	std::size_t pos;
public:
	CmdLine() {
		Console::getInstance().setTitle(std::string(1, CURSOR));
	}
	auto esc() {
		Console::getInstance().setTitle(std::string(1,CURSOR));//""だとアクセスエラー 
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
	auto getText() {
		auto cmd = Console::getInstance().getTitle();
		cmd.erase(pos,1);
		return std::move(cmd);
	}
};
class TextEditor {
private:
	inline static constexpr auto END_LINE = '|';
	Point<short> cursor,screenSize;
public:
	inline auto readAll() {
		std::string text;
		for (short y = 0;; ++y) {
			const auto line = Console::getInstance().read(screenSize.x, { 0,y });
			const auto endLine = line.find_last_of(END_LINE);
			if (endLine == std::string::npos)return text.substr(0,text.size()-1/*\n size*/);
			text.append(line.substr(0,endLine)+"\n");
		}
	}
	inline auto clear() {
		Console::getInstance().scroll({ {0,0},screenSize }, { 0,static_cast<decltype(screenSize.y)>(-screenSize.y)});
	}
	inline auto esc() {
		const auto args = split(KeyInput<CmdLine>().input().getText()," ");
		if (args.empty())return true;
		return !Command({
			{ "end",[](auto args) {return true; } },
			{"read",[&](auto args) {
				constexpr auto NAME = 1;
				if (args.size() <= NAME) {
					Console::getInstance().setTitle("error:Unspecified file name");
					return false;
				}
				const auto content = split(File(args[NAME]).read(), "\n");
				if (content.empty()) {
					Console::getInstance().setTitle("hit:file no content");
					return false;
				}
				clear();
				cursor = { 0,0 };
				Console::getInstance().setCursorPos(cursor);
				if (content.size() > screenSize.y) {
					screenSize.y = content.size();
					Console::getInstance().setScrollSize(screenSize);
				}
				for (const auto & line :content) {
					if (line.size() > screenSize.x) {
					    ++(screenSize.x= line.size());
						Console::getInstance().setScrollSize(screenSize);
					}
					std::cout << line << END_LINE<<std::endl;
				}
				Console::getInstance().setCursorPos(cursor);
				return false; 
			}},
			{"save",[&](auto args) {
				constexpr auto NAME = 1;
				if (args.size() <= NAME) {
					Console::getInstance().setTitle("error:Unspecified file name");
					return false;
				}
				Console::getInstance().setTitle(File(args[NAME]).write(readAll())?"success":"falid");
				return false;
			}},
			}
		)[args];
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
		if (Console::getInstance().read(screenSize.x, { 0,static_cast<decltype(screenSize.y)>(screenSize.y - 1) }).find('|') != std::string::npos) {
			++screenSize.y;//find not of \0
			Console::getInstance().setScrollSize(screenSize);
		}
		return true;
	}
	inline auto backspace() {
		if (!cursor.x) {
			if (!cursor.y)return;
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
		if (Console::getInstance().read(1/*END_LINE.size*/, { static_cast<decltype(screenSize.x)>(screenSize.x - 1),cursor.y }).front() == '|') {
			++screenSize.x += IsDBCSLeadByte(c);
			Console::getInstance().setScrollSize(screenSize);
		}//task:ENABLE_MOUSE_INPUT 
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
			if (Console::getInstance().read(screenSize.x, { 0,static_cast<decltype(cursor.y)>(cursor.y+1)}).find_last_of(END_LINE) == std::string::npos) return;
			++cursor.y;
			cursor.x = 0;
		}
		else if (endLine <= cursor.x)return;
		else {
			++cursor.x+= IsDBCSLeadByte(Console::getInstance().read(2, cursor).front());
		}
		Console::getInstance().setCursorPos(cursor);
	}
public:
	TextEditor() {
		clear();
		putchar(END_LINE);
		Console::getInstance().setCursorPos({0,0});
		cursor = Console::getInstance().getCursorPos();
		screenSize = Console::getInstance().getScreenSize();
	}
};

int main() {
	KeyInput<TextEditor> editor;
	editor.input();
	return EXIT_SUCCESS;
}

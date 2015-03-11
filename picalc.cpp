#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <string.h>

using namespace std;

enum ItemTypes { CHANNEL, VARIABLE, STRING };
class Item {
public:
	int type;
	string name, data;
	bool data_set;
	queue<Item*> stream;
	Item(const string n, const int t) : type(t) {
		if (type == STRING) { data = n; data_set = true; }
		else { name = n; data_set = false; }
	}
	void set(const string d) { data = d; data_set = true; }
};

vector<Item*> channels;
vector< vector<Item*> > variables;
Item* vfind(Item* item, vector<Item*> list) {
	for (int i = 0; i < list.size(); i++)
		if (list[i]->name == item->name) return list[i];
	return NULL;
}

enum ExprTypes { METHOD, INOUT, FLOW, KEYWORD, EMPTY };
class Expr {
public:
	int main_type;
	int id;
	virtual void run() { cerr << "Undefined expression, should not be here!" << endl; }
};

int input_count = 0;
int in_out_id = 0;
vector< queue<Item*> > io_lists;
bool read_halt = false;
bool write_halt = false;
bool read_ready = false;
vector<Expr*> read_halted;
vector<Expr*> write_halted;
void re_run(vector<Expr*> &halted);

enum InOutOpTypes { READ, WRITE };
class InOutOp : public Expr {
public:
	int type;
	int io_id;
	Item* channel;
	queue<Item*> list;
	InOutOp(const int t, const int i) : type(t) { main_type = INOUT; id = i; }
	void run() {
		Item *ch = vfind(channel, channels);
		if (ch == NULL) {
			cerr << "Channel " << channel->name << " has not been created" << endl;
			return;
		}

		if (list.empty()) list = io_lists[io_id];

		if (type == READ && !list.empty()) {
			if (!ch->stream.empty()) {
				Item *it = vfind(list.front(), variables[id]);
				Item *st = ch->stream.front(); ch->stream.pop();

				if (vfind(list.front(), channels) != NULL) {
					cerr << "Cannot create variable with name " << list.front()->name << ", channel exists with this name" << endl;
					return;
				}
				if (it == NULL) {
					cerr << "Variable with name " << list.front()->name << " does not exist" << endl;
					return;
				}

				it->set(st->data);
				list.pop();
			}

			if (!list.empty()) read_halt = true;
		}
		else if (type == WRITE && !list.empty()) {
			if (!read_halted.empty() || read_ready) {
				if (list.front()->type == STRING) {
					ch->stream.push(list.front()); list.pop();
				}
				else {
					Item *it = vfind(list.front(), variables[id]);

					if (vfind(list.front(), channels) != NULL)  {
						cerr << "Cannot send channels in this implementation" << endl;
						return;
					}
					if (it == NULL) {
						cerr << "Variable with name " << list.front()->name << " does not exist" << endl;
						return;
					}
					if (!it->data_set) {
						cerr << it->name << " has no data to send" << endl;
						return;
					}

					ch->stream.push(it); list.pop();
				}

				re_run(read_halted);
			}

			if (!list.empty()) write_halt = true;
		}
	}

	bool is_done() {
		return list.empty();
	}
};

bool run_and_verify(Expr* prog, Expr* add = NULL) {
	prog->run();
	if (read_halt) {
		read_halted.push_back(add != NULL ? add : prog);
		read_halt = false;
		re_run(write_halted);
		return false;
	}
	if (write_halt) {
		write_halted.push_back(add != NULL ? add : prog);
		write_halt = false;
		return false;
	}
	return true;
}

void re_run(vector<Expr*> &halted) {
	int max = write_halted.size() + read_halted.size();
	for (int i = 0; i < max; i++) {
		vector<Expr*> temp = halted;
		halted.clear();
		while (!temp.empty()) {
			run_and_verify(temp.back());
			temp.pop_back();
		}
	}
}

enum KeywordTypes { NEW, PRINT };
class Keyword : public Expr {
public:
	int type;
	int io_id;
	queue<Item*> list;
	Keyword(const int t, const int i) : type(t) { main_type = KEYWORD; id = i; }
	void run() {
		if (list.empty()) list = io_lists[io_id];

		if (type == PRINT) {
			while (!list.empty()) {
				if (list.front()->type == STRING) {
					cout << list.front()->data << endl;
				}
				else {
					Item* item = vfind(list.front(), variables[id]);
					if (item != NULL)
						if (item->data_set) cout << item->data << endl;
						else cerr << "Variable " << list.front()->name << " has no value" << endl;
					else {
						Item* itc = vfind(list.front(), channels);
						if (itc != NULL) cerr << "Cannot print channels" << endl;
						else cerr << "Variable " << list.front()->name << " has not been created" << endl;
					}
				}
				list.pop();
			}
		}
		else if (type == NEW) {
			while (!list.empty()) {
				if (vfind(list.front(), channels) == NULL) {
					if (vfind(list.front(), variables[id]) != NULL)
						cerr << "Cannot create channel with name " << list.front()->name << ", variable exists with this name" << endl;
					else
						channels.push_back(list.front());
				}
				list.pop();
			}

		}
	}
};

class Method : public Expr {
public:
	string name;
	Expr* program;
	Method(const string n, const int i) : name(n) { main_type = METHOD; id = i; }
	void run() { run_and_verify(program); }
};

vector<Method*> methods;
Method* vfind(string item, vector<Method*> list) {
	for (int i = 0; i < list.size(); i++)
		if (list[i]->name == item) return list[i];
	return NULL;
}

class Empty : public Expr {
public:
	void run() { }
};

Expr* deref(Expr* m) {
	Expr* ret = m;
	while (ret->main_type == METHOD)
		ret = ((Method*)ret)->program;
	return ret;
}

enum FlowOpTypes { PIPE, DOT, PLUS };
class FlowOp : public Expr {
public:
	int type;
	Expr* left;
	Expr* right;
	FlowOp(const int t, const int i) : type(t) { main_type = FLOW; id = i; }
	void run() {
		if (type == PIPE) {
			vector<Expr*> todo;
			todo.push_back(deref(left));
			todo.push_back(deref(right));

			bool added = true;
			while (added) {
				added = false;
				for (int i = 0; i < todo.size(); i++) {
					if (todo[i]->main_type == FLOW && ((FlowOp*)todo[i])->type == PIPE) {
						todo.push_back(deref(((FlowOp*)todo[i])->left));
						todo.push_back(deref(((FlowOp*)todo[i])->right));
						todo.erase(todo.begin() + i--);
						added = true;
					}
				}
			}

			for (int i = 0; i < todo.size(); i++) {
				if (todo[i]->main_type == KEYWORD && ((Keyword*)todo[i])->type == NEW)  {
					todo[i]->run(); todo.erase(todo.begin() + i--);
				}
			}

			for (int i = 0; i < todo.size(); i++) {
				if (todo[i]->main_type == INOUT && ((InOutOp*)todo[i])->type == READ)  {
					run_and_verify(todo[i]);
					todo.erase(todo.begin() + i--);
				}
			}

			for (int i = 0; i < todo.size(); i++) {
				if (todo[i]->main_type == INOUT && ((InOutOp*)todo[i])->type == WRITE)  {
					run_and_verify(todo[i]);
					todo.erase(todo.begin() + i--);
				}
			}

			for (int i = 0; i < todo.size(); i++) {
				if (todo[i]->main_type == KEYWORD && ((Keyword*)todo[i])->type == PRINT)  {
					todo[i]->run(); todo.erase(todo.begin() + i--);
				}
			}

			for (int i = 0; i < todo.size(); i++) {
				todo[i]->run();
			}
		}
		else if (type == DOT) {
			if (run_and_verify(left, this))
				run_and_verify(right);
		}
		else if (type == PLUS) {
			if (rand() % 2) { run_and_verify(left); }
			else { run_and_verify(right); }
		}
	}
};

queue<Item*> get_channels(string data) {
	if (data[0] == '(') return get_channels(data.substr(1, data.length() - 2));

	queue<Item*> ch;

	string text = "";
	for (int i = 0; i < data.size(); i++) {
		if (data[i] == ',') {
			ch.push(new Item(text, CHANNEL));
			text = "";
		}
		else
			text += data[i];
	}

	if (text.length() > 0)
		ch.push(new Item(text, CHANNEL));

	return ch;
}

queue<Item*> get_vars(string data, bool allow_strings) {
	if (data[0] == '(') return get_vars(data.substr(1, data.length() - 2), allow_strings);

	queue<Item*> vars;

	string text = "";
	for (int i = 0; i < data.size(); i++) {
		if (data[i] == '"' || data[i] == '\'') {
			char stop = data[i++];
			text = "";
			while (data[i] != stop)
				text += data[i++];

			if (allow_strings)
				vars.push(new Item(text, STRING));

			text = "";
			i++;
		}

		else if (data[i] == ',') {
			if (text.length() > 0) {
				vars.push(new Item(text, VARIABLE));
				variables[input_count].push_back(new Item(text, VARIABLE));
			}
			text = "";
		}
		else
			text += data[i];
	}

	if (text.length() > 0) {
		vars.push(new Item(text, VARIABLE));
		variables[input_count].push_back(new Item(text, VARIABLE));
	}

	return vars;
}

Expr* get_expr(string data) {
	if (data == "0") {
		return new Empty();
	}

	size_t ch = data.find(".");
	if (ch != string::npos) {
		int parens = 0;
		for (int i = 0; i < ch; i++) {
			if (data[i] == '(') parens++;
			else if (data[i] == ')') parens--;
		}
		if (parens == 0) {
			FlowOp* exp = new FlowOp(DOT, input_count);
			exp->left = get_expr(data.substr(0, ch));
			exp->right = get_expr(data.substr(ch + 1));
			return exp;
		}
	}

	ch = data.find("+");
	if (ch != string::npos) {
		int parens = 0;
		for (int i = 0; i < ch; i++) {
			if (data[i] == '(') parens++;
			else if (data[i] == ')') parens--;
		}
		if (parens == 0) {
			FlowOp* exp = new FlowOp(PLUS, input_count);
			exp->left = get_expr(data.substr(0, ch));
			exp->right = get_expr(data.substr(ch + 1));
			return exp;
		}
	}

	ch = data.find("|");
	if (ch != string::npos) {
		int parens = 0;
		for (int i = 0; i < ch; i++) {
			if (data[i] == '(') parens++;
			else if (data[i] == ')') parens--;
		}
		if (parens == 0) {
			FlowOp* exp = new FlowOp(PIPE, input_count);
			exp->left = get_expr(data.substr(0, ch));
			exp->right = get_expr(data.substr(ch + 1));
			return exp;
		}
	}

	ch = data.find("?");
	if (ch != string::npos) {
		int parens = 0;
		for (int i = 0; i < ch; i++) {
			if (data[i] == '(') parens++;
			else if (data[i] == ')') parens--;
		}
		if (parens == 0) {
			InOutOp* exp = new InOutOp(READ, input_count);
			exp->channel = new Item(data.substr(0, ch), CHANNEL);
			exp->list = get_vars(data.substr(ch + 1), false);
			exp->io_id = in_out_id++;
			io_lists.push_back(exp->list);
			return exp;
		}
	}

	ch = data.find("!");
	if (ch != string::npos) {
		int parens = 0;
		for (int i = 0; i < ch; i++) {
			if (data[i] == '(') parens++;
			else if (data[i] == ')') parens--;
		}
		if (parens == 0) {
			InOutOp* exp = new InOutOp(WRITE, input_count);
			exp->channel = new Item(data.substr(0, ch), CHANNEL);
			exp->list = get_vars(data.substr(ch + 1), true);
			exp->io_id = in_out_id++;
			io_lists.push_back(exp->list);
			return exp;
		}
	}

	if (data[0] == '(') {
		return get_expr(data.substr(1, data.length() - 2));
	}

	ch = data.find("new(");
	if (ch != string::npos) {
		Keyword* exp = new Keyword(NEW, input_count);
		exp->list = get_channels(data.substr(4, data.length() - 5));
		return exp;
	}

	ch = data.find("print(");
	if (ch != string::npos) {
		Keyword* exp = new Keyword(PRINT, input_count);
		exp->list = get_vars(data.substr(6, data.length() - 7), true);
		exp->io_id = in_out_id++;
		io_lists.push_back(exp->list);
		return exp;
	}

	if (data[0] >= 'A' || data[0] <= 'Z') {
		Method* m = vfind(data, methods);
		if (m == NULL) cerr << "Method " << data << " does not exist" << endl;
		else return m;
	}

	return NULL;
}

void parse(string data) {
	variables.push_back(vector<Item*>());

	int paren_count = 0;
	for (int i = 0; i < data.length(); i++) {
		if (data[i] == '(') paren_count++;
		else if (data[i] == ')') paren_count--;
		if (paren_count < 0) {
			cerr << "Mismatched parentheses (paren_count < 0)" << endl;
			return;
		}
	}
	if (paren_count != 0) {
		cerr << "Mismatched parentheses (paren_count != 0)" << endl;
		return;
	}

	size_t equals = data.find("=");
	if (equals != string::npos) {
		string n = data.substr(0, equals);
		Method* m = vfind(n, methods);
		if (m == NULL) {
			m = new Method(n, input_count);
			methods.push_back(m);
		}
		m->program = get_expr(data.substr(equals + 1));
		return;
	}

	Expr* curr = get_expr(data);
	if (curr == NULL) {
		cerr << "Invalid input" << endl;
		return;
	}

	curr->id = input_count++;
	run_and_verify(curr);
}

int main(int argc, char* argv[]) {
	bool show_markers = true;
	if (argc == 2 && strcmp(argv[1], "-p") == 0)
		show_markers = false;

	string input;
	if (show_markers) {
		cout << "Pi Calculus Engine, version 1.0" << endl;
		cout << "Copyright 2015 Alex DeBoni" << endl;
		cout << ">> ";
	}
	while (getline(cin, input)) {
		input.erase(remove_if(input.begin(), input.end(), ::isspace), input.end());
		if (input == "quit" || input == "exit") break;
		if (input == "") {}
		else if (input == "clear") {
			channels.clear();
			methods.clear();
			variables.clear();
			read_halted.clear();
			write_halted.clear();
			io_lists.clear();
			input_count = 0;
			in_out_id = 0;
		}
		else if (input == "status") {
			cout << "Channels: ";
			for (int i = 0; i < channels.size(); i++)
				cout << channels[i]->name << " ";
			cout << endl;

			cout << "Methods: ";
			for (int i = 0; i < methods.size(); i++)
				cout << methods[i]->name << " ";
			cout << endl;

			cout << read_halted.size() << " read halted processes" << endl;
			cout << write_halted.size() << " write halted processes" << endl;
		}
		else {
			parse(input);
		}

		if (show_markers) cout << ">> ";
	}

	return 0;
}


#include <iostream>
#include "ormcpp/ormcpp.h"
using namespace std;

class Person {
public:
	DEFINE_TABLE(Person, id, name, age);

	int id;

	std::string name;

	int age;
};

int main() {
	Person p{ 3, "JOJO", 15};
	Person q{ 1, "Huake", 23 };

	ormcpp<mysql>& pool = ormcpp<mysql>::instance();
	int max_connection_size = 5;
	pool.init(max_connection_size, "your ip", "your port", "your database password", "database name");
	auto con = pool.get();
	auto t = con->query<std::tuple<int, std::string>>("select id, name from Person order by id");
	for (auto d : t) {
		cout << std::get<0>(d) << ' ' << std::get<1>(d) << endl;
	}
	return 0;
}

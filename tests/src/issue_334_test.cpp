// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#include <daw/json/daw_json_iterator.h>
#include <daw/json/daw_json_link.h>

#include <iostream>
#include <unordered_map>

struct PersonObject {
	int id{ };
	std::string name;
	int age{ };
	std::unordered_map<std::string, std::string> fields;
};

namespace daw::json {
	template<>
	struct json_data_contract<PersonObject> {
		static constexpr char const id[] = "id";
		static constexpr char const name[] = "name";
		static constexpr char const age[] = "age";
		static constexpr char const fields[] = "fields";
		using type = json_member_list<
		  json_number<id, int>, json_string<name>, json_number_null<age, int>,
		  json_key_value<fields, std::unordered_map<std::string, std::string>,
		                 std::string>>;
	};
} // namespace daw::json

int main( ) {
	std::string raw_json{ R"_(
	{
		"id": 10,
		"name": "Jason",
		"fields": {"city": "Chicago", "Title": "Manager"},
		}
	)_" };
	PersonObject person = daw::json::from_json<PersonObject>( raw_json );

	return 0;
}

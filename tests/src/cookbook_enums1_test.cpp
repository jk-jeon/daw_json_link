// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//
// See cookbook/enums.md for the 1st example
//

#include "defines.h"

#include "daw/json/daw_json_link.h"

#include <daw/daw_memory_mapped_file.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

namespace daw::cookbook_enums1 {
	enum class Colours : uint8_t { red, green, blue, black };

	DAW_CONSTEXPR std::string_view to_string( Colours c ) {
		switch( c ) {
		case Colours::red:
			return "red";
		case Colours::green:
			return "green";
		case Colours::blue:
			return "blue";
		case Colours::black:
			return "black";
		}
		std::abort( );
	}

	DAW_CONSTEXPR Colours from_string( daw::tag_t<Colours>,
	                                   std::string_view sv ) {
		if( sv == "red" ) {
			return Colours::red;
		}
		if( sv == "green" ) {
			return Colours::green;
		}
		if( sv == "blue" ) {
			return Colours::blue;
		}
		if( sv == "black" ) {
			return Colours::black;
		}
		std::abort( );
	}

	struct MyClass1 {
		std::vector<Colours> member0;
	};

	bool operator==( MyClass1 const &lhs, MyClass1 const &rhs ) {
		return lhs.member0 == rhs.member0;
	}
} // namespace daw::cookbook_enums1

namespace daw::json {
	template<>
	struct json_data_contract<daw::cookbook_enums1::MyClass1> {
#ifdef __cpp_nontype_template_parameter_class
		using type = json_member_list<json_array<
		  "member0", json_custom<no_name, daw::cookbook_enums1::Colours>>>;
#else
		static DAW_CONSTEXPR inline char const member0[] = "member0";
		using type = json_member_list<
		  json_array<member0, json_custom<no_name, daw::cookbook_enums1::Colours>>>;
#endif
		static inline auto
		to_json_data( daw::cookbook_enums1::MyClass1 const &value ) {
			return std::forward_as_tuple( value.member0 );
		}
	};
} // namespace daw::json

int main( int argc, char **argv ) try {
	if( argc <= 1 ) {
		puts( "Must supply path to cookbook_enums1.json file\n" );
		exit( EXIT_FAILURE );
	}
	auto data = daw::filesystem::memory_mapped_file_t<>( argv[1] );

	auto const cls = daw::json::from_json<daw::cookbook_enums1::MyClass1>(
	  std::string_view( data.data( ), data.size( ) ) );

	daw_json_assert( cls.member0[0] == daw::cookbook_enums1::Colours::red,
	                 "Unexpected value" );
	daw_json_assert( cls.member0[1] == daw::cookbook_enums1::Colours::green,
	                 "Unexpected value" );
	daw_json_assert( cls.member0[2] == daw::cookbook_enums1::Colours::blue,
	                 "Unexpected value" );
	daw_json_assert( cls.member0[3] == daw::cookbook_enums1::Colours::black,
	                 "Unexpected value" );
	auto const str = daw::json::to_json( cls );
	puts( str.c_str( ) );

	auto const cls2 = daw::json::from_json<daw::cookbook_enums1::MyClass1>(
	  std::string_view( str.data( ), str.size( ) ) );

	daw_json_assert( cls == cls2, "Unexpected round trip error" );
} catch( daw::json::json_exception const &jex ) {
	std::cerr << "Exception thrown by parser: " << jex.reason( ) << std::endl;
	exit( 1 );
}
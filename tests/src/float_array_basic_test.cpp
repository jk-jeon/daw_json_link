// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#include "defines.h"

#include "daw/json/daw_json_iterator.h"

#include <daw/daw_do_n.h>
#include <daw/daw_random.h>

#include <iostream>
#include <nanobench.h>
#include <streambuf>
#include <string_view>

#if not defined( DAW_NUM_RUNS )
#if not defined( DEBUG ) or defined( NDEBUG )
static inline constexpr std::size_t DAW_NUM_RUNS = 10000;
#else
static inline constexpr std::size_t DAW_NUM_RUNS = 3000;
#endif
#endif
static_assert( DAW_NUM_RUNS > 0 );

struct Number {
	float a{ };
};
#ifdef __cpp_nontype_template_parameter_class
[[maybe_unused]] static DAW_CONSTEXPR auto
json_data_contract_for( Number ) noexcept {
	using namespace daw::json;
	return json_member_list<json_number<"a", float>>{ };
}
#else
namespace symbols_Number {
	static constexpr char const a[] = "a";
}

[[maybe_unused]] static DAW_CONSTEXPR auto
json_data_contract_for( Number ) noexcept {
	using namespace daw::json;
	return json_member_list<json_number<symbols_Number::a, float>>{ };
}
#endif

template<typename Float>
Float rand_float( ) {
	static DAW_CONSTEXPR Float fmin = 0;
	static DAW_CONSTEXPR Float fmax = 1;
	static auto e = std::default_random_engine( );
	static auto dis = std::uniform_real_distribution<Float>( fmin, fmax );
	return dis( e );
}

template<size_t NUMVALUES>
void test_func( ankerl::nanobench::Bench &b ) {
	using namespace daw::json;
	using iterator_t = daw::json::json_array_iterator<float>;

	std::string json_data3 = [] {
		std::string result = "[";
		result.reserve( NUMVALUES * 23 + 8 );
		daw::algorithm::do_n( NUMVALUES, [&result] {
			result += std::to_string( rand_float<float>( ) ) + ',';
		} );
		result.back( ) = ']';
		return result;
	}( );

	auto const json_sv =
	  daw::string_view( json_data3.data( ), json_data3.size( ) );
	auto data2 = std::unique_ptr<float[]>( new float[NUMVALUES] );
	b.batch( sizeof( float ) * NUMVALUES );
	b.run( "float", [&]( ) noexcept {
		//ankerl::nanobench::doNotOptimizeAway( json_sv );
		auto ptr = std::copy( iterator_t( json_sv ), iterator_t( ), data2.get( ) );
		ankerl::nanobench::doNotOptimizeAway( data2 );
		ankerl::nanobench::doNotOptimizeAway( ptr );
	} );
}

int main( int argc, char ** )
#ifdef DAW_USE_JSON_EXCEPTIONS
  try
#endif
{

	auto b1 = ankerl::nanobench::Bench( )
	            .title( "nativejson parts" )
	            .unit( "byte" )
	            .warmup( 3000 )
	            .minEpochIterations( DAW_NUM_RUNS );
	if( argc > 1 ) {
		test_func<1'000'000ULL>( b1 );
	} else {
		test_func<1'000ULL>( b1 );
	}
} catch( daw::json::json_exception const &jex ) {
	std::cerr << "Exception thrown by parser: " << jex.reason( ) << std::endl;
	exit( 1 );
}

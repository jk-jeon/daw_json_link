// The MIT License (MIT)
//
// Copyright (c) 2019-2020 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "impl/daw_iterator_range.h"
#include "impl/daw_json_link_impl.h"
#include "impl/daw_json_link_types_fwd.h"

#include <daw/daw_algorithm.h>
#include <daw/daw_array.h>
#include <daw/daw_bounded_string.h>
#include <daw/daw_cxmath.h>
#include <daw/daw_exception.h>
#include <daw/daw_parser_helper_sv.h>
#include <daw/daw_traits.h>
#include <daw/daw_utility.h>
#include <daw/iterator/daw_back_inserter.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>

namespace daw::json {
	/**
	 *
	 * @tparam JsonMembers JSON classes that map the relation ship from the json
	 * data to the classes constructor
	 */
	template<typename... JsonMembers>
	struct json_member_list {
		/**
		 * Serialize a C++ class to JSON data
		 * @tparam OutputIterator An output iterator with a char value_type
		 * @tparam Args  tuple of values that map to the JSON members
		 * @param it OutputIterator to append string data to
		 * @param args members from C++ class
		 * @return the OutputIterator it
		 */
		template<typename OutputIterator, typename... Args>
		[[maybe_unused, nodiscard]] static constexpr OutputIterator
		serialize( OutputIterator it, std::tuple<Args...> const &args ) {
			static_assert( sizeof...( Args ) == sizeof...( JsonMembers ),
			               "Argument count is incorrect" );

			static_assert( ( impl::is_a_json_type_v<JsonMembers> and ... ),
			               "Only value JSON types can be used" );
			return impl::serialize_json_class<JsonMembers...>(
			  it, std::index_sequence_for<Args...>{}, args );
		}

		/**
		 * Parse JSON data and construct a C++ class
		 * @tparam JsonClass The result of parsing json_class
		 * @tparam IsUnCheckedInput Is the input trusted, less checking is done
		 * @param sv JSON data to parse
		 * @return A T object
		 */
		template<typename JsonClass, bool IsUnCheckedInput>
		[[maybe_unused, nodiscard]] static constexpr JsonClass
		parse( std::string_view sv ) {
			daw_json_assert_weak( not sv.empty( ), "Cannot parse an empty string" );

			auto rng =
			  impl::IteratorRange<char const *, char const *, IsUnCheckedInput>(
			    sv.data( ), sv.data( ) + sv.size( ) );
			return impl::parse_json_class<JsonClass, JsonMembers...>(
			  rng, std::index_sequence_for<JsonMembers...>{} );
		}

		/**
		 *
		 * Parse JSON data and construct a C++ class
		 * @tparam T The result of parsing json_class
		 * @tparam First type of first iterator in range
		 * @tparam Last type of last iterator in range
		 * @tparam IsUnCheckedInput Is the input trusted, less checking is done
		 * @param rng JSON data to parse
		 * @return A T object
		 */
		template<typename T, typename First, typename Last, bool IsUnCheckedInput>
		[[maybe_unused, nodiscard]] static constexpr T
		parse( impl::IteratorRange<First, Last, IsUnCheckedInput> &rng ) {
			daw_json_assert_weak( rng.has_more( ), "Cannot parse an empty string" );
			return impl::parse_json_class<T, JsonMembers...>(
			  rng, std::index_sequence_for<JsonMembers...>{} );
		}
	};

	// Member types
	template<JSONNAMETYPE Name, typename T, LiteralAsStringOpt LiteralAsString,
	         typename Constructor, JsonRangeCheck RangeCheck,
	         JsonNullable Nullable, SIMDModes SIMDMode>
	struct json_number {
		using i_am_a_json_type = void;
		using wrapped_type = T;
		using base_type = impl::unwrap_type<T, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor, base_type>;
		static_assert(
		  Nullable == JsonNullable::Never or
		    std::is_same_v<parse_to_t, std::invoke_result_t<Constructor>>,
		  "Default ctor of constructor must match that of base" );
		using constructor_t = Constructor;
		static constexpr JSONNAMETYPE name = Name;

		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<impl::number_parse_type_v<base_type>, Nullable>;

		static constexpr JsonParseTypes base_expected_type =
		  impl::number_parse_type_v<base_type>;

		static constexpr LiteralAsStringOpt literal_as_string = LiteralAsString;
		static constexpr JsonRangeCheck range_check = RangeCheck;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::Number;
		static constexpr SIMDModes simd_mode = SIMDMode;
	};

	template<JSONNAMETYPE Name, typename T, LiteralAsStringOpt LiteralAsString,
	         typename Constructor, JsonNullable Nullable>
	struct json_bool {
		using i_am_a_json_type = void;
		using wrapped_type = T;
		using base_type = impl::unwrap_type<T, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor, base_type>;
		using constructor_t = Constructor;

		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::Bool, Nullable>;
		static constexpr JsonParseTypes base_expected_type = JsonParseTypes::Bool;

		static constexpr JSONNAMETYPE name = Name;
		static constexpr LiteralAsStringOpt literal_as_string = LiteralAsString;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::Bool;
	};

	template<JSONNAMETYPE Name, typename String, typename Constructor,
	         JsonNullable EmptyStringNull, EightBitModes EightBitMode,
	         JsonNullable Nullable>
	struct json_string_raw {
		using i_am_a_json_type = void;
		using constructor_t = Constructor;
		using wrapped_type = String;
		using base_type = impl::unwrap_type<String, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor, base_type>;

		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::StringRaw, Nullable>;
		static constexpr JsonParseTypes base_expected_type =
		  JsonParseTypes::StringRaw;
		static constexpr JsonNullable empty_is_null = EmptyStringNull;
		static constexpr EightBitModes eight_bit_mode = EightBitMode;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::String;
	};

	template<JSONNAMETYPE Name, typename String, typename Constructor,
	         typename Appender, JsonNullable EmptyStringNull,
	         EightBitModes EightBitMode, JsonNullable Nullable>
	struct json_string {
		using i_am_a_json_type = void;
		using constructor_t = Constructor;
		using wrapped_type = String;
		using base_type = impl::unwrap_type<String, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor, base_type>;
		using appender_t = Appender;

		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::StringEscaped, Nullable>;
		static constexpr JsonParseTypes base_expected_type =
		  JsonParseTypes::StringEscaped;
		static constexpr JsonNullable empty_is_null = EmptyStringNull;
		static constexpr EightBitModes eight_bit_mode = EightBitMode;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::String;
	};

	template<JSONNAMETYPE Name, typename T, typename Constructor,
	         JsonNullable Nullable>
	struct json_date {
		using i_am_a_json_type = void;
		using constructor_t = Constructor;
		using wrapped_type = T;
		using base_type = impl::unwrap_type<T, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor, char const *, size_t>;

		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::Date, Nullable>;
		static constexpr JsonParseTypes base_expected_type = JsonParseTypes::Date;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::String;
	};

	template<JSONNAMETYPE Name, typename T, typename Constructor,
	         JsonNullable Nullable>
	struct json_class {
		using i_am_a_json_type = void;
		using constructor_t = Constructor;
		using wrapped_type = T;
		using base_type = impl::unwrap_type<T, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = T;
		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::Class, Nullable>;
		static constexpr JsonParseTypes base_expected_type = JsonParseTypes::Class;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::Class;
	};

	/***
	 * A type to hold the types for parsing variants.
	 * @tparam JsonElements Up to one of a JsonElement that is a JSON number,
	 * string, object, or array
	 */
	template<typename... JsonElements>
	struct json_variant_type_list {
		using i_am_variant_element_list = void;
		static_assert(
		  sizeof...( JsonElements ) <= 5U,
		  "There can be at most 5 items, one for each JsonBaseParseTypes" );
		using element_map_t =
		  std::tuple<impl::unnamed_default_type_mapping<JsonElements>...>;
		static constexpr size_t base_map[5] = {
		  impl::find_json_element<JsonBaseParseTypes::Number>(
		    {impl::unnamed_default_type_mapping<
		      JsonElements>::underlying_json_type...} ),
		  impl::find_json_element<JsonBaseParseTypes::Bool>(
		    {impl::unnamed_default_type_mapping<
		      JsonElements>::underlying_json_type...} ),
		  impl::find_json_element<JsonBaseParseTypes::String>(
		    {impl::unnamed_default_type_mapping<
		      JsonElements>::underlying_json_type...} ),
		  impl::find_json_element<JsonBaseParseTypes::Class>(
		    {impl::unnamed_default_type_mapping<
		      JsonElements>::underlying_json_type...} ),
		  impl::find_json_element<JsonBaseParseTypes::Array>(
		    {impl::unnamed_default_type_mapping<
		      JsonElements>::underlying_json_type...} )};
	};

	template<JSONNAMETYPE Name, typename T, typename JsonElements,
	         typename Constructor, JsonNullable Nullable>
	struct json_variant {
		using i_am_a_json_type = void;
		static_assert(
		  std::is_same_v<typename JsonElements::i_am_variant_element_list, void>,
		  "Expected a json_variant_type_list" );
		using constructor_t = Constructor;
		using wrapped_type = T;
		using base_type = impl::unwrap_type<T, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = T;
		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::Variant, Nullable>;
		static constexpr JsonParseTypes base_expected_type =
		  JsonParseTypes::Variant;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::None;
		using json_elements = JsonElements;
	};

	template<JSONNAMETYPE Name, typename T, typename FromConverter,
	         typename ToConverter, CustomJsonTypes CustomJsonType,
	         JsonNullable Nullable>
	struct json_custom {
		using i_am_a_json_type = void;
		using to_converter_t = ToConverter;
		using from_converter_t = FromConverter;
		// TODO explore using char const *, size_t pair for ctor
		using base_type = impl::unwrap_type<T, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<FromConverter, std::string_view>;

		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::Custom, Nullable>;
		static constexpr JsonParseTypes base_expected_type = JsonParseTypes::Custom;
		static constexpr CustomJsonTypes custom_json_type = CustomJsonType;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::String;
	};

	template<JSONNAMETYPE Name, typename JsonElement, typename Container,
	         typename Constructor, typename Appender, JsonNullable Nullable>
	struct json_array {
		using i_am_a_json_type = void;
		using json_element_t = impl::unnamed_default_type_mapping<JsonElement>;
		static_assert( not std::is_same_v<json_element_t, void>,
		               "Unknown JsonElement type." );
		static_assert( impl::is_a_json_type_v<json_element_t>,
		               "Error determining element type" );
		using constructor_t = Constructor;

		using base_type = impl::unwrap_type<Container, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor>;
		using appender_t = Appender;
		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::Array, Nullable>;
		static constexpr JsonParseTypes base_expected_type = JsonParseTypes::Array;

		static_assert( json_element_t::name == no_name,
		               "All elements of json_array must be have no_name" );
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::Array;
	};

	template<JSONNAMETYPE Name, typename Container, typename JsonValueType,
	         typename JsonKeyType, typename Constructor, typename Appender,
	         JsonNullable Nullable>
	struct json_key_value {
		using i_am_a_json_type = void;
		using constructor_t = Constructor;
		using base_type = impl::unwrap_type<Container, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor>;
		using appender_t = Appender;
		using json_element_t = impl::unnamed_default_type_mapping<JsonValueType>;
		static_assert( not std::is_same_v<json_element_t, void>,
		               "Unknown JsonValueType type." );
		static_assert( json_element_t::name == no_name,
		               "Value member name must be the default no_name" );

		using json_key_t = impl::unnamed_default_type_mapping<JsonKeyType>;
		static_assert( not std::is_same_v<json_key_t, void>,
		               "Unknown JsonKeyType type." );
		static_assert( json_key_t::name == no_name,
		               "Key member name must be the default no_name" );

		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::KeyValue, Nullable>;
		static constexpr JsonParseTypes base_expected_type =
		  JsonParseTypes::KeyValue;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::Class;
	};

	template<JSONNAMETYPE Name, typename Container, typename JsonValueType,
	         typename JsonKeyType, typename Constructor, typename Appender,
	         JsonNullable Nullable>
	struct json_key_value_array {
		using i_am_a_json_type = void;
		using constructor_t = Constructor;
		using base_type = impl::unwrap_type<Container, Nullable>;
		static_assert( not std::is_same_v<void, base_type>,
		               "Failed to detect base type" );
		using parse_to_t = std::invoke_result_t<Constructor>;
		using appender_t = Appender;
		using json_key_t =
		  impl::unnamed_default_type_mapping<JsonKeyType, impl::default_key_name>;
		static_assert( not std::is_same_v<json_key_t, void>,
		               "Unknown JsonKeyType type." );
		static_assert( json_key_t::name != no_name,
		               "Must supply a valid key member name" );
		using json_value_t =
		  impl::unnamed_default_type_mapping<JsonValueType,
		                                     impl::default_value_name>;
		static_assert( not std::is_same_v<json_value_t, void>,
		               "Unknown JsonValueType type." );
		static_assert( json_value_t::name != no_name,
		               "Must supply a valid value member name" );
		static_assert( json_key_t::name != json_value_t::name,
		               "Key and Value member names cannot be the same" );
		static constexpr JSONNAMETYPE name = Name;
		static constexpr JsonParseTypes expected_type =
		  get_parse_type_v<JsonParseTypes::KeyValueArray, Nullable>;
		static constexpr JsonParseTypes base_expected_type =
		  JsonParseTypes::KeyValueArray;
		static constexpr JsonBaseParseTypes underlying_json_type =
		  JsonBaseParseTypes::Array;
	};

	/**
	 * Parse JSON and construct a T as the result.  This method
	 * provides checked json
	 * @tparam JsonClass type that has specialization of
	 * daw::json::json_data_contract
	 * @param json_data JSON string data
	 * @return A reified T constructed from JSON data
	 */
	template<typename JsonClass>
	[[maybe_unused, nodiscard]] constexpr JsonClass
	from_json( std::string_view json_data ) {
		static_assert( impl::has_json_data_contract_trait_v<JsonClass>,
		               "Expected a typed that has been mapped via specialization "
		               "of daw::json::json_data_contract" );
		return impl::from_json_impl<JsonClass, false>( json_data );
	}

	/***
	 * Parse a JSONMember from the json_data starting at member_path.
	 * @tparam JsonMember The type of the item being parsed
	 * @param json_data JSON string data
	 * @param member_path A dot separated path of member names, default is the
	 * root
	 * @return A value reified from the JSON data member
	 */
	template<typename JsonMember>
	[[maybe_unused, nodiscard]] static constexpr auto
	from_json( std::string_view json_data, std::string_view member_path ) {
		return impl::from_json_member_impl<JsonMember, false>( json_data,
		                                                       member_path );
	}

	/***
	 * Parse a JSONMember from the json_data starting at member_path.  This method
	 * performs less checking than the non-unchecked method
	 * @tparam JsonMember The type of the item being parsed
	 * @param json_data JSON string data
	 * @param member_path A dot separated path of member names, default is the
	 * root
	 * @return A value reified from the JSON data member
	 */
	template<typename JsonMember>
	[[maybe_unused, nodiscard]] static constexpr auto
	from_json_unchecked( std::string_view json_data,
	                     std::string_view member_path ) {
		return impl::from_json_member_impl<JsonMember, true>( json_data,
		                                                      member_path );
	}

	/**
	 * Parse JSON and construct a JsonClass as the result.  This method
	 * does not perform most checks on validity of data
	 * @tparam JsonClass type that has specialization of
	 * daw::json::json_data_contract
	 * @param json_data JSON string data
	 * @return A reified JsonClass constructed from JSON data
	 */
	template<typename JsonClass>
	[[maybe_unused, nodiscard]] constexpr JsonClass
	from_json_unchecked( std::string_view json_data ) {
		static_assert( impl::has_json_data_contract_trait_v<JsonClass>,
		               "Expected a typed that has been mapped via specialization "
		               "of daw::json::json_data_contract" );

		return impl::from_json_impl<JsonClass, true>( json_data );
	}

	/**
	 *
	 * @tparam Result std::string like type to put result into
	 * @tparam JsonClass Type that has json_parser_desription and to_json_data
	 * function overloads
	 * @param value  value to serialize
	 * @return  JSON string data
	 */
	template<typename Result = std::string, typename JsonClass>
	[[maybe_unused, nodiscard]] constexpr Result
	to_json( JsonClass const &value ) {
		static_assert( impl::has_json_data_contract_trait_v<JsonClass>,
		               "Expected a typed that has been mapped via specialization "
		               "of daw::json::json_data_contract" );
		static_assert( impl::has_json_to_json_data_v<JsonClass>,
		               "A function called to_json_data must exist for type." );

		Result result{};
		(void)impl::json_data_contract_trait_t<JsonClass>::template serialize(
		  daw::back_inserter( result ),
		  daw::json::json_data_contract<JsonClass>::to_json_data( value ) );
		return result;
	}

	namespace impl {
		namespace {
			template<bool IsUnCheckedInput, typename JsonElement, typename Container,
			         typename Constructor, typename Appender>
			[[maybe_unused, nodiscard]] constexpr Container
			from_json_array_impl( std::string_view json_data,
			                      std::string_view member_path ) {
				using parser_t =
				  json_array<no_name, JsonElement, Container, Constructor, Appender>;

				auto [is_found, rng] = impl::find_range<IsUnCheckedInput>(
				  json_data, {member_path.data( ), member_path.size( )} );
				if constexpr( parser_t::expected_type == JsonParseTypes::Null ) {
					if( not is_found ) {
						return typename parser_t::constructor_t{}( );
					}
				} else {
					daw_json_assert( is_found, "Could not find specified member" );
				}
				rng.trim_left_no_check( );
				daw_json_assert_weak( rng.front( '[' ), "Expected array class" );

				return parse_value<parser_t>( ParseTag<JsonParseTypes::Array>{}, rng );
			}
		} // namespace
	}   // namespace impl

	/**
	 * Parse JSON data where the root item is an array
	 * @tparam JsonElement The type of each element in array.  Must be one of
	 * the above json_XXX classes.  This version is checked
	 * @tparam Container Container to store values in
	 * @tparam Constructor Callable to construct Container with no arguments
	 * @tparam Appender Callable to call with JsonElement
	 * @param json_data JSON string data containing array
	 * @param member_path A dot separated path of member names to start parsing
	 * from.
	 * @return A Container containing parsed data from JSON string
	 */
	template<
	  typename JsonElement,
	  typename Container = std::vector<
	    typename impl::unnamed_default_type_mapping<JsonElement>::parse_to_t>,
	  typename Constructor = daw::construct_a_t<Container>,
	  typename Appender = impl::basic_appender<Container>>
	[[maybe_unused, nodiscard]] constexpr Container
	from_json_array( std::string_view json_data,
	                 std::string_view member_path = "" ) {
		using element_type = impl::unnamed_default_type_mapping<JsonElement>;
		static_assert( not std::is_same_v<element_type, void>,
		               "Unknown JsonElement type." );

		return impl::from_json_array_impl<false, element_type, Container,
		                                  Constructor, Appender>( json_data,
		                                                          member_path );
	}

	/**
	 * Parse JSON data where the root item is an array but do less checking
	 * @tparam JsonElement The type of each element in array.  Must be one of
	 * the above json_XXX classes.  This version isn't checked
	 * @tparam Container Container to store values in
	 * @tparam Constructor Callable to construct Container with no arguments
	 * @tparam Appender Callable to call with JsonElement
	 * @param json_data JSON string data containing array
	 * @param member_path A dot separated path of member names to start parsing
	 * from.
	 * @return A Container containing parsed data from JSON string
	 */
	template<
	  typename JsonElement,
	  typename Container = std::vector<
	    typename impl::unnamed_default_type_mapping<JsonElement>::parse_to_t>,
	  typename Constructor = daw::construct_a_t<Container>,
	  typename Appender = impl::basic_appender<Container>>
	[[maybe_unused, nodiscard]] constexpr Container
	from_json_array_unchecked( std::string_view json_data,
	                           std::string_view member_path = "" ) {
		using element_type = impl::unnamed_default_type_mapping<JsonElement>;
		static_assert( not std::is_same_v<element_type, void>,
		               "Unknown JsonElement type." );

		return impl::from_json_array_impl<true, element_type, Container,
		                                  Constructor, Appender>( json_data,
		                                                          member_path );
	}

	/**
	 * Serialize Container to a JSON array string
	 * @tparam Result std::string like type to serialize to
	 * @tparam Container Type of Container to serialize
	 * @param c Data to serialize
	 * @return A string containing the serialized elements of c
	 */
	template<typename Result = std::string, typename Container>
	[[maybe_unused, nodiscard]] constexpr Result to_json_array( Container &&c ) {
		static_assert(
		  daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		  "Supplied container must support begin( )/end( )" );

		Result result = "[";
		for( auto const &v : c ) {
			result += to_json( v );
			result += ',';
		}
		// The last character will be a ',' prior to this
		result.back( ) = ']';
		return result;
	}

	namespace impl {
		namespace {
			template<typename... Args>
			constexpr void
			is_unique_ptr_test_impl( std::unique_ptr<Args...> const & );

			template<typename T>
			using is_unique_ptr_test =
			  decltype( is_unique_ptr_test_impl( std::declval<T>( ) ) );

			template<typename T>
			inline constexpr bool is_unique_ptr_v =
			  daw::is_detected_v<is_unique_ptr_test, T>;
		} // namespace
	}   // namespace impl

	template<typename T>
	struct UniquePtrConstructor {
		static_assert( not impl::is_unique_ptr_v<T>,
		               "T should be the type contained in the unique_ptr" );

		constexpr std::unique_ptr<T> operator( )( ) const {
			return {};
		}

		template<typename Arg, typename... Args>
		std::unique_ptr<T> operator( )( Arg &&arg, Args &&... args ) const {
			return std::make_unique<T>( std::forward<Arg>( arg ),
			                            std::forward<Args>( args )... );
		}
	};
} // namespace daw::json

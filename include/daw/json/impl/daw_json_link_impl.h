// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "daw_arrow_proxy.h"
#include "daw_iso8601_utils.h"
#include "daw_iterator_range.h"
#include "daw_json_assert.h"
#include "daw_json_parse_common.h"
#include "daw_json_parse_value.h"
#include "daw_json_serialize_impl.h"
#include "daw_json_to_string.h"
#include "daw_murmur3.h"

#include <daw/daw_algorithm.h>
#include <daw/daw_cxmath.h>
#include <daw/daw_parser_helper_sv.h>
#include <daw/daw_scope_guard.h>
#include <daw/daw_sort_n.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>
#include <daw/daw_utility.h>
#include <daw/iterator/daw_back_inserter.h>
#include <daw/iterator/daw_inserter.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace daw::json {
	template<JsonNullable>
	struct construct_from_iso8601_timestamp;

	template<>
	struct construct_from_iso8601_timestamp<JsonNullable::Never> {
		using result_type = std::chrono::time_point<std::chrono::system_clock,
		                                            std::chrono::milliseconds>;

		[[maybe_unused, nodiscard]] inline constexpr result_type
		operator( )( char const *ptr, std::size_t sz ) const {
			return datetime::parse_iso8601_timestamp( daw::string_view( ptr, sz ) );
		}
	};

	template<>
	struct construct_from_iso8601_timestamp<JsonNullable::Nullable> {
		using result_type =
		  std::optional<std::chrono::time_point<std::chrono::system_clock,
		                                        std::chrono::milliseconds>>;

		[[maybe_unused, nodiscard]] inline constexpr result_type
		operator( )( ) const {
			return { };
		}

		[[maybe_unused, nodiscard]] inline constexpr result_type
		operator( )( char const *ptr, std::size_t sz ) const {
			return datetime::parse_iso8601_timestamp( daw::string_view( ptr, sz ) );
		}
	};

	template<typename T>
	struct custom_from_converter_t {
		[[nodiscard]] inline constexpr decltype( auto ) operator( )( ) {
			if constexpr( std::is_same_v<T, std::string_view> ) {
				return std::string_view{ };
			} else if constexpr( std::is_same_v<T,
			                                    std::optional<std::string_view>> ) {
				return std::string_view{ };
			} else {
				return from_string( daw::tag<T> );
			}
		}

		[[nodiscard]] inline constexpr decltype( auto )
		operator( )( std::string_view sv ) {
			if constexpr( std::is_same_v<T, std::string_view> or
			              std::is_same_v<T, std::optional<std::string_view>> ) {
				return sv;
			} else {
				return from_string( daw::tag<T>, sv );
			}
		}
	};
} // namespace daw::json

namespace daw::json::json_details {
	template<typename Container, typename Value>
	using can_insert1_test =
	  decltype( std::declval<Container &>( ).insert( std::declval<Value>( ) ) );

	template<typename Container, typename Value>
	inline constexpr bool can_insert1_v =
	  daw::is_detected_v<can_insert1_test, Container, Value>;

	template<bool HashesCollide, typename Range>
	struct location_info_t {
		std::uint32_t hash_value = 0;
		daw::string_view name;
		Range location{ };
		std::size_t count = 0;

		explicit constexpr location_info_t( daw::string_view Name ) noexcept
		  : hash_value( daw::murmur3_32( Name ) )
		  , name( Name ) {}

		[[maybe_unused, nodiscard]] inline constexpr bool missing( ) const {
			return location.is_null( );
		}
	};

	template<typename Range>
	struct location_info_t<false, Range> {
		std::uint32_t hash_value = 0;
		Range location{ };
		std::size_t count = 0;

		explicit constexpr location_info_t( daw::string_view Name ) noexcept
		  : hash_value( daw::murmur3_32( Name ) ) {}

		[[maybe_unused, nodiscard]] inline constexpr bool missing( ) const {
			return location.is_null( );
		}
	};

	/***
	 * Contains an array of member location_info mapped in a json_class
	 * @tparam N Number of mapped members from json_class
	 * @tparam Range see IteratorRange
	 */
	template<std::size_t N, typename Range, bool HasCollisions = true>
	struct locations_info_t {
		using value_type = location_info_t<HasCollisions, Range>;
		static constexpr bool has_collisons = HasCollisions;
		std::array<value_type, N> names;

		constexpr location_info_t<HasCollisions, Range> const &
		operator[]( std::size_t idx ) const {
			return names[idx];
		}

		constexpr location_info_t<HasCollisions, Range> &
		operator[]( std::size_t idx ) {
			return names[idx];
		}

		static constexpr std::size_t size( ) noexcept {
			return N;
		}

		[[nodiscard]] constexpr std::size_t
		find_name( daw::string_view key ) const {
			uint32_t const hash = murmur3_32( key );
			for( std::size_t n = 0; n < N; ++n ) {
				if( names[n].hash_value == hash ) {
					if constexpr( has_collisons ) {
						if( key != names[n].name ) {
							continue;
						}
					}
					return n;
				}
			}
			return N;
		}
	};

	template<typename... MemberNames>
	constexpr bool do_hashes_collide( ) noexcept {
		std::array<std::uint32_t, sizeof...( MemberNames )> hashes{
		  murmur3_32( MemberNames::name )... };

		daw::sort( hashes.begin( ), hashes.end( ) );
		return daw::algorithm::adjacent_find(
		         hashes.begin( ), hashes.end( ),
		         []( std::uint32_t const &l, std::uint32_t const &r ) {
			         return l == r;
		         } ) != hashes.end( );
	}

	template<typename Range, typename... JsonMembers>
	constexpr auto make_locations_info( ) {
		constexpr bool hashes_collide = do_hashes_collide<JsonMembers...>( );
		return locations_info_t<sizeof...( JsonMembers ), Range, hashes_collide>{
		  location_info_t<hashes_collide, Range>{ JsonMembers::name }... };
	}
	/***
	 * Get the position from already seen JSON members or move the parser
	 * forward until we reach the end of the class or the member.
	 * @tparam JsonMember current member in json_class
	 * @tparam N Number of members in json_class
	 * @tparam Range see IteratorRange
	 * @param pos JsonMember's position in locations
	 * @param locations members location and names
	 * @param rng Current JSON data
	 * @return IteratorRange with begin( ) being start of value
	 */
	template<typename JsonMember, std::size_t N, typename Range, bool B>
	[[nodiscard]] inline constexpr Range
	find_class_member( std::size_t pos, locations_info_t<N, Range, B> &locations,
	                   Range &rng ) {

		daw_json_assert_weak(
		  ( is_json_nullable_v<JsonMember> or not locations[pos].missing( ) or
		    not rng.is_closing_brace_checked( ) ),
		  "Unexpected end of class.  Non-nullable members still not found" );

		rng.trim_left_unchecked( );
		// TODO: should we check for end
		while( locations[pos].missing( ) and
		       not rng.is_closing_brace_unchecked( ) ) {
			daw_json_assert_weak( rng.has_more( ), "Unexpected end of stream" );
			auto const name = parse_name( rng );
			auto const name_pos = locations.find_name( name );
			if( name_pos >= locations.size( ) ) {
				// This is not a member we are concerned with
				(void)skip_value( rng );
				rng.clean_tail( );
				continue;
			}
			if( name_pos == pos ) {
				locations[pos].location = rng;
			} else {
				// We are out of order, store position for later
				// OLDTODO:	use type knowledge to speed up skip
				// OLDTODO:	on skipped classes see if way to store
				// 				member positions so that we don't have to
				//				reparse them after
				// RESULT: storing preparsed is slower, don't try 3 times
				// it also limits the type of things we can parse potentially
				// Using locations to switch on BaseType is slower too
				locations[name_pos].location = skip_value( rng );

				rng.clean_tail( );
			}
		}
		return locations[pos].location;
	}

	namespace pocm_details {
		/***
		 * Maybe skip json members
		 * @tparam Range see IteratorRange
		 * @param rng JSON data
		 * @param current_position current member index
		 * @param desired_position desired member index
		 */
		template<typename Range>
		constexpr void maybe_skip_members( Range &rng,
		                                   std::size_t &current_position,
		                                   std::size_t desired_position ) {

			rng.clean_tail( );
			daw_json_assert_weak( rng.has_more( ), "Unexpected end of range" );
			daw_json_assert_weak( current_position <= desired_position,
			                      "Order of ordered members must be ascending" );
			while( current_position < desired_position and
			       not rng.is_closing_bracket_unchecked( ) ) {
				(void)skip_value( rng );
				rng.clean_tail( );
				++current_position;
				daw_json_assert_weak( rng.has_more( ), "Unexpected end of range" );
			}
		}
	} // namespace pocm_details

	/***
	 * Parse a class member in an order json class(class as array)
	 * @tparam JsonMember type description of member to parse
	 * @tparam Range see IteratorRange
	 * @param member_position current position in array
	 * @param rng JSON data
	 * @return A reified value of type JsonMember::parse_to_t
	 */
	template<typename JsonMember, typename Range>
	[[nodiscard]] inline constexpr auto
	parse_ordered_class_member( std::size_t &member_position, Range &rng ) {

		/***
		 * Some members specify their index so there may be gaps between member
		 * data elements in the array.
		 */
		if constexpr( is_an_ordered_member_v<JsonMember> ) {
			pocm_details::maybe_skip_members( rng, member_position,
			                                  JsonMember::member_index );
		} else {
			rng.clean_tail( );
		}
		using json_member_type = ordered_member_subtype_t<JsonMember>;

		// this is an out value, get position ready
		++member_position;
		if( rng.is_closing_bracket_unchecked( ) ) {
			if constexpr( is_json_nullable_v<ordered_member_subtype_t<JsonMember>> ) {
				using constructor_t = typename json_member_type::constructor_t;
				return constructor_t{ }( );
			} else if constexpr( is_json_nullable_v<json_member_type> ) {
				daw_json_error( missing_member( "ordered_class_member" ) );
			}
		}
		return parse_value<json_member_type>(
		  ParseTag<json_member_type::expected_type>{ }, rng );
	}

	namespace parse_tokens {
		inline constexpr char const next_member_token[] = "\"}";
	}
	/***
	 * Parse a member from a json_class
	 * @tparam JsonMember type description of member to parse
	 * @tparam N Number of members in json_class
	 * @tparam Range see IteratorRange
	 * @param member_position positon in json_class member list
	 * @param locations location info for members
	 * @param rng JSON data
	 * @return parsed value from JSON data
	 */
	template<typename JsonMember, std::size_t N, typename Range, bool B>
	[[nodiscard]] DAW_ATTRIBUTE_FLATTEN inline constexpr json_result<JsonMember>
	parse_class_member( std::size_t member_position,
	                    locations_info_t<N, Range, B> &locations, Range &rng ) {

		rng.clean_tail( );
		static_assert( not is_no_name<JsonMember::name>,
		               "Array processing should never call parse_class_member" );

		daw_json_assert_weak( rng.template in<parse_tokens::next_member_token>( ),
		                      "Expected end of class or start of member" );
		auto loc = find_class_member<JsonMember>( member_position, locations, rng );

		// If the member was found loc will have it's position
		if( loc.begin( ) == rng.begin( ) ) {
			return parse_value<JsonMember>( ParseTag<JsonMember::expected_type>{ },
			                                rng );
		}
		if( not loc.is_null( ) ) {
			return parse_value<JsonMember, true>(
			  ParseTag<JsonMember::expected_type>{ }, loc );
		}
		if constexpr( is_json_nullable_v<JsonMember> ) {
			return parse_value<JsonMember, true>(
			  ParseTag<JsonMember::expected_type>{ }, loc );
		} else {
			daw_json_error( missing_member( JsonMember::name ) );
		}
	}

	template<typename JsonClass, typename... JsonMembers, std::size_t... Is,
	         typename Range>
	[[nodiscard]] inline constexpr JsonClass
	parse_json_class( Range &rng, std::index_sequence<Is...> ) {
		static_assert( has_json_data_contract_trait_v<JsonClass>,
		               "Unexpected type" );
		static_assert(
		  can_construct_a_v<JsonClass, typename JsonMembers::parse_to_t...>,
		  "Supplied types cannot be used for	construction of this type" );

		rng.trim_left( );
		daw_json_assert_weak( rng.is_opening_brace_checked( ),
		                      "Expected start of class" );
		rng.set_class_position( );
		rng.remove_prefix( );
		rng.template move_to_next_of<parse_tokens::next_member_token>( );

		auto const cleanup_fn = [&] {
			if( not rng.has_more( ) ) {
				return;
			}
			rng.clean_tail( );
			// If we fullfill the contract before all values are parses
			daw_json_assert_weak( rng.has_more( ), "Unexpected end of range" );
			rng.template move_to_next_of<parse_tokens::next_member_token>( );
			(void)rng.skip_class( );
			// Yes this must be checked.  We maybe at the end of document.  After the
			// 2nd try, give up
			rng.trim_left_checked( );
		};

		if constexpr( sizeof...( JsonMembers ) == 0 ) {
			// Clang-CL with MSVC has issues if we don't do empties this way
			cleanup_fn( );
			return daw::construct_a<JsonClass>( );
		} else {

#if not defined( _MSC_VER ) or defined( __clang__ )
			constexpr auto known_locations_v =
			  make_locations_info<Range, JsonMembers...>( );

			auto known_locations = known_locations_v;
#else
			// MSVC is doing the murmur3 hash at compile time incorrectly
			// this puts it at runtime.
			auto known_locations = make_locations_info<Range, JsonMembers...>( );

#endif
			using tp_t = std::tuple<decltype(
			  parse_class_member<traits::nth_type<Is, JsonMembers...>>(
			    Is, known_locations, rng ) )...>;

#if defined( __cpp_constexpr_dynamic_alloc ) or                                \
  defined( DAW_JSON_NO_CONST_EXPR )
			// This relies on non-trivial dtor's being allowed.  So C++20 constexpr
			// or not in a constant expression.  It does allow for construction of
			// classes without move/copy special members

			// Do this before we exit but after return
			auto const oe = daw::on_exit_success( cleanup_fn );
			/*
			 * Rather than call directly use apply/tuple to evaluate left->right
			 */
			return std::apply(
			  daw::construct_a<JsonClass>,
			  tp_t{ parse_class_member<traits::nth_type<Is, JsonMembers...>>(
			    Is, known_locations, rng )... } );
#else
			JsonClass result = std::apply(
			  daw::construct_a<JsonClass>,
			  tp_t{ parse_class_member<traits::nth_type<Is, JsonMembers...>>(
			    Is, known_locations, rng )... } );
			cleanup_fn( );
			return result;
#endif
		}
	}

	template<typename JsonClass, typename... JsonMembers, typename Range>
	[[nodiscard]] inline constexpr JsonClass
	parse_ordered_json_class( Range &rng ) {
		static_assert( has_json_data_contract_trait_v<JsonClass>,
		               "Unexpected type" );
		static_assert(
		  can_construct_a_v<JsonClass, typename JsonMembers::parse_to_t...>,
		  "Supplied types cannot be used for	construction of this type" );

		rng.trim_left( );
		daw_json_assert_weak( rng.is_opening_bracket_checked( ),
		                      "Expected start of array" );
		rng.set_class_position( );
		rng.remove_prefix( );
		rng.trim_left( );

		auto const cleanup_fn = [&] {
			rng.clean_tail( );
			daw_json_assert_weak( rng.has_more( ), "Unexpected end of stream" );
			while( not rng.is_closing_bracket_unchecked( ) ) {
				(void)skip_value( rng );
				rng.clean_tail( );
				daw_json_assert_weak( rng.has_more( ), "Unexpected end of stream" );
			}
			// TODO: should we check for end
			daw_json_assert_weak( rng.is_closing_bracket_unchecked( ),
			                      "Expected a ']'" );
			rng.remove_prefix( );
			rng.trim_left( );
		};
		size_t current_idx = 0;
		using tp_t = std::tuple<decltype(
		  parse_ordered_class_member<JsonMembers>( current_idx, rng ) )...>;

#if defined( __cpp_constexpr_dynamic_alloc ) or                                \
  defined( DAW_JSON_NO_CONST_EXPR )
		auto const oe = daw::on_exit_success( cleanup_fn );
		return std::apply(
		  daw::construct_a<JsonClass>,
		  tp_t{ parse_ordered_class_member<JsonMembers>( current_idx, rng )... } );
#else
		JsonClass result = std::apply(
		  daw::construct_a<JsonClass>,
		  tp_t{ parse_ordered_class_member<JsonMembers>( current_idx, rng )... } );

		cleanup_fn( );
		return result;
#endif
	}
} // namespace daw::json::json_details

// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/
//

#include "apache_builds_json.h"

#include <daw/json/daw_from_json.h>

#include <string_view>

namespace daw::json {
	template apache_builds::apache_builds
	from_json<apache_builds::apache_builds,
	          SIMDNoCommentSkippingPolicyUnchecked<constexpr_exec_tag>>(
	  std::string_view json_doc, std::string_view json_data );

	template apache_builds::apache_builds
	from_json<apache_builds::apache_builds,
	          SIMDNoCommentSkippingPolicyUnchecked<constexpr_exec_tag>>(
	  std::string_view json_data );
} // namespace daw::json

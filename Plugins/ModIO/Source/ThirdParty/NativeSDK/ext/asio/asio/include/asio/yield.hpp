//
// yield.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "coroutine.hpp"

#if defined(__clang__)

# define ASIO_SUPRESS_WARNING_FALLTROUGH _Pragma("clang diagnostic push")\
										_Pragma("clang diagnostic ignored \"-Wimplicit-fallthrough\"")
# define ASIO_SUPRESS_WARNING_POP  _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)

#define ASIO_SUPRESS_WARNING_FALLTROUGH _Pragma("GCC diagnostic push")\
										_Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\"")
#define ASIO_SUPRESS_WARNING_POP _Pragma("GCC diagnostic pop")

#else
	#define ASIO_SUPRESS_WARNING_FALLTROUGH
	#define ASIO_SUPRESS_WARNING_POP
#endif

#ifndef reenter
# define reenter(c) ASIO_SUPRESS_WARNING_FALLTROUGH \
					ASIO_CORO_REENTER(c) \
					ASIO_SUPRESS_WARNING_POP
#endif

#ifndef yield
# define yield  ASIO_SUPRESS_WARNING_FALLTROUGH \
				ASIO_CORO_YIELD \
			    ASIO_SUPRESS_WARNING_POP
#endif

#ifndef fork
# define fork ASIO_CORO_FORK
#endif
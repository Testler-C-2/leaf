// Copyright (c) 2018 Emil Dotchevski
// Copyright (c) 2018 Second Spectrum, Inc.

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/leaf/handle.hpp>
#include <boost/leaf/result.hpp>
#include "boost/core/lightweight_test.hpp"

namespace leaf = boost::leaf;

////////////////////////////////

enum class api_error
{
	no_such_file_or_directory = 1,
	no_such_process = 2
};

namespace std { template<> struct is_error_condition_enum<api_error>: std::true_type { }; }

std::error_category const & api_category()
{
	class cat : public std::error_category
	{
		char const * name() const noexcept
		{
			return "api";
		}
		std::string message(int ev) const
		{
			return "API error";
		}
		bool equivalent(std::error_code const & code, int condition) const noexcept
		{
			switch( api_error(condition) )
			{
			case api_error::no_such_file_or_directory:
				return code==std::error_code(ENOENT, std::system_category());
			case api_error::no_such_process:
				return code==std::error_code(ESRCH, std::system_category());
			default:
				return false;
			}
		}
	};
	static cat c;
	return c;
}

std::error_condition make_error_condition(api_error e)
{
	return std::error_condition(int(e), api_category());
}

////////////////////////////////

template <int> struct info { int value; };

enum class not_error_code_enum
{
	ok,
	error1,
	error2,
	error3
};
namespace boost { namespace leaf {
	template <> struct is_error_type<not_error_code_enum>: std::true_type { };
} }

struct e_error_code { not_error_code_enum value; };

template <class R>
leaf::result<R> f( not_error_code_enum ec )
{
	if( ec==not_error_code_enum::ok )
		return R(42);
	else
		return leaf::new_error(ec, e_error_code{ec}, info<1>{1}, info<2>{2}, info<3>{3});
}

template <class R>
leaf::result<R> f_errc( int ec )
{
	if( ec==0 )
		return R(42);
	else
		return leaf::error_id(std::error_code(ec, std::system_category()), info<1>{1}, info<2>{2}, info<3>{3});
}

int main()
{

	// void, handle_some (success)
	{
		int c=0;
		leaf::result<void> r = leaf::handle_some(
			[&c]() -> leaf::result<void>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::ok));
				c = answer;
				return { };
			},
			[&c]( leaf::error_info const & unmatched )
			{
				BOOST_TEST(c==0);
				c = 1;
				return unmatched.error();
			} );
		BOOST_TEST(r);
		BOOST_TEST(c==42);
	}

	// void, handle_some (failure, matched)
	{
		int c=0;
		leaf::result<void> r = leaf::handle_some(
			[&c]() -> leaf::result<void>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				c = answer;
				return { };
			},
			[&c]( not_error_code_enum ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 1;
			} );
		BOOST_TEST(c==1);
		BOOST_TEST(r);
	}

	// void, handle_some (failure, matched), match api_error (single enum value)
	{
		int c=0;
		leaf::result<void> r = leaf::handle_some(
			[&c]() -> leaf::result<void>
			{
				LEAF_AUTO(answer, f_errc<int>(ENOENT));
				c = answer;
				return { };
			},
			[&c]( leaf::match<api_error,api_error::no_such_file_or_directory> ec, info<1> const & x, info<2> const & y )
			{
				BOOST_TEST(ec.value()==std::error_code(ENOENT, std::system_category()));
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 1;
			} );
		BOOST_TEST(c==1);
		BOOST_TEST(r);
	}

	// void, handle_some (failure, matched), match enum (single enum value)
	{
		int c=0;
		leaf::result<void> r = leaf::handle_some(
			[&c]() -> leaf::result<void>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				c = answer;
				return { };
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 1;
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			} );
		BOOST_TEST(c==2);
		BOOST_TEST(r);
	}

	// void, handle_some (failure, matched), match enum (multiple enum values)
	{
		int c=0;
		leaf::result<void> r = leaf::handle_some(
			[&c]() -> leaf::result<void>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				c = answer;
				return { };
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 1;
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			} );
		BOOST_TEST(c==2);
		BOOST_TEST(r);
	}

	// void, handle_some (failure, matched), match value (single value)
	{
		int c=0;
		leaf::result<void> r = leaf::handle_some(
			[&c]() -> leaf::result<void>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				c = answer;
				return { };
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 1;
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			} );
		BOOST_TEST(c==2);
		BOOST_TEST(r);
	}

	// void, handle_some (failure, matched), match value (multiple values)
	{
		int c=0;
		leaf::result<void> r = leaf::handle_some(
			[&c]() -> leaf::result<void>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				c = answer;
				return { };
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 1;
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			} );
		BOOST_TEST(c==2);
		BOOST_TEST(r);
	}

	// void, handle_some (failure, initially not matched)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( info<4> )
					{
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(!r);
				BOOST_TEST(c==0);
				return r;
			},
			[&c]( not_error_code_enum ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==2);
	}

	// void, handle_some (failure, initially not matched), match api_error (single enum value)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f_errc<int>(ENOENT));
						c = answer;
						return { };
					},
					[&c]( leaf::match<api_error, api_error::no_such_process> )
					{
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(!r);
				BOOST_TEST(c==0);
				return r;
			},
			[&c]( leaf::match<api_error, api_error::no_such_file_or_directory> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==std::error_code(ENOENT, std::system_category()));
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==2);
	}

	// void, handle_some (failure, initially not matched), match enum (single enum value)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
					{
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(!r);
				BOOST_TEST(c==0);
				return r;
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==2);
	}

	// void, handle_some (failure, initially not matched), match enum (multiple enum values)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
					{
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(!r);
				BOOST_TEST(c==0);
				return r;
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==2);
	}

	// void, handle_some (failure, initially not matched), match value (single value)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<e_error_code,not_error_code_enum::error2> )
					{
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(!r);
				BOOST_TEST(c==0);
				return r;
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==2);
	}

	// void, handle_some (failure, initially not matched), match value (multiple values)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<e_error_code,not_error_code_enum::error2> )
					{
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(!r);
				BOOST_TEST(c==0);
				return r;
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==2);
	}

	// void, handle_some (failure, initially matched)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( not_error_code_enum ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(r);
				BOOST_TEST(c==1);
				return r;
			},
			[&c]( info<4> )
			{
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==1);
	}

	// void, handle_some (failure, initially matched), match api_error (single enum value)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f_errc<int>(ENOENT));
						c = answer;
						return { };
					},
					[&c]( leaf::match<api_error, api_error::no_such_file_or_directory> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==std::error_code(ENOENT, std::system_category()));
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(r);
				BOOST_TEST(c==1);
				return r;
			},
			[&c]( leaf::match<api_error, api_error::no_such_process> )
			{
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==1);
	}

	// void, handle_some (failure, initially matched), match enum (single enum value)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(r);
				BOOST_TEST(c==1);
				return r;
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==1);
	}

	// void, handle_some (failure, initially matched), match enum (multiple enum values)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(r);
				BOOST_TEST(c==1);
				return r;
			},
			[&c]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==1);
	}

	// void, handle_some (failure, initially matched), match value (single value)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<e_error_code,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(r);
				BOOST_TEST(c==1);
				return r;
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==1);
	}

	// void, handle_some (failure, initially matched), match value (multiple values)
	{
		int c=0;
		leaf::handle_all(
			[&c]
			{
				leaf::result<void> r = leaf::handle_some(
					[&c]() -> leaf::result<void>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						c = answer;
						return { };
					},
					[&c]( leaf::match<e_error_code,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						BOOST_TEST(c==0);
						c = 1;
					} );
				BOOST_TEST(r);
				BOOST_TEST(c==1);
				return r;
			},
			[&c]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				BOOST_TEST(c==0);
				c = 2;
			},
			[&c]()
			{
				BOOST_TEST(c==0);
				c = 3;
			} );
		BOOST_TEST(c==1);
	}

	//////////////////////////////////////

	// int, handle_some (success)
	{
		leaf::result<int> r = leaf::handle_some(
			[ ]() -> leaf::result<int>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::ok));
				return answer;
			},
			[ ]( leaf::error_info const & unmatched )
			{
				return unmatched.error();
			} );
		BOOST_TEST(r && *r==42);
	}

	// int, handle_some (failure, matched)
	{
		leaf::result<int> r = leaf::handle_some(
			[ ]() -> leaf::result<int>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				return answer;
			},
			[ ]( not_error_code_enum ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 1;
			} );
		BOOST_TEST(r && *r==1);
	}

	// int, handle_some (failure, matched), match api_error (single enum value)
	{
		leaf::result<int> r = leaf::handle_some(
			[ ]() -> leaf::result<int>
			{
				LEAF_AUTO(answer, f_errc<int>(ENOENT));
				return answer;
			},
			[ ]( leaf::match<api_error, api_error::no_such_process> )
			{
				return 1;
			},
			[ ]( leaf::match<api_error, api_error::no_such_file_or_directory> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==std::error_code(ENOENT, std::system_category()));
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			} );
		BOOST_TEST(r && *r==2);
	}

	// int, handle_some (failure, matched), match enum (single enum value)
	{
		leaf::result<int> r = leaf::handle_some(
			[ ]() -> leaf::result<int>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				return answer;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				return 1;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			} );
		BOOST_TEST(r && *r==2);
	}

	// int, handle_some (failure, matched), match enum (multiple enum values)
	{
		leaf::result<int> r = leaf::handle_some(
			[ ]() -> leaf::result<int>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				return answer;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				return 1;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			} );
		BOOST_TEST(r && *r==2);
	}

	// int, handle_some (failure, matched), match value (single value)
	{
		leaf::result<int> r = leaf::handle_some(
			[ ]() -> leaf::result<int>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				return answer;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				return 1;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			} );
		BOOST_TEST(r && *r==2);
	}

	// int, handle_some (failure, matched), match value (multiple values)
	{
		leaf::result<int> r = leaf::handle_some(
			[ ]() -> leaf::result<int>
			{
				LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
				return answer;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				return 1;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			} );
		BOOST_TEST(r && *r==2);
	}

	// int, handle_some (failure, initially not matched)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( info<4> )
					{
						return 1;
					} );
				BOOST_TEST(!r);
				return r;
			},
			[ ]( not_error_code_enum ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==2);
	}

	// int, handle_some (failure, initially not matched), match api_error (single enum value)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f_errc<int>(ENOENT));
						return answer;
					},
					[ ]( leaf::match<api_error, api_error::no_such_process> )
					{
						return 1;
					} );
				BOOST_TEST(!r);
				return r;
			},
			[ ]( leaf::match<api_error, api_error::no_such_file_or_directory> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==std::error_code(ENOENT, std::system_category()));
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==2);
	}

	// int, handle_some (failure, initially not matched), match enum (single enum value)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
					{
						return 1;
					} );
				BOOST_TEST(!r);
				return r;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==2);
	}

	// int, handle_some (failure, initially not matched), match enum (multiple enum values)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
					{
						return 1;
					} );
				BOOST_TEST(!r);
				return r;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==2);
	}

	// int, handle_some (failure, initially not matched), match value (single value)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<e_error_code,not_error_code_enum::error2> )
					{
						return 1;
					} );
				BOOST_TEST(!r);
				return r;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==2);
	}

	// int, handle_some (failure, initially not matched), match value (multiple values)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<e_error_code,not_error_code_enum::error2> )
					{
						return 1;
					} );
				BOOST_TEST(!r);
				return r;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
			{
				BOOST_TEST(ec.value()==not_error_code_enum::error1);
				BOOST_TEST(x.value==1);
				BOOST_TEST(y.value==2);
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==2);
	}

	// int, handle_some (failure, initially matched)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( not_error_code_enum ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						return 1;
					} );
				BOOST_TEST(r);
				return r;
			},
			[ ]( info<4> )
			{
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==1);
	}

	// int, handle_some (failure, initially matched), match enum (single enum value)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						return 1;
					} );
				BOOST_TEST(r);
				return r;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==1);
	}

	// int, handle_some (failure, initially matched), match enum (multiple enum values)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						return 1;
					} );
				BOOST_TEST(r);
				return r;
			},
			[ ]( leaf::match<not_error_code_enum,not_error_code_enum::error2> )
			{
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==1);
	}

	// int, handle_some (failure, initially matched), match value (single value)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<e_error_code,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						return 1;
					} );
				BOOST_TEST(r);
				return r;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==1);
	}

	// int, handle_some (failure, initially matched), match value (multiple values)
	{
		int r = leaf::handle_all(
			[ ]
			{
				leaf::result<int> r = leaf::handle_some(
					[ ]() -> leaf::result<int>
					{
						LEAF_AUTO(answer, f<int>(not_error_code_enum::error1));
						return answer;
					},
					[ ]( leaf::match<e_error_code,not_error_code_enum::error2,not_error_code_enum::error1> ec, info<1> const & x, info<2> y )
					{
						BOOST_TEST(ec.value()==not_error_code_enum::error1);
						BOOST_TEST(x.value==1);
						BOOST_TEST(y.value==2);
						return 1;
					} );
				BOOST_TEST(r);
				return r;
			},
			[ ]( leaf::match<e_error_code,not_error_code_enum::error2> )
			{
				return 2;
			},
			[ ]
			{
				return 3;
			} );
		BOOST_TEST(r==1);
	}

	return boost::report_errors();
}

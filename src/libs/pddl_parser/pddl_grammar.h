
/***************************************************************************
 *  grammar.h
 *
 *  Created: Fri 19 May 2017 14:07:29 CEST
 *  Copyright  2017  Matthias Loebach
 *                   Till Hofmann
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#ifndef PLUGINS_PDDL_GRAMMAR_H_
#define PLUGINS_PDDL_GRAMMAR_H_

#include "pddl_ast.h"
#include "pddl_semantics.h"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>

namespace px = boost::phoenix;
namespace qr = boost::spirit::repository::qi;

namespace pddl_parser {
namespace grammar {
/** @class pddl_skipper
     * A skipper for PDDL files.
     * This skipper skips spaces and comments starting with ';'
     */
template <typename Iterator>
struct pddl_skipper : public qi::grammar<Iterator>
{
	pddl_skipper() : pddl_skipper::base_type(skip, "PDDL")
	{
		skip = ascii::space | (';' >> *(qi::char_ - qi::eol));
	}
	/** The actual skipping rule. */
	qi::rule<Iterator> skip;
};

/** @class domain_parser
     * A Boost QI parser for a PDDL domain.
     */
template <typename Iterator, typename Skipper = pddl_skipper<Iterator>>
struct domain_parser : qi::grammar<Iterator, Domain(), Skipper>
{
	domain_parser() : domain_parser::base_type(domain)
	{
		using namespace qi;
		using ascii::alnum;
		using ascii::blank;
		using ascii::char_;

		name_type = lexeme[alnum > *(alnum | char_('-') | char_('_'))];

		domain_name = lit("define") > '(' > lit("domain") > +(char_ - ')') > ')';

		requirements = '(' > lit(":requirements") > *(':' > lexeme[*qi::alnum]) > ')';

		type_pair = name_type > -('-' > name_type);
		types     = '(' > lit(":types") > +type_pair > ')';

		constant_value_list = +name_type;
		constant_multi_pair = constant_value_list > -('-' > name_type);
		constants           = '(' > lit(":constants") > +constant_multi_pair > ')';

		param_pair  = '?' > name_type > '-' > name_type;
		param_pairs = +param_pair;
		pred        = '(' > name_type > -param_pairs > ')';
		predicates  = '(' > lit(":predicates") > +pred > ')';


		atom    = +(graph - '(' - ')');
		bool_op = qi::string("and") | qi::string("or") | qi::string("not");
		comparison_op =
		  qi::string("<") | qi::string(">") | qi::string("=") | qi::string("<=") | qi::string(">=");
		numerical_op = qi::string("+") | qi::string("-") | qi::string("/") | qi::string("*")
		               | qi::string("=") | string("increase") | string("decrease");

		// no expectation parsing to allow proper backtracking
		numerical_expression = attr(ExpressionType::VALUE) >> qi::as_string[qi::float_];
		pred_expression      = attr(ExpressionType::PREDICATE)
		                  >> qi::as<Predicate>()[atom >> *(hold[attr(ExpressionType::ATOM) >> atom])];
		bool_expression = attr(ExpressionType::BOOL) >> qi::as<Predicate>()[(bool_op >> +expression)];

		// hold to backtrack the ExpressionType
		expression = '(' >> hold[(hold[bool_expression] |
		                          //	hold[fluent_expression] | hold[fluent_change_expression] |
		                          hold[pred_expression])]
		             >> ')';
		temp_breakup  = lit(":temporal-breakup") > expression;
		cond_breakup  = lit(":conditional-breakup") > expression;
		effects       = lit(":effect") > expression;
		preconditions = lit(":precondition") > expression;
		duration      = lit(":duration") > '(' > '=' > lit("?duration") > uint_ > ')';
		action_params = lit(":parameters") > '(' > +param_pair > ')';

		// validate action semantics after parsing
		action =
		  ('(' >> qr::iter_pos
		   >> qi::as<Action>()[(lit(":durative-action") | lit(":action")) > name_type > action_params
		                       > -duration > preconditions > effects > -cond_breakup > -temp_breakup
		                       > ')'])[_val = action_semantics_(qi::_1, qi::_2, qi::_r1)];
		// pass down the domain for semantics check
		actions = +(action(qi::_r1));

		domain = '(' > domain_name > requirements > -types > -constants > predicates > -fluents
		         > actions(qi::_val) // pass down the domain for semantic check
		         // make closing parenthesis optional to stay backwards compatible
		         > -lit(")");
	}

private:
	/** Semantic checks for each parsed action. */
	px::function<pddl_parser::ActionSemantics> action_semantics_;
	/** Named placeholder for parsing a name. */
	qi::rule<Iterator, std::string(), Skipper> name_type;

	/** Named placeholder for parsing a domain name. */
	qi::rule<Iterator, std::string(), Skipper> domain_name;

	/** Named placeholder for parsing requirements. */
	qi::rule<Iterator, std::vector<std::string>(), Skipper> requirements;

	/** Named placeholder for parsing types. */
	qi::rule<Iterator, pairs_type(), Skipper> types;
	/** Named placeholder for parsing type pairs. */
	qi::rule<Iterator, pair_type(), Skipper> type_pair;

	/** Named placeholder for parsing a list of constant values. */
	qi::rule<Iterator, type_list(), Skipper> constant_value_list;
	/** Named placeholder for parsing a list of predicate parameters. */
	qi::rule<Iterator, type_list(), Skipper> predicate_params;
	/** Named placeholder for parsing a list of typed constants. */
	qi::rule<Iterator, pair_multi_const(), Skipper> constant_multi_pair;
	/** Named placeholder for parsing a list of constants. */
	qi::rule<Iterator, pairs_multi_consts(), Skipper> constants;

	/** Named placeholder for parsing a parameter pair. */
	qi::rule<Iterator, string_pair_type(), Skipper> param_pair;
	/** Named placeholder for parsing a list of parameter pairs. */
	qi::rule<Iterator, string_pairs_type(), Skipper> param_pairs;
	/** Named placeholder for parsing a predicate type. */
	qi::rule<Iterator, predicate_type(), Skipper> pred;
	/** Named placeholder for parsing a list of predicate types. */
	qi::rule<Iterator, std::vector<predicate_type>(), Skipper> predicates;

	/** Named placeholder for parsing any atom. */
	qi::rule<Iterator, Atom()> atom;
	/** Named placeholder for parsing an atom that is a logical operator. */
	qi::rule<Iterator, Atom()> bool_op;
	/** Named placeholder for parsing an atom that is a comparison operator. */
	qi::rule<Iterator, Atom()> comparison_op;
	/** Named placeholder for parsing an atom that is a numerical operator. */
	qi::rule<Iterator, Atom()> numerical_op;
	/** Named placeholder for parsing a predicate. */
	qi::rule<Iterator, Predicate(), Skipper> predicate;
	/** Named placeholder for parsing a PDDL predicate expression. */
	qi::rule<Iterator, Expression(), Skipper> pred_expression;
	/** Named placeholder for parsing a PDDL numeric expression. */
	qi::rule<Iterator, Expression(), Skipper> numerical_expression;
	/** Named placeholder for parsing a PDDL bool expression. */
	qi::rule<Iterator, Expression(), Skipper> bool_expression;
	/** Named placeholder for parsing a PDDL expression. */
	qi::rule<Iterator, Expression(), Skipper> expression;
	/** Named placeholder for parsing a PDDL precondition. */
	qi::rule<Iterator, Expression(), Skipper> preconditions;
	/** Named placeholder for parsing a PDDL effect. */
	qi::rule<Iterator, Expression(), Skipper> effects;
	/** Named placeholder for parsing a temporal breakup. */
	qi::rule<Iterator, Expression(), Skipper> temp_breakup;
	/** Named placeholder for parsing a conditional breakup. */
	qi::rule<Iterator, Expression(), Skipper> cond_breakup;
	/** Named placeholder for parsing an action duration. */
	qi::rule<Iterator, int(), Skipper> duration;
	/** Named placeholder for parsing action parameters. */
	qi::rule<Iterator, string_pairs_type(), Skipper> action_params;
	/** Named placeholder for parsing an action. Inherits a domain. */
	qi::rule<Iterator, Action(const Domain &), Skipper> action;
	/** Named placeholder for parsing a fluent type. */
	qi::rule<Iterator, Fluent(), Skipper> fluent;
	/** Named placeholder for parsing a list of fluents. */
	qi::rule<Iterator, std::vector<Fluent>(), Skipper> fluents;
	/** Named placeholder for parsing a list of actions. */
	qi::rule<Iterator, std::vector<Action>(const Domain &), Skipper> actions;

	/** Named placeholder for parsing a domain. */
	qi::rule<Iterator, Domain(), Skipper> domain;
};

/** @class problem_parser
     * A Boost QI parser for a PDDL problem.
     */
template <typename Iterator, typename Skipper = pddl_skipper<Iterator>>
struct problem_parser : qi::grammar<Iterator, Problem(), Skipper>
{
	problem_parser() : problem_parser::base_type(problem)
	{
		using namespace qi;
		using ascii::alnum;
		using ascii::blank;
		using ascii::char_;

		name_type = lexeme[alnum > *(alnum | char_('-') | char_('_'))];

		problem_name = '(' > lit("define") > '(' > lit("problem") > name_type > ')';

		domain_name = '(' > lit(":domain") > name_type > ')';

		constant_value_list = +name_type;
		constant_multi_pair = constant_value_list > -('-' > name_type);
		objects             = '(' > lit(":objects") > +constant_multi_pair > ')';

		atom       = +(graph - '(' - ')');
		predicate  = '(' > atom > *expression > ')';
		expression = attr(ExpressionType::PREDICATE) > predicate;
		init       = '(' > lit(":init") > +expression > ')';

		goal = '(' > lit(":goal") > expression > ')';

		problem = problem_name > domain_name > objects > init > goal;
	}

	/** Named placeholder for parsing a name. */
	qi::rule<Iterator, std::string(), Skipper> name_type;

	/** Named placeholder for parsing a problem name. */
	qi::rule<Iterator, std::string(), Skipper> problem_name;
	/** Named placeholder for parsing a domain name. */
	qi::rule<Iterator, std::string(), Skipper> domain_name;

	/** Named placeholder for parsing a list of constant values. */
	qi::rule<Iterator, type_list(), Skipper> constant_value_list;
	/** Named placeholder for parsing a list of predicate parameters. */
	qi::rule<Iterator, type_list(), Skipper> predicate_params;
	/** Named placeholder for parsing a list of typed constants. */
	qi::rule<Iterator, pair_multi_const(), Skipper> constant_multi_pair;
	/** Named placeholder for parsing a list of domain objects. */
	qi::rule<Iterator, pairs_multi_consts(), Skipper> objects;

	/** Named placeholder for parsing an atom. */
	qi::rule<Iterator, Atom()> atom;
	/** Named placeholder for parsing a predicate. */
	qi::rule<Iterator, Predicate(), Skipper> predicate;
	/** Named placeholder for parsing a PDDL expression. */
	qi::rule<Iterator, Expression(), Skipper> expression;
	/** Named placeholder for parsing a PDDL goal. */
	qi::rule<Iterator, Expression(), Skipper> goal;
	/** Named placeholder for parsing the initial state. */
	qi::rule<Iterator, std::vector<Expression>(), Skipper> init;

	/** Named placeholder for parsing a PDDL problem. */
	qi::rule<Iterator, Problem(), Skipper> problem;
};

} // namespace grammar
} // namespace pddl_parser

#endif

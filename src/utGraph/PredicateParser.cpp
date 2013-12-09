/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the 
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

/**
 * @ingroup srg_algorithms
 * @file
 * UTQL predicate parser implementation
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

//#define BOOST_SPIRIT_DEBUG
//#define BOOST_SPIRIT_DUMP_XML

#include <algorithm>

#include <boost/version.hpp>
#if BOOST_VERSION >= 104900

#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_grammar.hpp>
#include <boost/spirit/include/classic_grammar_def.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#ifdef BOOST_SPIRIT_DUMP_XML
#include <string.h>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#endif

#else

#include <boost/spirit.hpp>
#include <boost/spirit/utility/grammar_def.hpp>
#include <boost/spirit/tree/ast.hpp>
#ifdef BOOST_SPIRIT_DUMP_XML
#include <string.h>
#include <boost/spirit/tree/tree_to_xml.hpp>
#endif

#endif

#include <utUtil/Exception.h>
#include "PredicateParser.h"
#include "AttributeExpression.h"

#if BOOST_VERSION >= 104900
namespace spirit = BOOST_SPIRIT_CLASSIC_NS;
#else
namespace spirit = boost::spirit;
#endif

namespace Ubitrack { namespace Graph {

// define anonymous namespace for implementation details
namespace {

using namespace spirit;

/* defines a grammar for the predicate parser */
struct PredicateGrammar
	: public grammar< PredicateGrammar >
{
	// define unique ids to identify rules
	enum { attributeId=1, constantId, functionId, valueId, expExpressionId, multExpressionId, expressionId, compOpId, statementId, predicateId, predicateFunctionId };

	template< typename ScannerT >
	struct definition
		: public grammar_def
		<
			rule< ScannerT, parser_context<>, parser_tag< predicateId > >,
			rule< ScannerT, parser_context<>, parser_tag< expressionId > >
		>
	{
		// here the actual rules are defined
		definition( const PredicateGrammar& self )
		{
			#if SPIRIT_VERSION >= 0x1805
				attribute = reduced_node_d[ !( alpha_p >> *( alnum_p | ch_p( '_' ) ) >> ch_p( '.' ) ) >> alpha_p >> *( alnum_p | ch_p( '_' ) ) ];
				constant = reduced_node_d[ inner_node_d[ confix_p( '"', *c_escape_ch_p, '"' ) | confix_p( '\'', *c_escape_ch_p, '\'' ) ] | real_p ];
				function = root_node_d[ reduced_node_d[ alpha_p >> *( alnum_p | ch_p( '_' ) ) ] ] >> 
					infix_node_d[ inner_node_d[ confix_p( '(', !list_p( expression, ch_p( ',' ) ), ')' ) ] ];
				predicateFunction = root_node_d[ reduced_node_d[ alpha_p >> *( alnum_p | ch_p( '_' ) ) ] ] >> 
					infix_node_d[ inner_node_d[ confix_p( '(', !list_p( expression, ch_p( ',' ) ), ')' ) ] ];
			#else
				attribute = leaf_node_d[ !( alpha_p >> *( alnum_p | ch_p( '_' ) ) >> ch_p( '.' ) ) >> alpha_p >> *( alnum_p | ch_p( '_' ) ) ];
				constant = leaf_node_d[ inner_node_d[ confix_p( '"', *c_escape_ch_p, '"' ) ] | inner_node_d[ confix_p( '\'', *c_escape_ch_p, '\'' ) ] | real_p ];
				function = root_node_d[ leaf_node_d[ alpha_p >> *( alnum_p | ch_p( '_' ) ) ] ] >> 
					infix_node_d[ inner_node_d[ confix_p( '(', !list_p( expression, ch_p( ',' ) ), ')' ) ] ];
				predicateFunction = root_node_d[ leaf_node_d[ alpha_p >> *( alnum_p | ch_p( '_' ) ) ] ] >> 
					infix_node_d[ inner_node_d[ confix_p( '(', !list_p( expression, ch_p( ',' ) ), ')' ) ] ];
			#endif
			value = function | attribute | constant | inner_node_d[ ch_p( '(' ) >> expression >> ch_p( ')' ) ];
			expExpression = value >> !( root_node_d[ ch_p( '^' ) ] >> value );
			multExpression = 
				( root_node_d[ ch_p('-') ] >> multExpression )
				| ( expExpression >> *( root_node_d[ ch_p( '*' ) | ch_p( '/' ) ] >> expExpression ) );
			expression = 
				( multExpression >> *( root_node_d[ ch_p( '+' ) | ch_p( '-' ) ] >> multExpression ) );
			compOp = str_p( "==" ) | str_p( "!=" ) | str_p( ">=" ) | str_p( ">" ) | str_p( "<=" ) | str_p( "<" );
			statement = 
				( expression >> root_node_d[ compOp ] >> expression ) 
				| predicateFunction
				| inner_node_d[ confix_p( '(', predicate, ')' ) ] 
				| ( root_node_d[ ch_p( '!' ) ] >> statement );
			predicate = statement >> *( root_node_d[ str_p( "&&" ) | str_p( "||" ) ] >> statement );
			
			this->start_parsers( predicate, expression );
		}
		
		// the rules
		rule< ScannerT, parser_context<>, parser_tag< attributeId > > attribute;
		rule< ScannerT, parser_context<>, parser_tag< constantId > > constant;
		rule< ScannerT, parser_context<>, parser_tag< functionId > > function;
		rule< ScannerT, parser_context<>, parser_tag< valueId > > value;
		rule< ScannerT, parser_context<>, parser_tag< expExpressionId > > expExpression;
		rule< ScannerT, parser_context<>, parser_tag< multExpressionId > > multExpression;
		rule< ScannerT, parser_context<>, parser_tag< expressionId > > expression;
		rule< ScannerT, parser_context<>, parser_tag< compOpId > > compOp;
		rule< ScannerT, parser_context<>, parser_tag< statementId > > statement;
		rule< ScannerT, parser_context<>, parser_tag< predicateId > > predicate;
		rule< ScannerT, parser_context<>, parser_tag< predicateFunctionId > > predicateFunction;
	};
};

// define one static instance of the parser
PredicateGrammar predicateGrammar;

// typedef for tree iterators
typedef tree_match< const char* >::tree_iterator TreeIter;


// creates a value object from an AST parse tree
boost::shared_ptr< AttributeExpression > createValue( const TreeIter& it )
{
	std::string value( it->value.begin(), it->value.end() );
	
	// constants
	if ( it->value.id() == PredicateGrammar::constantId )
		return boost::shared_ptr< AttributeExpression >( new AttributeExpressionConstant( value ) );
	// attribute names
	else if ( it->value.id() == PredicateGrammar::attributeId )
		return boost::shared_ptr< AttributeExpression >( new AttributeExpressionAttribute( value ) );
	// +/- operations
	else if ( it->value.id() == PredicateGrammar::expressionId && it->children.size() == 2 )
	{
		if ( value == "+" )
			return boost::shared_ptr< AttributeExpression >( new AttributeExpressionBinary< std::plus< double > >(
				createValue( it->children.begin() ), createValue( it->children.begin() + 1 ) ) );
		else if ( value == "-" )
			return boost::shared_ptr< AttributeExpression >( new AttributeExpressionBinary< std::minus< double > >( 
				createValue( it->children.begin() ), createValue( it->children.begin() + 1 ) ) );
		else
			UBITRACK_THROW( "invalid operator: " + value );
	}
	// *// operations
	else if ( it->value.id() == PredicateGrammar::multExpressionId && it->children.size() == 2 )
	{
		if ( value == "*" )
			return boost::shared_ptr< AttributeExpression >( new AttributeExpressionBinary< std::multiplies< double > >(
				createValue( it->children.begin() ), createValue( it->children.begin() + 1 ) ) );
		else if ( value == "/" )
			return boost::shared_ptr< AttributeExpression >( new AttributeExpressionBinary< std::divides< double > >(
				createValue( it->children.begin() ), createValue( it->children.begin() + 1 ) ) );
		else
			UBITRACK_THROW( "invalid operator: " + value );
	}
	// unary -
	else if ( it->value.id() == PredicateGrammar::multExpressionId && it->children.size() == 1 )
		return boost::shared_ptr< AttributeExpression >( new AttributeExpressionUnary< std::negate< double > >(
			createValue( it->children.begin() ) ) );
	// exponentials
	else if ( it->value.id() == PredicateGrammar::expExpressionId && it->children.size() == 2 )
		return boost::shared_ptr< AttributeExpression >( 
			new AttributeExpressionBinary< std::pointer_to_binary_function< double, double, double > >(
				createValue( it->children.begin() ), createValue( it->children.begin() + 1 ), 
				std::pointer_to_binary_function< double, double, double >( &::pow ) ) );
	// functions
	else if ( it->value.id() == PredicateGrammar::functionId )
	{
		// parse parameters into vector
		std::vector< boost::shared_ptr< AttributeExpression > > params;
		for ( TreeIter itParam = it->children.begin(); itParam != it->children.end(); itParam++ )
			params.push_back( createValue( itParam ) );

		// check for some well-known functions
		if ( value == "sqrt" )
		{
			if ( params.size() != 1 )
			{ UBITRACK_THROW( "Invalid number of arguments to function " + value ); }
			else
				return boost::shared_ptr< AttributeExpression >( 
					new AttributeExpressionUnary< std::pointer_to_unary_function< double, double > >(
						params[ 0 ], std::pointer_to_unary_function< double, double >( &::sqrt ) ) );
		}
		else if ( value == "max" )
		{
			if ( params.size() != 2 )
			{ UBITRACK_THROW( "Invalid number of arguments to function " + value ); }
			else
				return boost::shared_ptr< AttributeExpression >( 
					new AttributeExpressionBinary< std::pointer_to_binary_function< const double&, const double&, const double& > >(
						params[ 0 ], params[ 1 ], 
						std::pointer_to_binary_function< const double&, const double&, const double& >( &std::max ) ) );
		}
		else if ( value == "min" )
		{
			if ( params.size() != 2 )
			{ UBITRACK_THROW( "Invalid number of arguments to function " + value ); }
			else
				return boost::shared_ptr< AttributeExpression >( 
					new AttributeExpressionBinary< std::pointer_to_binary_function< const double&, const double&, const double& > >(
						params[ 0 ], params[ 1 ], 
						std::pointer_to_binary_function< const double&, const double&, const double& >( &std::min ) ) );
		}
		else
			// takes care of other functions
			return boost::shared_ptr< AttributeExpression >( new AttributeExpressionFunction( value, params ) );
	}
	// spirit prunes empty constants out of the parse tree
	else if ( it->children.size() == 0 ) 
		return boost::shared_ptr< AttributeExpression >( new AttributeExpressionConstant( "" ) );
	else
		UBITRACK_THROW( "Invalid expression: " + value );
}


// creates a predicate object from an AST parse tree
boost::shared_ptr< Predicate > createPredicate( const TreeIter& it )
{
	std::string op( it->value.begin(), it->value.end() );
	
	if ( it->value.id() == PredicateGrammar::predicateId )
	{
		if ( op == "&&" )
			return boost::shared_ptr< Predicate >( new PredicateAnd( 
				createPredicate( it->children.begin() ), createPredicate( it->children.begin() + 1 ) ) );
		else if ( op == "||" )
			return boost::shared_ptr< Predicate >( new PredicateOr( 
				createPredicate( it->children.begin() ), createPredicate( it->children.begin() + 1 ) ) );
		else
			UBITRACK_THROW( "Bad predicate: " + op );
	}
	else if ( it->value.id() == PredicateGrammar::statementId )
	{
		if ( op == "!" )
			return boost::shared_ptr< Predicate >( new PredicateNot( createPredicate( it->children.begin() ) ) );
		else
			UBITRACK_THROW( "Bad statement: " + op );
	}
	else if ( it->value.id() == PredicateGrammar::compOpId )
	{
		if ( it->children.size() == 2 )
			return boost::shared_ptr< Predicate >( new PredicateCompare( op, 
				createValue( it->children.begin() ), createValue( it->children.begin() + 1 ) ) );

		// hack: spirit prunes empty string constants out of the tree
		if ( it->children.size() == 1 )
			return boost::shared_ptr< Predicate >( new PredicateCompare( op, 
				createValue( it->children.begin() ), 
				boost::shared_ptr< AttributeExpression >( new AttributeExpressionConstant( "" ) ) ) );
		
		UBITRACK_THROW( "Problem with comparison parsing: illegal numer of children" );
	}
	else if ( it->value.id() == PredicateGrammar::predicateFunctionId )
	{
		// parse parameters into vector
		std::vector< boost::shared_ptr< AttributeExpression > > params;
		for ( TreeIter itParam = it->children.begin(); itParam != it->children.end(); itParam++ )
			params.push_back( createValue( itParam ) );

		return boost::shared_ptr< Predicate >( new PredicateFunction( op, params ) );
	}
	else
	{
		UBITRACK_THROW( "bad predicate" );
	}
}

} // anonymous namespace


boost::shared_ptr< Predicate > parsePredicate( const std::string& sPredicate )
{
	spirit::tree_parse_info<> info = spirit::ast_parse( sPredicate.c_str(), 
		predicateGrammar.use_parser< 0 >(), space_p );
	
	if ( info.full )
	{
#ifdef BOOST_SPIRIT_DUMP_XML
		// dump parse tree as XML
		std::map< spirit::parser_id, std::string > ruleNames;
		ruleNames[ PredicateGrammar::attributeId ] = "attribute";
		ruleNames[ PredicateGrammar::constantId ] = "constant";
		ruleNames[ PredicateGrammar::functionId ] = "function";
		ruleNames[ PredicateGrammar::valueId ] = "value";
		ruleNames[ PredicateGrammar::expExpressionId ] = "expExpression";
		ruleNames[ PredicateGrammar::multExpressionId ] = "multExpression";
		ruleNames[ PredicateGrammar::expressionId ] = "expression";
		ruleNames[ PredicateGrammar::compOpId ] = "compOp";
		ruleNames[ PredicateGrammar::statementId ] = "statement";
		ruleNames[ PredicateGrammar::predicateId ] = "predicate";
		ruleNames[ PredicateGrammar::predicateFunctionId ] = "predicateFunction";
		tree_to_xml( std::cout, info.trees, sPredicate.c_str(), ruleNames );
#endif
		
		return createPredicate( info.trees.begin() );
	}
	else
		UBITRACK_THROW( "Syntax error in predicate: " + std::string( info.stop ) );
}


boost::shared_ptr< AttributeExpression > parseAttributeExpression( const std::string& sExpression )
{
	spirit::tree_parse_info<> info = spirit::ast_parse( sExpression.c_str(), 
		predicateGrammar.use_parser< 1 >(), space_p );
	
	if ( info.full )
	{
#ifdef BOOST_SPIRIT_DUMP_XML
		// dump parse tree as XML
		std::map< spirit::parser_id, std::string > ruleNames;
		ruleNames[ PredicateGrammar::attributeId ] = "attribute";
		ruleNames[ PredicateGrammar::constantId ] = "constant";
		ruleNames[ PredicateGrammar::functionId ] = "function";
		ruleNames[ PredicateGrammar::valueId ] = "value";
		ruleNames[ PredicateGrammar::expExpressionId ] = "expExpression";
		ruleNames[ PredicateGrammar::multExpressionId ] = "multExpression";
		ruleNames[ PredicateGrammar::expressionId ] = "expression";
		tree_to_xml( std::cout, info.trees, sExpression.c_str(), ruleNames );
#endif
		
		return createValue( info.trees.begin() );
	}
	else
		UBITRACK_THROW( "Syntax error in attribute expression >>" + sExpression + "<<: " + std::string( info.stop ) );
}


} } // namespace Ubitrack::Graph


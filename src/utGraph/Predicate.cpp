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
 * UTQL Predicates implementation
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include "Predicate.h"
#include "AttributeExpression.h"
#include "KeyValueAttributes.h"
#include "UTQLSubgraph.h"
#include "EvaluationContext.h"

namespace Ubitrack { namespace Graph {

PredicateNot::PredicateNot( const boost::shared_ptr< Predicate > pChild )
	: m_pChild( pChild )
{
}


bool PredicateNot::evaluate( const EvaluationContext& c ) const
{
	return !m_pChild->evaluate( c );
}


PredicateAnd::PredicateAnd( const boost::shared_ptr< Predicate > pChild1, const boost::shared_ptr< Predicate > pChild2 )
	: m_pChild1( pChild1 )
	, m_pChild2( pChild2 )
{
}


bool PredicateAnd::evaluate( const EvaluationContext& c ) const
{
	return m_pChild1->evaluate( c ) && m_pChild2->evaluate( c );
}


Predicate::AttribList PredicateAnd::getConjunctiveEqualities() const
{
	AttribList attribs;

	AttribList a = m_pChild1->getConjunctiveEqualities();
	attribs.splice( attribs.end(), a );

	a = m_pChild2->getConjunctiveEqualities();
	attribs.splice( attribs.end(), a );

	return attribs;
}


PredicateOr::PredicateOr( const boost::shared_ptr< Predicate > pChild1, const boost::shared_ptr< Predicate > pChild2 )
	: m_pChild1( pChild1 )
	, m_pChild2( pChild2 )
{
}


bool PredicateOr::evaluate( const EvaluationContext& c ) const
{
	return m_pChild1->evaluate( c ) || m_pChild2->evaluate( c );
}


PredicateCompare::PredicateCompare( const std::string& op,
	const boost::shared_ptr< AttributeExpression > pChild1, const boost::shared_ptr< AttributeExpression > pChild2 )
	: m_pChild1( pChild1 )
	, m_pChild2( pChild2 )
{
	if ( op == "==" )
		m_type = equals;
	else if ( op == "!=" )
		m_type = notEquals;
	else if ( op == ">" )
		m_type = greater;
	else if ( op == ">=" )
		m_type = greaterEquals;
	else if ( op == "<" )
		m_type = less;
	else if ( op == "<=" )
		m_type = lessEquals;
	else
		UBITRACK_THROW( "Bad comparison operator: " + op );
}


bool PredicateCompare::evaluate( const EvaluationContext& c ) const
{
	return doCompare( m_pChild1->evaluate( c ), m_pChild2->evaluate( c ) );
}


bool PredicateCompare::doCompare( const AttributeValue& a, const AttributeValue& b ) const
{
	if ( m_type == equals )
	{
		if ( a.isNumber() )
		{
			if ( b.isNumber() )
				return a.getNumber() == b.getNumber();
			else
				return false;
		}
		else
			return a.getText() == b.getText();
	}
	else if ( m_type == notEquals )
	{
		if ( a.isNumber() )
			if ( b.isNumber() )
				return a.getNumber() != b.getNumber();
			else
				return true;
		else
			return a.getText() != b.getText();
	}
	else if ( m_type == greater )
		return a.getNumber() > b.getNumber();
	else if ( m_type == greaterEquals )
		return a.getNumber() >= b.getNumber();
	else if ( m_type == less )
		return a.getNumber() < b.getNumber();
	else if ( m_type == lessEquals )
		return a.getNumber() <= b.getNumber();

	return false;
}


Predicate::AttribList PredicateCompare::getConjunctiveEqualities() const
{
	if ( m_type == equals )
	{
		try
		{
			// only expressions of the form "<attribute>==<constant>" are supported
			AttribList l;
			l.push_back( AttribList::value_type(
				dynamic_cast< const AttributeExpressionAttribute& >( *m_pChild1 ).getName(),
				dynamic_cast< const AttributeExpressionConstant& >( *m_pChild2 ).getValue()
			) );

			return l;
		}
		catch (...)
		{}
	}

	return AttribList();
}


PredicateFunction::PredicateFunction( const std::string& f,
	const std::vector< boost::shared_ptr< AttributeExpression > >& pChildren )
{
	if ( f == "inSourceSet" )
	{
		m_type = inSourceSet;
		if ( pChildren.size() != 1 )
			UBITRACK_THROW( "inSourceSet must have exactly one argument" );
		m_pChild1 = pChildren[ 0 ];
	}
	else
		UBITRACK_THROW( "Bad predicate function operator: " + f );
}


bool PredicateFunction::evaluate( const EvaluationContext& c ) const
{
	if ( m_type == inSourceSet )
	{
		// inSourceSet returns true if the set of source edges contains a pattern whose ID starts with the given string
		if ( c.isGlobal() )
			UBITRACK_THROW( "inSourceSet can only be used in edge predicates!" );
			
		std::string query = m_pChild1->evaluate( c ).getText();
		const std::set< std::string >& sources( c.getNodeEdgePatternAttributes()->m_InformationSources );
		std::set< std::string >::const_iterator it = sources.lower_bound( query );
		
		return it != sources.end() && it->size() >= query.size() && !it->compare( 0, query.size(), query );
	}
	
	return true;
}

} } // namespace Ubitrack::Graph


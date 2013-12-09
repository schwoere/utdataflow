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
 * UTQL Predicates header file
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Graph_Predicate_H_INCLUDED__
#define __Ubitrack_Graph_Predicate_H_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <utDataflow.h>
#include "AttributeValue.h"

namespace Ubitrack { namespace Graph {

// forward declarations
class KeyValueAttributes;
class AttributeExpression;
class UTQLSubgraph;
class EvaluationContext;

/**
 * Virtual base class of all predicates
 */
class UTDATAFLOW_EXPORT Predicate
{
public:
	/** list of attributes */
	typedef std::list< std::pair< std::string, std::string > > AttribList;

	/** Virtual destructor. One should always have one in his basement. */
	virtual ~Predicate()
	{}

	/** virtual method to evaluate the predicate on an attribute list */
	virtual bool evaluate( const EvaluationContext& ) const = 0;

	/** 
	 * used for optimization: returns a list of key-value pairs which represents attributes
	 * that must be equal for the predicate to match.
	 */
	virtual AttribList getConjunctiveEqualities() const
	{ return AttribList(); }

};


/** Predicate implementation: Negation of a predicate */
class PredicateNot
	: public Predicate
{
public:
	/** Constructs the predicate from a pointer to a child expression */
	PredicateNot( const boost::shared_ptr< Predicate > pChild );

	/** Evaluates the predicate on a context */
	bool evaluate( const EvaluationContext& ) const;

protected:
	const boost::shared_ptr< Predicate > m_pChild;
};


/** Predicate implementation: Conjugation of two predicates */
class PredicateAnd
	: public Predicate
{
public:
	/** Constructs the predicate from pointers to two child expressions */
	PredicateAnd( const boost::shared_ptr< Predicate > pChild1, const boost::shared_ptr< Predicate > pChild2 );

	/** Evaluates the predicate on a context */
	bool evaluate( const EvaluationContext& ) const;

	/** for optimization */
	AttribList getConjunctiveEqualities() const;

protected:
	const boost::shared_ptr< Predicate > m_pChild1;
	const boost::shared_ptr< Predicate > m_pChild2;
};


/** Predicate implementation: Disjuction of two predicates */
class PredicateOr
	: public Predicate
{
public:
	/** Constructs the predicate from pointers to two child expressions */
	PredicateOr( const boost::shared_ptr< Predicate > pChild1, const boost::shared_ptr< Predicate > pChild2 );

	/** Evaluates the predicate on a context */
	bool evaluate( const EvaluationContext& ) const;

protected:
	const boost::shared_ptr< Predicate > m_pChild1;
	const boost::shared_ptr< Predicate > m_pChild2;
};


/** Predicate implementation: Comparison of two values */
class PredicateCompare
	: public Predicate
{
public:
	/** possible types of comparisons */
	enum ComparisonType	{ equals, notEquals, greater, less, greaterEquals, lessEquals };

	/** Constructs the predicate from pointers to two child expressions */
	PredicateCompare( const std::string& op, const boost::shared_ptr< AttributeExpression > pChild1,
		const boost::shared_ptr< AttributeExpression > pChild2 );

	/** Evaluates the predicate on a context */
	bool evaluate( const EvaluationContext& ) const;

	/** for optimization */
	AttribList getConjunctiveEqualities() const;
	
protected:
	ComparisonType m_type;
	const boost::shared_ptr< AttributeExpression > m_pChild1;
	const boost::shared_ptr< AttributeExpression > m_pChild2;
	
	bool doCompare( const AttributeValue& a, const AttributeValue& b ) const;
};


/** Predicate implementation: more complicated predicates expressed as functions p(...) */
class PredicateFunction
	: public Predicate
{
public:
	/** possible types of functions */
	enum FunctionType { inSourceSet };

	/** Constructs the predicate from list of child expressions */
	PredicateFunction( const std::string& func, const std::vector< boost::shared_ptr< AttributeExpression > >& children );

	/** Evaluates the predicate on a context */
	bool evaluate( const EvaluationContext& ) const;

protected:
	FunctionType m_type;
	boost::shared_ptr< AttributeExpression > m_pChild1;
};

} } // namespace Ubitrack::Graph

#endif

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
 * UTQL attribute expressions header file
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Graph_AttributeExpression_H_INCLUDED__
#define __Ubitrack_Graph_AttributeExpression_H_INCLUDED__

#include <vector>
#include "Predicate.h"
#include "AttributeValue.h"
#include <utUtil/Exception.h>

namespace Ubitrack { namespace Graph {

/** virtual base class of all attribute expressions */
class UTDATAFLOW_EXPORT AttributeExpression
{
public:
	/** always have a virtual destructor */
	virtual ~AttributeExpression()
	{}

	/** 
	 * Evaluate this attribute expression on the supplied context. 
	 */
	virtual AttributeValue evaluate( const EvaluationContext& ) const = 0;
};


/** constant expression */
class AttributeExpressionConstant
	: public AttributeExpression
{
public:
	/** constructs the expression from the supplied value */
	AttributeExpressionConstant( const std::string& value );

	/** Evaluates the expression on a context */
	AttributeValue evaluate( const EvaluationContext& ) const;

	/** returns the value of the constant expression */
	const std::string& getValue() const
	{ return m_value.getText(); }
	
protected:
	AttributeValue m_value;
};


/** expression that reads an attribute */
class AttributeExpressionAttribute
	: public AttributeExpression
{
public:
	/** constructs the expression from the attribute name */
	AttributeExpressionAttribute( const std::string& name );

	/** Evaluates the expression on a context */
	AttributeValue evaluate( const EvaluationContext& ) const;

	/** gets the name of the attribute */
	const std::string& getName() const
	{ return m_name; }
	
protected:
	std::string m_name;
	std::string m_nodeEdge;
};


/** expression that performs a unary mathematical operation */
template< class Functor >
class AttributeExpressionUnary
	: public AttributeExpression
{
public:
	/** constructs the expression */
	AttributeExpressionUnary( boost::shared_ptr< AttributeExpression > pChild, const Functor& f = Functor() )
		: m_f( f )
		, m_pChild( pChild )
	{}

	/** Evaluates the expression on a context */
	AttributeValue evaluate( const EvaluationContext& c ) const
	{ return AttributeValue( m_f( m_pChild->evaluate( c ).getNumber() ) ); }

protected:
	Functor m_f;
	boost::shared_ptr< AttributeExpression > m_pChild;
};


/** expression that performs a binary mathematical operation */
template< class Functor >
class AttributeExpressionBinary
	: public AttributeExpression
{
public:
	/** constructs the expression */
	AttributeExpressionBinary( boost::shared_ptr< AttributeExpression > pChild1, boost::shared_ptr< AttributeExpression > pChild2,
		const Functor& f = Functor() )
		: m_f( f )
		, m_pChild1( pChild1 )
		, m_pChild2( pChild2 )
	{}

	/** Evaluates the expression on a context */
	AttributeValue evaluate( const EvaluationContext& c ) const
	{ return AttributeValue( m_f( 
		m_pChild1->evaluate( c ).getNumber(), 
		m_pChild2->evaluate( c ).getNumber() ) ); }

protected:
	Functor m_f;
	boost::shared_ptr< AttributeExpression > m_pChild1;
	boost::shared_ptr< AttributeExpression > m_pChild2;
};


/** evaluates arbitrary functions */
class AttributeExpressionFunction
	: public AttributeExpression
{
public:
	/** constructs the expression */
	AttributeExpressionFunction( const std::string& sF, std::vector< boost::shared_ptr< AttributeExpression > >& children );

	/** Evaluates the expression on a context */
	AttributeValue evaluate( const EvaluationContext& ) const;

protected:
	enum FunctionType { FunctionSyncError, FunctionSteadyState, FunctionSourceCount };
	FunctionType m_functionType;
	std::vector< boost::shared_ptr< AttributeExpression > > m_children;
};

} } // namespace Ubitrack::Graph

#endif

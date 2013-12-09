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
 * UTQL attribute expressions  implementation
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include <assert.h>
#include "../utDataflow.h"
#include "AttributeExpression.h"
#include "KeyValueAttributes.h"
#include "UTQLSubgraph.h"
#include "EvaluationContext.h"

#ifdef HAVE_LAPACK
#include <utMath/Matrix.h>
#include <complex>
#include <boost/numeric/bindings/blas/blas3.hpp>
#include <boost/numeric/bindings/lapack/gesv.hpp>
#include <boost/numeric/bindings/lapack/geev.hpp>
namespace ublas = boost::numeric::ublas;
namespace lapack = boost::numeric::bindings::lapack;
namespace blas = boost::numeric::bindings::blas;
#endif

namespace Ubitrack { namespace Graph {

AttributeExpressionConstant::AttributeExpressionConstant( const std::string& value )
	: m_value( value )
{
	// try conversion to number now so we don't have to do it at runtime
	m_value.isNumber();
}


AttributeValue AttributeExpressionConstant::evaluate( const EvaluationContext& ) const
{
	return m_value;
}


AttributeExpressionAttribute::AttributeExpressionAttribute( const std::string& name )
{
	// split name into node/edge and attribute name
	std::string::size_type iSplit = name.find( '.', 0 );
	if ( iSplit != std::string::npos )
	{
		m_nodeEdge = name.substr( 0, iSplit );
		m_name = name.substr( iSplit + 1, std::string::npos );
	}
	else
		m_name = name;
}


AttributeValue AttributeExpressionAttribute::evaluate( const EvaluationContext& c ) const
{
	const KeyValueAttributes* pAttr;
	if ( c.isGlobal() )
	{
		pAttr = c.getNodeEdgeAttributes( m_nodeEdge );
		if ( !pAttr )
			return AttributeValue();
	}
	else 
		pAttr = c.getNodeEdgeAttributes();

	assert( pAttr != 0 );
		
	if ( pAttr->hasAttribute( m_name ) )
		return pAttr->getAttribute( m_name );
	else
		return AttributeValue();
}


AttributeExpressionFunction::AttributeExpressionFunction( const std::string& sF,
	std::vector< boost::shared_ptr< AttributeExpression > >& children )
	: m_children( children )
{
	if ( sF == "syncError" )
	{
		if ( children.size() != 3 )
			UBITRACK_THROW( "Illegal number of arguments for function " + sF );

		m_functionType = FunctionSyncError;
	}
#ifdef HAVE_LAPACK
	else if ( sF == "steadyState" )
	{
		if ( children.size() < 4 || ( children.size() - 1 ) % 3 != 0 )
			UBITRACK_THROW( "Illegal number of arguments for function " + sF );

		m_functionType = FunctionSteadyState;
	}
#endif
	else if ( sF == "sourceCount" )
	{
		if ( children.size() > 1 )
			UBITRACK_THROW( "sourceCount can have at most one argument" );

		m_functionType = FunctionSourceCount;
	}
	else
		UBITRACK_THROW( "Unknown function: " + sF );
}


AttributeValue AttributeExpressionFunction::evaluate( const EvaluationContext& c ) const
{
	if ( !c.isGlobal() )
		UBITRACK_THROW( "Function can only be used within AttributeExpressions" );
	
	if ( m_functionType == FunctionSyncError )
	{
		// compute error of synchronizing a pull input (arg 2) with a reference input (arg 3)
		// arg 1 is the motion model q
		double q = m_children[ 0 ]->evaluate( c ).getNumber();
		std::string syncEdge = m_children[ 1 ]->evaluate( c ).getText();
		std::string refEdge = m_children[ 2 ]->evaluate( c ).getText();

		const KeyValueAttributes* pSync = c.getNodeEdgeAttributes( syncEdge );
		const KeyValueAttributes* pRef = c.getNodeEdgeAttributes( refEdge );
		if ( !pSync || !pRef )
			UBITRACK_THROW( "edge not found" );
		double syncUpdateTime = pSync->getAttribute( "updateTime" ).getNumber();
		double syncLatency = pSync->getAttribute( "latency" ).getNumber();
		double refLatency = pRef->getAttribute( "latency" ).getNumber();
		double result;

		// avoid division by zero. 
		// todo: compute real limit
		if ( syncUpdateTime < 1e-10 )
			result = 0.0;
		else
		{
			double t1 = std::max( 0.0, syncLatency - refLatency + syncUpdateTime );
			double t2 = std::max( 0.0, syncLatency - refLatency );
			result = q / ( 12.0 * syncUpdateTime ) * ( t1*t1*t1*t1 - t2*t2*t2*t2 );
		}

		return AttributeValue( result );
	}

#ifdef HAVE_LAPACK
	else if ( m_functionType == FunctionSteadyState )
	{
		// Compute steady state solution of a simple two-state kalman filter.
		// State update: S_n+1 = S_n * [ 1, dt; 0, 1 ]
		//
		// For an explanation, see: D. Allen and  G. Welch, "A General Method for Comparing the Expected Performance
		// of Tracking and Motion Capture Systems", VRST 2005
		//
		// The function has 1 + 3 * n arguments:
		// arg 1: q that defines the motion model: Q = q * [ 1/3*dt^3, 1/2*dt^2; 1/2*dt^2, dt ]
		// arg 3*i+2: "A" for absolute (H = [ 1, 0 ]), "R" for relative (H = [ 0, 1 ])measurement
		// arg 3*i+3: dt, the time between measurements
		// arg 3*i+4: r, the measurement variance

		Math::Matrix< double, 4, 4 > psiSum( Math::Matrix< double, 4, 4 >::zeros( ) );
		double q = m_children[ 0 ]->evaluate( c ).getNumber();

		for ( unsigned startArg = 1; startArg < m_children.size(); startArg += 3 )
		{
			Math::Matrix< double, 4, 4 > psi;
			std::string sType = m_children[ startArg ]->evaluate( c ).getText();
			double dt = m_children[ startArg + 1 ]->evaluate( c ).getNumber();
			double r = m_children[ startArg + 2 ]->evaluate( c ).getNumber();

			if ( sType == "A" )
			{
				// measurement of absolute value
				psi( 0, 0 ) = 1.0-1/r*(q*dt*dt*dt)/6.0;
				psi( 0, 1 ) = dt;      
				psi( 0, 2 ) = -(q*dt*dt*dt)/6.0;      
				psi( 0, 3 ) = (q*dt*dt)/2.0;
				psi( 1, 0 ) = -1/r*(q*dt*dt)/2.0;      
				psi( 1, 1 ) = 1.0;      
				psi( 1, 2 ) = -(q*dt*dt)/2.0;
				psi( 1, 3 ) = q*dt;
				psi( 2, 0 ) = 1/r;
				psi( 2, 1 ) = 0.0;
				psi( 2, 2 ) = 1.0;
				psi( 2, 3 ) = 0.0;
				psi( 3, 0 ) = -1/r*dt;
				psi( 3, 1 ) = 0.0;
				psi( 3, 2 ) = -dt;
				psi( 3, 3 ) = 1.0;
			}
			else if ( sType == "R" )
			{
				// measurement of velocity
				psi( 0, 0 ) = 1.0;
				psi( 0, 1 ) = dt+1/r*(q*dt*dt)/2.0;
				psi( 0, 2 ) = -(q*dt*dt*dt)/6.0;      
				psi( 0, 3 ) = (q*dt*dt)/2.0;
				psi( 1, 0 ) = 0.0;
				psi( 1, 1 ) = 1.0+1/r*(q*dt);
				psi( 1, 2 ) = -(q*dt*dt)/2.0;
				psi( 1, 3 ) = q*dt;
				psi( 2, 0 ) = 0.0;    
				psi( 2, 1 ) = 0.0;
				psi( 2, 2 ) = 1.0;
				psi( 2, 3 ) = 0.0;
				psi( 3, 0 ) = 0.0;
				psi( 3, 1 ) = 1/r;
				psi( 3, 2 ) = -dt;
				psi( 3, 3 ) = 1.0;
			}
			else
				UBITRACK_THROW( "steadyState: unknown measurement type " + sType );

			psiSum += psi;
		}

		// compute complex eigenvalues of psiSum;
		typedef	Math::Matrix< std::complex< double > > ComplexMat;
		Math::Vector< std::complex< double > > eigenvalues( 4 );
		ComplexMat eigenvectors( 4, 4 );
		if ( lapack::geev( psiSum, eigenvalues, (ComplexMat*)0, &eigenvectors, lapack::optimal_workspace() ) != 0 )
			UBITRACK_THROW( "singular matrix" );

		// get B and C submatrics
		ublas::matrix_range< ComplexMat > B( eigenvectors, ublas::range( 0, 2 ), ublas::range( 0, 2 ) );
		ublas::matrix_range< ComplexMat > C( eigenvectors, ublas::range( 2, 4 ), ublas::range( 0, 2 ) );

		// invert C
		Math::Vector< int > ipiv( 2 );
		if ( lapack::getrf( C, ipiv ) != 0 )
			UBITRACK_THROW( "singular matrix" );
		lapack::getri( C, ipiv );

		// compute result
		std::complex< double > result = ublas::prod( B, C )( 0, 0 );

		return AttributeValue( sqrt( result.imag() * result.imag() + result.real() * result.real() ) );
	}
#endif
	else if ( m_functionType == FunctionSourceCount )
	{
		// counts number of information sources whose pattern IDs start with a certain prefix
		const std::set< std::string >& sources( 
			c.isGlobal() ? 
				c.getMatching()->m_informationSources : 
				c.getNodeEdgePatternAttributes()->m_InformationSources );
				
		if ( m_children.empty() )
			return AttributeValue( sources.size() );
		else
		{
			unsigned nSources = 0;
			std::string query = m_children[ 0 ]->evaluate( c ).getText();
			
			std::set< std::string >::const_iterator it = sources.lower_bound( query );
			while ( it != sources.end() && it->size() >= query.size() && !it->compare( 0, query.size(), query ) )
				it++, nSources++;
				
			return AttributeValue( nSources );
		}
	}

	return AttributeValue(); 
}


} } // namespace Ubitrack::Graph

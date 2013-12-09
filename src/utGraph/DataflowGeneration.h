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
 * Encapsulates the whole pattern matching in a single function
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Graph_DataflowGeneration_H_INCLUDED__
#define __Ubitrack_Graph_DataflowGeneration_H_INCLUDED__

#include <boost/shared_ptr.hpp>
#include <utDataflow.h>
#include "UTQLDocument.h"

namespace Ubitrack { namespace Graph {

/**
 * Performs the pattern matching on a single set of patterns and generates a data flow description.
 * @param patterns input patterns
 * @return UTQLDocument containing the generated dataflow
 */
UTDATAFLOW_EXPORT boost::shared_ptr< UTQLDocument > generateDataflow( UTQLDocument& patterns ); 


/**
 * Performs the pattern matching on a single set of patterns and generates a data flow description (string version).
 * @param patterns input patterns
 * @return UTQLDocument containing the generated dataflow
 */
UTDATAFLOW_EXPORT std::string generateDataflow( const std::string& query );


/**
 * Converts a UTQL response to a graphviz dataflow representation (DOT format).
 */
UTDATAFLOW_EXPORT std::string dataflowToGraphviz( const std::string& dataflow );
 
} } // namespace Ubitrack::Graph

#endif

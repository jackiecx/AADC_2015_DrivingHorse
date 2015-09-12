/**
 *
 * Template Filter
 *
 * @file
 * Copyright &copy; Audi Electronics Venture GmbH. All rights reserved.
 *
 * @author               $Author: voigtlpi $
 * @date                 $Date: 2009-07-16 15:36:37 +0200 (Do, 16 Jul 2009) $
 * @version              $Revision: 10093 $
 *
 * @remarks
 *
 */
 
/**
 * \page page_demo_template Template Filter
 *
 * Implements a filter which processes incoming mediasamples from an input pin and tranmits new samples on an
 * output pin. This filter is intended to be used as a template for new filters.
 * 
 * \par Location
 * \code
 *    ./src/examples/src/templates/template_filter
 * \endcode
 *
 * \par Build Environment
 * To see how to set up the build environment have a look at this page @ref page_cmake_overview
 *
 * \par This example shows:
 * \li how to implement a common adtf filter for processing data
 * \li how to receive data from a pin
 * \li how to read and write data to and from samples
 * \li how to send data over a pin
 *
 * \par Call Sequence
 * The source code of the ADTF SDK base classes is included in the installation. You can use it to gain
 * a better understanding of the call sequences when using base classes of ADTF SDK by debugging .\n
 * 
 * \par The data definitions for the Template Filter
 * \include templates/template_filter/include/template_data.h
 *
 * \par The Header for the Template Filter
 * \include templates/template_filter/src/template.h
 *
 * \par The Implementation for the Template Filter
 * \include templates/template_filter/src/template.cpp
 *
 */ 
 
/*------------------------------------------------------------------------------
 *                             (c) 2015 by Ivan Peon
 *                             All rights reserved
 *------------------------------------------------------------------------------
 *   Module   : CppU.h
 *   Date     : 14.10.2015
 *------------------------------------------------------------------------------
 *   Module description:
 */
/** @file
 *  ...
 */
/*------------------------------------------------------------------------------
 */
#ifndef CPPU_H_
#define CPPU_H_

#ifdef _WIN32
# define CPPU_RESPATH "res\\"
#else
# define CPPU_RESPATH "res/"
#endif

// STL macros
//#define STL_VEC_SZ(V) (V._M_impl._M_finish - V._M_impl._M_start)
//#define STL_MAP_SZ(M) (M._M_t._M_impl._M_node_count)
//
//#define TestMAP sLoggers._M_t._M_impl._M_node_count


#endif /* CPPU_H_ */

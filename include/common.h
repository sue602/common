/*
 * common.h
 *
 *  Created on: Aug 18, 2016
 *      Author: ltzd
 */

#ifndef INCLUDE_COMMON_H_
#define INCLUDE_COMMON_H_

//init function
void common_init();

//uninit function
void common_fini();
//
void * easy_malloc(unsigned int sz);

//
void easy_free(void * p);

#endif /* INCLUDE_COMMON_H_ */

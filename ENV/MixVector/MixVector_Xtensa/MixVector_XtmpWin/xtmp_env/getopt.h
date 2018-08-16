/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
* and may contain trade secrets and/or other confidential information of VIA   *
* Technologies,Inc.                                                            *
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
*******************************************************************************/

#ifndef MY_GETOPT_H_INCLUDED
#define MY_GETOPT_H_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

/* UNIX-style short-argument parser */
extern int getopt(int argc, char* const* argv, const char *opts);

extern int optind, opterr, optopt;
extern char *optarg;

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

/* human-readable values for has_arg */
#undef no_argument
#define no_argument 0
#undef required_argument
#define required_argument 1
#undef optional_argument
#define optional_argument 2

/* GNU-style long-argument parsers */
extern int getopt_long(int argc, char * argv[], const char *shortopts,
                       const struct option *longopts, int *longind);

extern int getopt_long_only(int argc, char * argv[], const char *shortopts,
                            const struct option *longopts, int *longind);

extern int _getopt_internal(int argc, char * argv[], const char *shortopts,
                            const struct option *longopts, int *longind,
                            int long_only);

#ifdef	__cplusplus
}
#endif

#endif /* MY_GETOPT_H_INCLUDED */

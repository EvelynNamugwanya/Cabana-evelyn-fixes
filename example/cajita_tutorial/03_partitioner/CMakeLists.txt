############################################################################
# Copyright (c) 2018-2022 by the Cabana authors                            #
# All rights reserved.                                                     #
#                                                                          #
# This file is part of the Cabana library. Cabana is distributed under a   #
# BSD 3-clause license. For the licensing terms see the LICENSE file in    #
# the top-level directory.                                                 #
#                                                                          #
# SPDX-License-Identifier: BSD-3-Clause                                    #
############################################################################

add_executable(Partitioner partitioner_example.cpp)
target_link_libraries(Partitioner Cajita)
add_test(NAME Cajita_tutorial_03 COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} $<TARGET_FILE:Partitioner> ${MPIEXEC_POSTFLAGS})
set_tests_properties(Cajita_tutorial_03 PROPERTIES PROCESSORS ${MPIEXEC_MAX_NUMPROCS})

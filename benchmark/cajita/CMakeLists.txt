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

if(NOT Kokkos_ENABLE_SYCL) #FIXME_SYCL
  add_executable(SparseMapPerformance Cajita_SparseMapPerformance.cpp)
  target_link_libraries(SparseMapPerformance Cajita)

  add_executable(SparsePartitionerPerformance Cajita_SparsePartitionerPerformance.cpp)
  target_link_libraries(SparsePartitionerPerformance Cajita)
endif()

add_executable(HaloPerformance Cajita_HaloPerformance.cpp)
target_link_libraries(HaloPerformance Cajita)

add_executable(InterpolationPerformance Cajita_InterpolationPerformance.cpp)
target_link_libraries(InterpolationPerformance Cajita)

if(Cabana_ENABLE_HEFFTE)
  add_executable(FastFourierTransformPerformance Cajita_FastFourierTransformPerformance.cpp)
  target_link_libraries(FastFourierTransformPerformance Cajita)
endif()

if(Cabana_ENABLE_TESTING)
  if(NOT Kokkos_ENABLE_SYCL) #FIXME_SYCL
    add_test(NAME Cajita_SparseMapPerformance COMMAND ${NONMPI_PRECOMMAND} $<TARGET_FILE:SparseMapPerformance> sparsemap_output.txt)
    add_test(NAME Cajita_SparsePartitionerPerformance COMMAND ${NONMPI_PRECOMMAND} $<TARGET_FILE:SparsePartitionerPerformance> sparsepartitioner_output.txt)
  endif()

  add_test(NAME Cajita_HaloPerformance COMMAND ${NONMPI_PRECOMMAND} $<TARGET_FILE:HaloPerformance> halo_output.txt)

  add_test(NAME Cajita_InterpolationPerformance COMMAND ${NONMPI_PRECOMMAND} $<TARGET_FILE:InterpolationPerformance> interpolation_output.txt)

  if (Cabana_ENABLE_HEFFTE)
    add_test(NAME Cajita_FastFourierTransformPerformance COMMAND ${NONMPI_PRECOMMAND} $<TARGET_FILE:FastFourierTransformPerformance> fastfouriertransform_output.txt)
  endif()
endif()

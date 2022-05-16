/****************************************************************************
 * Copyright (c) 2018-2022 by the Cabana authors                            *
 * All rights reserved.                                                     *
 *                                                                          *
 * This file is part of the Cabana library. Cabana is distributed under a   *
 * BSD 3-clause license. For the licensing terms see the LICENSE file in    *
 * the top-level directory.                                                 *
 *                                                                          *
 * SPDX-License-Identifier: BSD-3-Clause                                    *
 ****************************************************************************/

/****************************************************************************
 * Copyright (c) 2022 by the Picasso authors                                *
 * All rights reserved.                                                     *
 *                                                                          *
 * This file is part of the Picasso library. Picasso is distributed under a *
 * BSD 3-clause license. For the licensing terms see the LICENSE file in    *
 * the top-level directory.                                                 *
 *                                                                          *
 * SPDX-License-Identifier: BSD-3-Clause                                    *
 ****************************************************************************/

/*!
  \file Cabana_HDF5ParticleOutput.hpp
  \brief Write particle output using the HDF5 (XDMF) format.
*/

#ifndef CABANA_HDF5PARTICLEOUTPUT_HPP
#define CABANA_HDF5PARTICLEOUTPUT_HPP

#include <Kokkos_Core.hpp>

#include <hdf5.h>
#include <mpi.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace Cabana
{
namespace Experimental
{
namespace HDF5ParticleOutput
{

namespace Impl
{
// XDMF file creation routines
//! \cond Impl
inline void writeXdmfHeader( const char* xml_file_name, hsize_t dims0,
                             hsize_t dims1, const char* dtype, uint precision,
                             const char* h5_file_name )
{
    std::ofstream xdmf_file( xml_file_name, std::ios::trunc );
    xdmf_file << "<?xml version=\"1.0\" ?>\n";
    xdmf_file << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n";
    xdmf_file << "<Xdmf Version=\"2.0\">\n";
    xdmf_file << "  <Domain>\n";
    xdmf_file << "    <Grid Name=\"points\" GridType=\"Uniform\">\n";
    xdmf_file << "      <Topology TopologyType=\"Polyvertex\"";
    xdmf_file << " Dimensions=\"" << dims0 << "\"";
    xdmf_file << " NodesPerElement=\"1\"> </Topology>\n";
    xdmf_file << "      <Geometry Type=\"XYZ\">\n";
    xdmf_file << "         <DataItem Dimensions=\"" << dims0 << " " << dims1;
    xdmf_file << "\" NumberType=\"" << dtype;
    xdmf_file << "\" Precision=\"" << precision;
    xdmf_file << "\" Format=\"HDF\"> " << h5_file_name << ":/coord_xyz";
    xdmf_file << " </DataItem>\n";
    xdmf_file << "      </Geometry>\n";
    xdmf_file.close();
}

inline void writeXdmfAttribute( const char* xml_file_name,
                                const char* field_name, hsize_t dims0,
                                hsize_t dims1, hsize_t dims2, const char* dtype,
                                uint precision, const char* h5_file_name,
                                const char* dataitem )
{
    std::string AttributeType = "\"Scalar\"";
    if ( dims2 != 0 )
        AttributeType = "\"Tensor\"";
    else if ( dims1 != 0 )
        AttributeType = "\"Vector\"";

    std::ofstream xdmf_file( xml_file_name, std::ios::app );
    xdmf_file << "      <Attribute AttributeType =" << AttributeType
              << " Center=\"Node\"";
    xdmf_file << " Name=\"" << field_name << "\">\n";
    xdmf_file << "        <DataItem ItemType=\"Uniform\" Dimensions=\""
              << dims0;
    if ( dims1 != 0 )
        xdmf_file << " " << dims1;
    if ( dims2 != 0 )
        xdmf_file << " " << dims2;
    xdmf_file << "\" DataType=\"" << dtype << "\" Precision=\"" << precision
              << "\"";
    xdmf_file << " Format=\"HDF\"> " << h5_file_name << ":/" << dataitem;
    xdmf_file << " </DataItem>\n";
    xdmf_file << "      </Attribute>\n";
    xdmf_file.close();
}

inline void writeXdmfFooter( const char* xml_file_name )
{
    std::ofstream xdmf_file( xml_file_name, std::ios::app );
    xdmf_file << "    </Grid>\n";
    xdmf_file << "  </Domain>\n</Xdmf>\n";
    xdmf_file.close();
}
//! \endcond
} // namespace Impl

/*!
  \brief HDF5 tuning settings.

  Various property list setting to tune HDF5 for a given system. For an
  in-depth description of these settings, see the HDF5 reference manual at
  https://docs.hdfgroup.org/hdf5/develop

  File access property list alignment settings result in any file
  object &ge; threshold bytes aligned on an address which is a multiple of
  alignment.
*/
struct HDF5Config
{
    //! I/O transfer mode to collective or independent (default)
    bool collective = false;

    //! Set alignment on or off
    bool align = false;
    //! Threshold for aligning file objects
    unsigned long threshold = 0;
    //! Alignment value
    unsigned long alignment = 16777216;

    //! Sets metadata I/O mode operations to collective or independent (default)
    bool meta_collective = true;

    //! Cause all metadata for an object to be evicted from the cache
    bool evict_on_close = false;
};

//---------------------------------------------------------------------------//
// HDF5 (XDMF) Particle Field Output.
//---------------------------------------------------------------------------//
//! \cond Impl
// Format traits for both HDF5 and XDMF.
template <typename T>
struct HDF5Traits;

template <>
struct HDF5Traits<int>
{
    static hid_t type( std::string* dtype, uint* precision )
    {
        *dtype = "Int";
        *precision = sizeof( int );
        return H5T_NATIVE_INT;
    }
};

template <>
struct HDF5Traits<unsigned int>
{
    static hid_t type( std::string* dtype, uint* precision )
    {
        *dtype = "UInt";
        *precision = sizeof( unsigned int );
        return H5T_NATIVE_UINT;
    }
};

template <>
struct HDF5Traits<long>
{
    static hid_t type( std::string* dtype, uint* precision )
    {
        *dtype = "Int";
        *precision = sizeof( long );
        return H5T_NATIVE_LONG;
    }
};

template <>
struct HDF5Traits<unsigned long>
{
    static hid_t type( std::string* dtype, uint* precision )
    {
        *dtype = "UInt";
        *precision = sizeof( unsigned long );
        return H5T_NATIVE_ULONG;
    }
};

template <>
struct HDF5Traits<float>
{
    static hid_t type( std::string* dtype, uint* precision )
    {
        *dtype = "Float";
        *precision = sizeof( float );
        return H5T_NATIVE_FLOAT;
    }
};

template <>
struct HDF5Traits<double>
{
    static hid_t type( std::string* dtype, uint* precision )
    {
        *dtype = "Float";
        *precision = sizeof( double );
        return H5T_NATIVE_DOUBLE;
    }
};

namespace Impl
{
//---------------------------------------------------------------------------//
// Rank-0 field
template <class SliceType>
void writeFields(
    HDF5Config h5_config, hid_t file_id, std::size_t n_local,
    std::size_t n_global, hsize_t n_offset, int comm_rank,
    const char* filename_hdf5, const char* filename_xdmf,
    const SliceType& slice,
    typename std::enable_if<
        2 == SliceType::kokkos_view::traits::dimension::rank, int*>::type = 0 )
{
    hid_t plist_id;
    hid_t dset_id;
    hid_t filespace_id;
    hid_t memspace_id;

    // HDF5 hyperslab parameters
    hsize_t offset[1];
    hsize_t dimsf[1];
    hsize_t count[1];

    offset[0] = n_offset;
    count[0] = n_local;
    dimsf[0] = n_global;

    // Reorder in a contiguous blocked format.
    Kokkos::View<typename SliceType::value_type*,
                 typename SliceType::device_type>
        view( Kokkos::ViewAllocateWithoutInitializing( "field" ),
              slice.size() );
    Kokkos::parallel_for(
        "Cabana::HDF5ParticleOutput::writeFieldRank0",
        Kokkos::RangePolicy<typename SliceType::execution_space>(
            0, slice.size() ),
        KOKKOS_LAMBDA( const int i ) { view( i ) = slice( i ); } );

    // Mirror the field to the host.
    auto host_view =
        Kokkos::create_mirror_view_and_copy( Kokkos::HostSpace(), view );

    std::string dtype;
    uint precision = 0;
    hid_t type_id =
        HDF5Traits<typename SliceType::value_type>::type( &dtype, &precision );

    filespace_id = H5Screate_simple( 1, dimsf, NULL );
    dset_id = H5Dcreate( file_id, slice.label().c_str(), type_id, filespace_id,
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );

    H5Sselect_hyperslab( filespace_id, H5S_SELECT_SET, offset, NULL, count,
                         NULL );

    memspace_id = H5Screate_simple( 1, count, NULL );

    plist_id = H5Pcreate( H5P_DATASET_XFER );
    // Default IO in HDF5 is independent
    if ( h5_config.collective )
        H5Pset_dxpl_mpio( plist_id, H5FD_MPIO_COLLECTIVE );

    H5Dwrite( dset_id, type_id, memspace_id, filespace_id, plist_id,
              host_view.data() );

    H5Pclose( plist_id );
    H5Sclose( memspace_id );
    H5Dclose( dset_id );
    H5Sclose( filespace_id );

    if ( 0 == comm_rank )
    {
        hsize_t zero = 0;
        Impl::writeXdmfAttribute(
            filename_xdmf, slice.label().c_str(), dimsf[0], zero, zero,
            dtype.c_str(), precision, filename_hdf5, slice.label().c_str() );
    }
}

// Rank-1 field
template <class SliceType>
void writeFields(
    HDF5Config h5_config, hid_t file_id, std::size_t n_local,
    std::size_t n_global, hsize_t n_offset, int comm_rank,
    const char* filename_hdf5, const char* filename_xdmf,
    const SliceType& slice,
    typename std::enable_if<
        3 == SliceType::kokkos_view::traits::dimension::rank, int*>::type = 0 )
{
    hid_t plist_id;
    hid_t dset_id;
    hid_t filespace_id;
    hid_t memspace_id;

    // Reorder in a contiguous blocked format.
    Kokkos::View<typename SliceType::value_type**, Kokkos::LayoutRight,
                 typename SliceType::device_type>
        view( Kokkos::ViewAllocateWithoutInitializing( "field" ), slice.size(),
              slice.extent( 2 ) );
    Kokkos::parallel_for(
        "Cabana::HDF5ParticleOutput::writeFieldRank1",
        Kokkos::RangePolicy<typename SliceType::execution_space>(
            0, slice.size() ),
        KOKKOS_LAMBDA( const int i ) {
            for ( std::size_t d0 = 0; d0 < slice.extent( 2 ); ++d0 )
                view( i, d0 ) = slice( i, d0 );
        } );

    // Mirror the field to the host.
    auto host_view =
        Kokkos::create_mirror_view_and_copy( Kokkos::HostSpace(), view );

    hsize_t offset[2];
    hsize_t dimsf[2];
    hsize_t dimsm[2];
    hsize_t count[2];

    dimsf[0] = n_global;
    dimsf[1] = host_view.extent( 1 );
    dimsm[0] = n_local;
    dimsm[1] = host_view.extent( 1 );

    offset[0] = n_offset;
    offset[1] = 0;

    count[0] = dimsm[0];
    count[1] = dimsm[1];

    std::string dtype;
    uint precision;
    hid_t type_id =
        HDF5Traits<typename SliceType::value_type>::type( &dtype, &precision );

    filespace_id = H5Screate_simple( 2, dimsf, NULL );
    dset_id = H5Dcreate( file_id, slice.label().c_str(), type_id, filespace_id,
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );

    H5Sselect_hyperslab( filespace_id, H5S_SELECT_SET, offset, NULL, count,
                         NULL );

    memspace_id = H5Screate_simple( 2, dimsm, NULL );
    plist_id = H5Pcreate( H5P_DATASET_XFER );
    // Default IO in HDF5 is independent
    if ( h5_config.collective )
        H5Pset_dxpl_mpio( plist_id, H5FD_MPIO_COLLECTIVE );

    H5Dwrite( dset_id, type_id, memspace_id, filespace_id, plist_id,
              host_view.data() );

    H5Pclose( plist_id );
    H5Sclose( memspace_id );
    H5Dclose( dset_id );
    H5Sclose( filespace_id );

    if ( 0 == comm_rank )
    {
        hsize_t zero = 0;
        Impl::writeXdmfAttribute(
            filename_xdmf, slice.label().c_str(), dimsf[0], dimsf[1], zero,
            dtype.c_str(), precision, filename_hdf5, slice.label().c_str() );
    }
}

// Rank-2 field
template <class SliceType>
void writeFields(
    HDF5Config h5_config, hid_t file_id, std::size_t n_local,
    std::size_t n_global, hsize_t n_offset, int comm_rank,
    const char* filename_hdf5, const char* filename_xdmf,
    const SliceType& slice,
    typename std::enable_if<
        4 == SliceType::kokkos_view::traits::dimension::rank, int*>::type = 0 )
{
    hid_t plist_id;
    hid_t dset_id;
    hid_t filespace_id;
    hid_t memspace_id;

    // Reorder in a contiguous blocked format.
    Kokkos::View<typename SliceType::value_type***, Kokkos::LayoutRight,
                 typename SliceType::device_type>
        view( Kokkos::ViewAllocateWithoutInitializing( "field" ), slice.size(),
              slice.extent( 2 ), slice.extent( 3 ) );
    Kokkos::parallel_for(
        "Cabana::HDF5ParticleOutput::writeFieldRank2",
        Kokkos::RangePolicy<typename SliceType::execution_space>(
            0, slice.size() ),
        KOKKOS_LAMBDA( const int i ) {
            for ( std::size_t d0 = 0; d0 < slice.extent( 2 ); ++d0 )
                for ( std::size_t d1 = 0; d1 < slice.extent( 3 ); ++d1 )
                    view( i, d0, d1 ) = slice( i, d0, d1 );
        } );

    // Mirror the field to the host.
    auto host_view =
        Kokkos::create_mirror_view_and_copy( Kokkos::HostSpace(), view );

    hsize_t offset[3];
    hsize_t dimsf[3];
    hsize_t dimsm[3];
    hsize_t count[3];

    dimsf[0] = n_global;
    dimsf[1] = host_view.extent( 1 );
    dimsf[2] = host_view.extent( 2 );
    dimsm[0] = n_local;
    dimsm[1] = host_view.extent( 1 );
    dimsm[2] = host_view.extent( 2 );

    offset[0] = n_offset;
    offset[1] = 0;
    offset[2] = 0;

    count[0] = dimsm[0];
    count[1] = dimsm[1];
    count[2] = dimsm[2];

    std::string dtype;
    uint precision;
    hid_t type_id =
        HDF5Traits<typename SliceType::value_type>::type( &dtype, &precision );

    filespace_id = H5Screate_simple( 3, dimsf, NULL );
    dset_id = H5Dcreate( file_id, slice.label().c_str(), type_id, filespace_id,
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );

    H5Sselect_hyperslab( filespace_id, H5S_SELECT_SET, offset, NULL, count,
                         NULL );

    memspace_id = H5Screate_simple( 3, dimsm, NULL );
    plist_id = H5Pcreate( H5P_DATASET_XFER );
    // Default IO in HDF5 is independent
    if ( h5_config.collective )
        H5Pset_dxpl_mpio( plist_id, H5FD_MPIO_COLLECTIVE );

    H5Dwrite( dset_id, type_id, memspace_id, filespace_id, plist_id,
              host_view.data() );

    H5Pclose( plist_id );
    H5Sclose( memspace_id );
    H5Dclose( dset_id );
    H5Sclose( filespace_id );

    if ( 0 == comm_rank )
    {
        Impl::writeXdmfAttribute(
            filename_xdmf, slice.label().c_str(), dimsf[0], dimsf[1], dimsf[2],
            dtype.c_str(), precision, filename_hdf5, slice.label().c_str() );
    }
}
//! \endcond
} // namespace Impl

//! Write particle data to HDF5 output.
template <class SliceType>
void writeFields( HDF5Config h5_config, hid_t file_id, std::size_t n_local,
                  std::size_t n_global, hsize_t n_offset, int comm_rank,
                  const char* filename_hdf5, const char* filename_xdmf,
                  const SliceType& slice )
{
    Impl::writeFields( h5_config, file_id, n_local, n_global, n_offset,
                       comm_rank, filename_hdf5, filename_xdmf, slice );
}

//! Write particle data to HDF5 output.
template <class SliceType, class... FieldSliceTypes>
void writeFields( HDF5Config h5_config, hid_t file_id, std::size_t n_local,
                  std::size_t n_global, hsize_t n_offset, int comm_rank,
                  const char* filename_hdf5, const char* filename_xdmf,
                  const SliceType& slice, FieldSliceTypes&&... fields )
{
    Impl::writeFields( h5_config, file_id, n_local, n_global, n_offset,
                       comm_rank, filename_hdf5, filename_xdmf, slice );
    writeFields( h5_config, file_id, n_local, n_global, n_offset, comm_rank,
                 filename_hdf5, filename_xdmf, fields... );
}

//---------------------------------------------------------------------------//
/*!
  \brief Write particle output in HDF5 format.
  \param h5_config HDF5 configuration settings.
  \param prefix Filename prefix.
  \param comm MPI communicator.
  \param time_step_index Current simulation step index.
  \param time Current simulation time.
  \param n_local Number of local particles.
  \param coords_slice Particle coordinates.
  \param fields Variadic list of particle property fields.
*/
template <class CoordSliceType, class... FieldSliceTypes>
void writeTimeStep( HDF5Config h5_config, const std::string& prefix,
                    MPI_Comm comm, const int time_step_index, const double time,
                    const std::size_t n_local,
                    const CoordSliceType& coords_slice,
                    FieldSliceTypes&&... fields )
{
    hid_t plist_id;
    hid_t dset_id;
    hid_t file_id;
    hid_t filespace_id;
    hid_t memspace_id;

    // HDF5 hyperslab parameters
    hsize_t offset[2];
    hsize_t dimsf[2];
    hsize_t count[2];

    int comm_rank;
    MPI_Comm_rank( comm, &comm_rank );
    int comm_size;
    MPI_Comm_size( comm, &comm_size );

    // Compose a data file name.
    std::stringstream filename_hdf5;
    filename_hdf5 << prefix << "_" << time_step_index << ".h5";

    std::stringstream filename_xdmf;
    filename_xdmf << prefix << "_" << time_step_index << ".xmf";

    plist_id = H5Pcreate( H5P_FILE_ACCESS );
    H5Pset_fapl_mpio( plist_id, comm, MPI_INFO_NULL );
    H5Pset_libver_bounds( plist_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST );

#if HDF5_VERSION_GE( 1, 10, 1 )
    if ( h5_config.evict_on_close )
    {
        H5Pset_evict_on_close( plist_id, (hbool_t)1 );
    }
#endif

#if H5_VERSION_GE( 1, 10, 0 )
    if ( h5_config.collective )
    {
        H5Pset_all_coll_metadata_ops( plist_id, 1 );
        H5Pset_coll_metadata_write( plist_id, 1 );
    }
#endif

    if ( h5_config.align )
        H5Pset_alignment( plist_id, h5_config.threshold, h5_config.alignment );

    file_id = H5Fcreate( filename_hdf5.str().c_str(), H5F_ACC_TRUNC,
                         H5P_DEFAULT, plist_id );
    H5Pclose( plist_id );

    // Write current simulation time
    hid_t fspace = H5Screate( H5S_SCALAR );
    hid_t attr_id = H5Acreate( file_id, "Time", H5T_NATIVE_DOUBLE, fspace,
                               H5P_DEFAULT, H5P_DEFAULT );
    H5Awrite( attr_id, H5T_NATIVE_DOUBLE, &time );
    H5Aclose( attr_id );
    H5Sclose( fspace );

    // Reorder the coordinates in a blocked format.
    Kokkos::View<typename CoordSliceType::value_type**, Kokkos::LayoutRight,
                 typename CoordSliceType::device_type>
        coords_view( Kokkos::ViewAllocateWithoutInitializing( "coords" ),
                     coords_slice.size(), coords_slice.extent( 2 ) );
    Kokkos::parallel_for(
        "Cabana::HDF5ParticleOutput::writeCoords",
        Kokkos::RangePolicy<typename CoordSliceType::execution_space>(
            0, coords_slice.size() ),
        KOKKOS_LAMBDA( const int i ) {
            for ( std::size_t d0 = 0; d0 < coords_slice.extent( 2 ); ++d0 )
                coords_view( i, d0 ) = coords_slice( i, d0 );
        } );

    // Mirror the coordinates to the host.
    auto host_coords =
        Kokkos::create_mirror_view_and_copy( Kokkos::HostSpace(), coords_view );

    std::vector<int> all_offsets( comm_size );
    all_offsets[comm_rank] = n_local;

    MPI_Allreduce( MPI_IN_PLACE, all_offsets.data(), comm_size, MPI_INT,
                   MPI_SUM, comm );

    offset[0] = 0;
    offset[1] = 0;

    size_t n_global = 0;
    for ( int i = 0; i < comm_size; i++ )
    {
        if ( i < comm_rank )
        {
            offset[0] += static_cast<hsize_t>( all_offsets[i] );
        }
        n_global += (size_t)all_offsets[i];
    }
    std::vector<int>().swap( all_offsets );

    dimsf[0] = n_global;
    dimsf[1] = 3;

    filespace_id = H5Screate_simple( 2, dimsf, NULL );

    count[0] = n_local;
    count[1] = 3;

    memspace_id = H5Screate_simple( 2, count, NULL );

    plist_id = H5Pcreate( H5P_DATASET_XFER );

    // Default IO in HDF5 is independent
    if ( h5_config.collective )
        H5Pset_dxpl_mpio( plist_id, H5FD_MPIO_COLLECTIVE );

    std::string dtype;
    uint precision;
    hid_t type_id = HDF5Traits<typename CoordSliceType::value_type>::type(
        &dtype, &precision );

    dset_id = H5Dcreate( file_id, "coord_xyz", type_id, filespace_id,
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );

    H5Sselect_hyperslab( filespace_id, H5S_SELECT_SET, offset, NULL, count,
                         NULL );

    H5Dwrite( dset_id, type_id, memspace_id, filespace_id, plist_id,
              host_coords.data() );
    H5Dclose( dset_id );

    H5Pclose( plist_id );
    H5Sclose( filespace_id );
    H5Sclose( memspace_id );

    if ( 0 == comm_rank )
    {
        Impl::writeXdmfHeader( filename_xdmf.str().c_str(), dimsf[0], dimsf[1],
                               dtype.c_str(), precision,
                               filename_hdf5.str().c_str() );
    }

    // Add variables.
    hsize_t n_offset = offset[0];
    writeFields( h5_config, file_id, n_local, n_global, n_offset, comm_rank,
                 filename_hdf5.str().c_str(), filename_xdmf.str().c_str(),
                 fields... );

    H5Fclose( file_id );

    if ( 0 == comm_rank )
        Impl::writeXdmfFooter( filename_xdmf.str().c_str() );
}

//---------------------------------------------------------------------------//

} // namespace HDF5ParticleOutput
} // namespace Experimental
} // end namespace Cabana

#endif // CABANA_HDF5PARTICLEOUTPUT_HPP
